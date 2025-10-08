/*
Bridge Utils
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include <obs.h>

#include "ObsUnique.hpp"

namespace KaitoTokyo {
namespace BridgeUtils {

namespace GsUnique {

using GsResourceDeleterPair = std::pair<std::function<void(void *)>, void *>;

inline std::mutex &getMutex()
{
	static std::mutex mtx;
	return mtx;
}

inline std::deque<GsResourceDeleterPair> &getResourceDeque()
{
	static std::deque<GsResourceDeleterPair> resourcesToDelete;
	return resourcesToDelete;
}

inline void scheduleResourceToDelete(void *resource, std::function<void(void *)> deleter)
{
	if (resource) {
		std::lock_guard lock(getMutex());
		getResourceDeque().emplace_back(deleter, resource);
	}
}

inline void drain()
{
	std::deque<GsResourceDeleterPair> resources;
	{
		std::lock_guard lock(getMutex());
		resources = std::move(getResourceDeque());
	}

	for (const auto &pair : resources) {
		pair.first(pair.second);
	}
}

struct GsEffectDeleter {
	void operator()(gs_effect_t *effect) const
	{
		scheduleResourceToDelete(effect, [](void *p) { gs_effect_destroy(static_cast<gs_effect_t *>(p)); });
	}
};

struct GsTextureDeleter {
	void operator()(gs_texture_t *texture) const
	{
		scheduleResourceToDelete(texture, [](void *p) { gs_texture_destroy(static_cast<gs_texture_t *>(p)); });
	}
};

struct GsStagesurfDeleter {
	void operator()(gs_stagesurf_t *surface) const
	{
		scheduleResourceToDelete(surface,
					 [](void *p) { gs_stagesurface_destroy(static_cast<gs_stagesurf_t *>(p)); });
	}
};

} // namespace GsUnique

using unique_gs_effect_t = std::unique_ptr<gs_effect_t, GsUnique::GsEffectDeleter>;

inline unique_gs_effect_t make_unique_gs_effect_from_file(const unique_bfree_char_t &file)
{
	char *raw_error_string = nullptr;
	gs_effect_t *raw_effect = gs_effect_create_from_file(file.get(), &raw_error_string);
	unique_bfree_char_t error_string(raw_error_string);

	if (!raw_effect) {
		throw std::runtime_error(std::string("gs_effect_create_from_file failed: ") +
					 (error_string ? error_string.get() : "(unknown error)"));
	}
	return unique_gs_effect_t(raw_effect);
}

using unique_gs_texture_t = std::unique_ptr<gs_texture_t, GsUnique::GsTextureDeleter>;

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

using unique_gs_stagesurf_t = std::unique_ptr<gs_stagesurf_t, GsUnique::GsStagesurfDeleter>;

inline unique_gs_stagesurf_t make_unique_gs_stagesurf(std::uint32_t width, std::uint32_t height,
						      enum gs_color_format color_format)
{
	gs_stagesurf_t *rawSurface = gs_stagesurface_create(width, height, color_format);
	if (!rawSurface) {
		throw std::runtime_error("gs_stagesurface_create failed");
	}
	return unique_gs_stagesurf_t(rawSurface);
}

class GraphicsContextGuard {
public:
	GraphicsContextGuard() noexcept { obs_enter_graphics(); }
	~GraphicsContextGuard() noexcept { obs_leave_graphics(); }

	GraphicsContextGuard(const GraphicsContextGuard &) = delete;
	GraphicsContextGuard(GraphicsContextGuard &&) = delete;
	GraphicsContextGuard &operator=(const GraphicsContextGuard &) = delete;
	GraphicsContextGuard &operator=(GraphicsContextGuard &&) = delete;
};

} // namespace BridgeUtils
} // namespace KaitoTokyo
