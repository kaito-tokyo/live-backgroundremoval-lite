/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * KaitoTokyo ObsBridgeUtils Library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include <obs.h>

#include "ObsUnique.hpp"

namespace KaitoTokyo {
namespace ObsBridgeUtils {

/**
 * @brief Internal helpers for managing Graphics Suite (GS) resources
 * via a thread-safe, deferred deletion mechanism.
 *
 * This namespace provides custom deleters for std::unique_ptr that,
 * instead of deleting the resource immediately, schedule it for deletion
 * on a dedicated queue. This queue must be manually processed (drained)
 * from a thread that has a valid graphics context (e.g., the OBS render thread).
 */
namespace GsUnique {

/**
 * @brief A pair holding a type-erased deleter function and a void pointer
 * to the resource.
 */
using GsResourceDeleterPair = std::pair<std::function<void(void *)>, void *>;

/**
 * @brief Retrieves the global mutex protecting the resource deletion deque.
 * @return A reference to the static std::mutex.
 */
inline std::mutex &getMutex()
{
	static std::mutex mtx;
	return mtx;
}

/**
 * @brief Retrieves the global deque used to store resources pending deletion.
 * @return A reference to the static std::deque.
 */
inline std::deque<GsResourceDeleterPair> &getResourceDeque()
{
	static std::deque<GsResourceDeleterPair> resourcesToDelete;
	return resourcesToDelete;
}

/**
 * @brief Schedules a raw resource pointer for deferred deletion.
 *
 * This function is thread-safe. It adds the resource and its corresponding
 * deleter function to a global queue.
 *
 * @param resource The raw resource pointer to delete (e.g., gs_effect_t*).
 * @param deleter A std::function that knows how to correctly destroy the resource.
 */
inline void scheduleResourceToDelete(void *resource, std::function<void(void *)> deleter)
{
	if (resource) {
		std::lock_guard lock(getMutex());
		getResourceDeque().emplace_back(deleter, resource);
	}
}

/**
 * @brief Drains the deferred deletion queue, freeing all pending resources.
 *
 * This function MUST be called from a thread that has a valid graphics
 * context (e.g., the OBS render thread).
 *
 * It is thread-safe and designed to hold the lock for the shortest possible
 * duration by swapping the global deque with a local one before processing.
 */
inline void drain()
{
	std::deque<GsResourceDeleterPair> resources;
	{
		std::lock_guard lock(getMutex());
		resources.swap(getResourceDeque());
	}

	// Process deletions outside the lock
	for (const auto &pair : resources) {
		pair.first(pair.second); // Call the specific deleter (e.g., gs_effect_destroy)
	}
}

/**
 * @brief Custom deleter for unique_gs_effect_t.
 * Schedules the gs_effect_t for deferred deletion.
 */
struct GsEffectDeleter {
	void operator()(gs_effect_t *effect) const noexcept
	{
		scheduleResourceToDelete(effect, [](void *p) { gs_effect_destroy(static_cast<gs_effect_t *>(p)); });
	}
};

/**
 * @brief Custom deleter for unique_gs_texture_t.
 * Schedules the gs_texture_t for deferred deletion.
 */
struct GsTextureDeleter {
	void operator()(gs_texture_t *texture) const noexcept
	{
		scheduleResourceToDelete(texture, [](void *p) { gs_texture_destroy(static_cast<gs_texture_t *>(p)); });
	}
};

/**
 * @brief Custom deleter for unique_gs_stagesurf_t.
 * Schedules the gs_stagesurf_t for deferred deletion.
 */
struct GsStagesurfDeleter {
	void operator()(gs_stagesurf_t *surface) const noexcept
	{
		scheduleResourceToDelete(surface,
					 [](void *p) { gs_stagesurface_destroy(static_cast<gs_stagesurf_t *>(p)); });
	}
};

} // namespace GsUnique

/**
 * @brief A std::unique_ptr for a gs_effect_t that uses deferred deletion.
 */
using unique_gs_effect_t = std::unique_ptr<gs_effect_t, GsUnique::GsEffectDeleter>;

/**
 * @brief A std::unique_ptr for a gs_texture_t that uses deferred deletion.
 */
using unique_gs_texture_t = std::unique_ptr<gs_texture_t, GsUnique::GsTextureDeleter>;

/**
 * @brief A std::unique_ptr for a gs_stagesurf_t that uses deferred deletion.
 */
using unique_gs_stagesurf_t = std::unique_ptr<gs_stagesurf_t, GsUnique::GsStagesurfDeleter>;

/**
 * @class GraphicsContextGuard
 * @brief An RAII helper to ensure obs_enter_graphics() and obs_leave_graphics()
 * are called in a balanced way.
 *
 * Creates a guard object on the stack to acquire the graphics context for
 * the current scope. The context is released when the guard is destroyed.
 */
class GraphicsContextGuard final {
public:
	/**
	 * @brief Enters the OBS graphics context.
	 */
	GraphicsContextGuard() noexcept { obs_enter_graphics(); }

