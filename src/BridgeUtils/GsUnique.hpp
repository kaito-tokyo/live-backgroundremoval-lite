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

inline std::mutex &getMutex()
{
	static std::mutex mtx;
	return mtx;
}

inline std::deque<gs_effect_t *> &getEffectsDeque()
{
	static std::deque<gs_effect_t *> effectsToDelete;
	return effectsToDelete;
}

inline std::deque<gs_texture_t *> &getTexturesDeque()
{
	static std::deque<gs_texture_t *> texturesToDelete;
	return texturesToDelete;
}

inline std::deque<gs_stagesurf_t *> &getStagesurfsDeque()
{
	static std::deque<gs_stagesurf_t *> stagesurfsToDelete;
	return stagesurfsToDelete;
}

inline void scheduleEffectToDelete(gs_effect_t *effect)
{
	if (effect) {
		std::lock_guard lock(getMutex());
		getEffectsDeque().push_back(effect);
	}
}

inline void scheduleTextureToDelete(gs_texture_t *texture)
{
	if (texture) {
		std::lock_guard lock(getMutex());
		getTexturesDeque().push_back(texture);
	}
}

inline void scheduleStagesurfsToDelete(gs_stagesurf_t *surface)
{
	if (surface) {
		std::lock_guard lock(getMutex());
		getStagesurfsDeque().push_back(surface);
	}
}

inline void drain()
{
	std::deque<gs_effect_t *> _effects_to_delete;
	std::deque<gs_texture_t *> _textures_to_delete;
	std::deque<gs_stagesurf_t *> _stagesurfs_to_delete;
	{
		std::lock_guard lock(getMutex());
		if (!getEffectsDeque().empty()) {
			_effects_to_delete = std::move(getEffectsDeque());
		}
		if (!getTexturesDeque().empty()) {
			_textures_to_delete = std::move(getTexturesDeque());
		}
		if (!getStagesurfsDeque().empty()) {
			_stagesurfs_to_delete = std::move(getStagesurfsDeque());
		}
	}

	for (gs_effect_t *effect : _effects_to_delete) {
		gs_effect_destroy(effect);
	}
	for (gs_texture_t *texture : _textures_to_delete) {
		gs_texture_destroy(texture);
	}
	for (gs_stagesurf_t *surface : _stagesurfs_to_delete) {
		gs_stagesurface_destroy(surface);
	}
}

struct GsEffectDeleter {
	void operator()(gs_effect_t *effect) const { scheduleEffectToDelete(effect); }
};

struct GsTextureDeleter {
	void operator()(gs_texture_t *texture) const { scheduleTextureToDelete(texture); }
};

struct GsStagesurfDeleter {
	void operator()(gs_stagesurf_t *surface) const { scheduleStagesurfsToDelete(surface); }
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
