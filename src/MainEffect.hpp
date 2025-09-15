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

#include <cstdint>

#include <obs.h>

#include <obs-bridge-utils/gs_unique.hpp>
#include <obs-bridge-utils/ILogger.hpp>
#include <obs-bridge-utils/unique_bfree.hpp>

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

namespace main_effect_detail {

inline gs_eparam_t *getEffectParam(const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t &effect, const char *name,
				   const kaito_tokyo::obs_bridge_utils::ILogger &logger)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);

	if (!param) {
		logger.error("Effect parameter {} not found", name);
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

inline gs_technique_t *getEffectTech(const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t &effect, const char *name,
				     const kaito_tokyo::obs_bridge_utils::ILogger &logger)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);

	if (!tech) {
		logger.error("Effect technique {} not found", name);
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

} // namespace main_effect_detail

struct TransformStateGuard {
	TransformStateGuard()
	{
		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();
	}
	~TransformStateGuard()
	{
		gs_matrix_pop();
		gs_projection_pop();
		gs_viewport_pop();
	}
};

struct RenderTargetGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderTargetGuard()
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
	}

	~RenderTargetGuard()
	{
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

class MainEffect {
public:
	const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t effect = nullptr;

	gs_eparam_t *const textureImage = nullptr;

	gs_eparam_t *const floatTexelWidth = nullptr;
	gs_eparam_t *const floatTexelHeight = nullptr;
	gs_eparam_t *const intKernelSize = nullptr;

	gs_eparam_t *const textureMask = nullptr;

	gs_technique_t *const techDraw = nullptr;
	gs_technique_t *const techDrawWithMask = nullptr;
	gs_technique_t *const techConvertToGrayscale = nullptr;
	gs_technique_t *const techHorizontalBoxFilterR8 = nullptr;
	gs_technique_t *const techVerticalBoxFilterR8 = nullptr;

	MainEffect(const kaito_tokyo::obs_bridge_utils::unique_bfree_char_t &effectPath,
		   const kaito_tokyo::obs_bridge_utils::ILogger &logger)
		: effect(kaito_tokyo::obs_bridge_utils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage(main_effect_detail::getEffectParam(effect, "image", logger)),
		  floatTexelWidth(main_effect_detail::getEffectParam(effect, "texelWidth", logger)),
		  floatTexelHeight(main_effect_detail::getEffectParam(effect, "texelHeight", logger)),
		  intKernelSize(main_effect_detail::getEffectParam(effect, "kernelSize", logger)),
		  textureMask(main_effect_detail::getEffectParam(effect, "mask", logger)),
		  techDraw(main_effect_detail::getEffectTech(effect, "Draw", logger)),
		  techDrawWithMask(main_effect_detail::getEffectTech(effect, "DrawWithMask", logger)),
		  techConvertToGrayscale(main_effect_detail::getEffectTech(effect, "ConvertToGrayscale", logger)),
		  techHorizontalBoxFilterR8(main_effect_detail::getEffectTech(effect, "HorizontalBoxFilterR8", logger)),
		  techVerticalBoxFilterR8(main_effect_detail::getEffectTech(effect, "VerticalBoxFilterR8", logger))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	void draw(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture) const noexcept
	{
		gs_technique_t *tech = techDraw;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void drawWithMask(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture,
			  gs_texture_t *maskTexture) const noexcept
	{
		gs_technique_t *tech = techDrawWithMask;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);
				gs_effect_set_texture(textureMask, maskTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void convertToGrayscale(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture) const noexcept
	{
		gs_technique_t *tech = techConvertToGrayscale;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void applyBoxFilterR8(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture, gs_texture_t *targetTexture, int kernelSize, gs_texture_t *temporaryTexture1) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();
	
		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_render_target_with_color_space(temporaryTexture1, nullptr, GS_CS_SRGB);
		std::size_t passesHorizontal = gs_technique_begin(techHorizontalBoxFilterR8);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(techHorizontalBoxFilterR8, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_effect_set_int(intKernelSize, kernelSize);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techHorizontalBoxFilterR8);
			}
		}
		gs_technique_end(techHorizontalBoxFilterR8);

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		std::size_t passesVertical = gs_technique_begin(techVerticalBoxFilterR8);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(techVerticalBoxFilterR8, i)) {
				gs_effect_set_texture(textureImage, temporaryTexture1);

				gs_effect_set_float(floatTexelHeight, texelHeight);
				gs_effect_set_int(intKernelSize, kernelSize);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techVerticalBoxFilterR8);
			}
		}
		gs_technique_end(techVerticalBoxFilterR8);
	}
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
