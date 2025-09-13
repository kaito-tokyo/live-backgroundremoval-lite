/*
OBS Background Removal Lite
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

#include "plugin-support.h"
#include <obs.h>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

namespace main_effect_detail {

struct RenderingGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderingGuard(gs_texture_t *targetTexture, gs_blend_type srcBlendType = GS_BLEND_ONE,
		       gs_blend_type dstBlendType = GS_BLEND_ZERO, gs_zstencil_t *targetZStencil = nullptr,
		       gs_color_space targetColorSpace = GS_CS_SRGB)
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
		gs_set_render_target_with_color_space(targetTexture, targetZStencil, targetColorSpace);
		gs_blend_state_push();
		gs_blend_function(srcBlendType, dstBlendType);
	}

	~RenderingGuard()
	{
		gs_blend_state_pop();
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

inline gs_eparam_t *getEffectParam(const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t &effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);

	if (!param) {
		obs_log(LOG_ERROR, "Effect parameter %s not found", name);
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

inline gs_technique_t *getEffectTech(const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);

	if (!tech) {
		obs_log(LOG_ERROR, "Effect technique %s not found", name);
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

} // namespace main_effect_detail

class MainEffect {
public:
	explicit MainEffect(const kaito_tokyo::obs_bridge_utils::unique_bfree_t &effectPath)
		: effect(kaito_tokyo::obs_bridge_utils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage(main_effect_detail::getEffectParam(effect, "image")),
		  textureMask(main_effect_detail::getEffectParam(effect, "mask")),
		  techDraw(main_effect_detail::getEffectTech(effect, "Draw")),
		  techDrawWithMask(main_effect_detail::getEffectTech(effect, "DrawWithMask"))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t effect;

	gs_eparam_t *const textureImage;
	gs_eparam_t *const textureMask;

	gs_technique_t *const techDraw;
	gs_technique_t *const techDrawWithMask;

	void draw(uint32_t width, uint32_t height, gs_texture_t *sourceTexture) noexcept
	{
		gs_technique_t *tech = techDraw;
		size_t passes = gs_technique_begin(tech);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void drawWithMask(uint32_t width, uint32_t height, gs_texture_t *sourceTexture,
			  gs_texture_t *maskTexture) noexcept
	{
		gs_technique_t *tech = techDrawWithMask;
		size_t passes = gs_technique_begin(tech);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);
				gs_effect_set_texture(textureMask, maskTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