	/**
	 * @brief Leaves the OBS graphics context.
	 */
	~GraphicsContextGuard() noexcept { obs_leave_graphics(); }

	// Delete copy and move operations to prevent misuse.
	GraphicsContextGuard(const GraphicsContextGuard &) = delete;
	GraphicsContextGuard(GraphicsContextGuard &&) = delete;
	GraphicsContextGuard &operator=(const GraphicsContextGuard &) = delete;
	GraphicsContextGuard &operator=(GraphicsContextGuard &&) = delete;
};

/**
 * @brief Factory function to create a unique_gs_effect_t from a file path.
 *
 * @param file A unique_bfree_char_t containing the path to the effect file.
 * @return A valid (non-null) unique_gs_effect_t managing the created effect.
 * @throws std::runtime_error If effect creation fails (e.g., file not found or compile error).
 * This function throws on failure and **never returns an empty (null) pointer.**
 */
[[nodiscard]]
inline unique_gs_effect_t make_unique_gs_effect_from_file(const unique_bfree_char_t &file)
{
	char *raw_error_string = nullptr;
	gs_effect_t *raw_effect = gs_effect_create_from_file(file.get(), &raw_error_string);
	unique_bfree_char_t error_string(raw_error_string); // Manage error string memory

	if (!raw_effect) {
		throw std::runtime_error(std::string("gs_effect_create_from_file failed: ") +
					 (error_string ? error_string.get() : "(unknown error)"));
	}
	return unique_gs_effect_t(raw_effect);
}

/**
 * @brief Factory function to create a unique_gs_texture_t.
 *
 * @param width Texture width.
 * @param height Texture height.
 * @param color_format Texture color format.
 * @param levels Mipmap levels.
 * @param data Pointer to raw pixel data array.
 * @param flags Texture flags (e.g., GS_DYNAMIC).
 * @return A valid (non-null) unique_gs_texture_t managing the created texture.
 * @throws std::runtime_error If texture creation fails.
 * This function throws on failure and **never returns an empty (null) pointer.**
 */
[[nodiscard]]
inline unique_gs_texture_t make_unique_gs_texture(std::uint32_t width, std::uint32_t height,
						  enum gs_color_format color_format, std::uint32_t levels,
						  const std::uint8_t **data, std::uint32_t flags)
{
	gs_texture_t *rawTexture = gs_texture_create(width, height, color_format, levels, data, flags);
	if (!rawTexture) {
		throw std::runtime_error("gs_texture_create failed");
	}
	return unique_gs_texture_t(rawTexture);
}

/**
 * @brief Factory function to create a unique_gs_stagesurf_t (staging surface).
 *
 * @param width Surface width.
 * @param height Surface height.
 * @param color_format Surface color format.
 * @return A valid (non-null) unique_gs_stagesurf_t managing the created surface.
 * @throws std::runtime_error If surface creation fails (e.g., memory allocation failure).
 * This function throws on failure and **never returns an empty (null) pointer.**
 */
[[nodiscard]]
inline unique_gs_stagesurf_t make_unique_gs_stagesurf(std::uint32_t width, std::uint32_t height,
						      enum gs_color_format color_format)
{
	gs_stagesurf_t *rawSurface = gs_stagesurface_create(width, height, color_format);
	if (!rawSurface) {
		throw std::runtime_error("gs_stagesurface_create failed");
	}
	return unique_gs_stagesurf_t(rawSurface);
}

} // namespace ObsBridgeUtils
} // namespace KaitoTokyo
