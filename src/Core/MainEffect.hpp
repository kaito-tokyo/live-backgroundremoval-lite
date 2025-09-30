/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>

#include <obs.h>

#include "BridgeUtils/GsUnique.hpp"
#include "BridgeUtils/ILogger.hpp"
#include "BridgeUtils/ObsUnique.hpp"

namespace KaitoTokyo {
namespace BackgroundRemovalLite {

namespace main_effect_detail {

inline gs_eparam_t *getEffectParam(const BridgeUtils::unique_gs_effect_t &effect, const char *name,
				   const BridgeUtils::ILogger &logger)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);

	if (!param) {
		logger.error("Effect parameter {} not found", name);
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
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
	const BridgeUtils::unique_gs_effect_t effect = nullptr;

	gs_eparam_t *const textureImage = nullptr;

	gs_eparam_t *const floatTexelWidth = nullptr;
	gs_eparam_t *const floatTexelHeight = nullptr;

	gs_eparam_t *const textureImage1 = nullptr;
	gs_eparam_t *const textureImage2 = nullptr;
	gs_eparam_t *const textureImage3 = nullptr;

	gs_eparam_t *const floatEps = nullptr;
	gs_eparam_t *const floatGamma = nullptr;
	gs_eparam_t *const floatLowerBound = nullptr;
	gs_eparam_t *const floatUpperBound = nullptr;

	MainEffect(const BridgeUtils::unique_bfree_char_t &effectPath, const BridgeUtils::ILogger &logger)
		: effect(BridgeUtils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage(main_effect_detail::getEffectParam(effect, "image", logger)),
		  floatTexelWidth(main_effect_detail::getEffectParam(effect, "texelWidth", logger)),
		  floatTexelHeight(main_effect_detail::getEffectParam(effect, "texelHeight", logger)),
		  textureImage1(main_effect_detail::getEffectParam(effect, "image1", logger)),
		  textureImage2(main_effect_detail::getEffectParam(effect, "image2", logger)),
		  textureImage3(main_effect_detail::getEffectParam(effect, "image3", logger)),
		  floatEps(main_effect_detail::getEffectParam(effect, "eps", logger)),
		  floatGamma(main_effect_detail::getEffectParam(effect, "gamma", logger)),
		  floatLowerBound(main_effect_detail::getEffectParam(effect, "lowerBound", logger)),
		  floatUpperBound(main_effect_detail::getEffectParam(effect, "upperBound", logger))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	void draw(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture) const noexcept
	{
		while (gs_effect_loop(effect.get(), "Draw")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void drawWithMask(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture,
			  gs_texture_t *maskTexture) const noexcept
	{
		while (gs_effect_loop(effect.get(), "DrawWithMask")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_effect_set_texture(textureImage1, maskTexture);
			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void drawWithRefinedMask(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture,
				 gs_texture_t *maskTexture, double gamma, double lowerBound,
				 double upperBound) const noexcept
	{
		while (gs_effect_loop(effect.get(), "DrawWithRefinedMask")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_effect_set_texture(textureImage1, maskTexture);

			gs_effect_set_float(floatGamma, static_cast<float>(gamma));
			gs_effect_set_float(floatLowerBound, static_cast<float>(lowerBound));
			gs_effect_set_float(floatUpperBound, static_cast<float>(upperBound));

			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void resampleByNearestR8(std::uint32_t targetWidth, std::uint32_t targetHeight, gs_texture_t *targetTexture,
				 gs_texture_t *sourceTexture) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, targetWidth, targetHeight);
		gs_ortho(0.0f, static_cast<float>(targetWidth), 0.0f, static_cast<float>(targetHeight), -100.0f,
			 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "ResampleByNearestR8")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_draw_sprite(nullptr, 0, targetWidth, targetHeight);
		}
	}

	void convertToGrayscale(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
				gs_texture_t *sourceTexture) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "ConvertToGrayscale")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void applyBoxFilterR8KS17(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
				  gs_texture_t *sourceTexture, gs_texture_t *temporaryTexture1) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_render_target_with_color_space(temporaryTexture1, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "HorizontalBoxFilterR8KS17")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_effect_set_float(floatTexelWidth, texelWidth);
			gs_draw_sprite(nullptr, 0, width, height);
		}

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "VerticalBoxFilterR8KS17")) {
			gs_effect_set_texture(textureImage, temporaryTexture1);
			gs_effect_set_float(floatTexelHeight, texelHeight);
			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void applyBoxFilterWithMulR8KS17(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
					 gs_texture_t *source1Texture, gs_texture_t *source2Texture,
					 gs_texture_t *temporaryTexture1) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_render_target_with_color_space(temporaryTexture1, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "HorizontalBoxFilterWithMulR8KS17")) {
			gs_effect_set_texture(textureImage, source1Texture);
			gs_effect_set_texture(textureImage1, source2Texture);

			gs_effect_set_float(floatTexelWidth, texelWidth);

			gs_draw_sprite(nullptr, 0, width, height);
		}

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "VerticalBoxFilterR8KS17")) {
			gs_effect_set_texture(textureImage, temporaryTexture1);
			gs_effect_set_float(floatTexelHeight, texelHeight);
			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void applyBoxFilterWithSqR8KS17(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
					gs_texture_t *sourceTexture, gs_texture_t *temporaryTexture1) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_render_target_with_color_space(temporaryTexture1, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "HorizontalBoxFilterWithSqR8KS17")) {
			gs_effect_set_texture(textureImage, sourceTexture);
			gs_effect_set_float(floatTexelWidth, texelWidth);
			gs_draw_sprite(nullptr, 0, width, height);
		}

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "VerticalBoxFilterR8KS17")) {
			gs_effect_set_texture(textureImage, temporaryTexture1);

			gs_effect_set_float(floatTexelHeight, texelHeight);

			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void calculateGuidedFilterAAndB(std::uint32_t width, std::uint32_t height, gs_texture_t *targetATexture,
					gs_texture_t *targetBTexture, gs_texture_t *sourceMeanGuideSqTexture,
					gs_texture_t *sourceMeanGuideTexture,
					gs_texture_t *sourceMeanGuideSourceTexture,
					gs_texture_t *sourceMeanSourceTexture, float eps) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(targetATexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "CalculateGuidedFilterA")) {
			gs_effect_set_texture(textureImage, sourceMeanGuideSqTexture);
			gs_effect_set_texture(textureImage1, sourceMeanGuideTexture);
			gs_effect_set_texture(textureImage2, sourceMeanGuideSourceTexture);
			gs_effect_set_texture(textureImage3, sourceMeanSourceTexture);
			gs_effect_set_float(floatEps, eps);

			gs_draw_sprite(nullptr, 0, width, height);
		}

		gs_set_render_target_with_color_space(targetBTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "CalculateGuidedFilterB")) {
			gs_effect_set_texture(textureImage, targetATexture);
			gs_effect_set_texture(textureImage1, sourceMeanSourceTexture);
			gs_effect_set_texture(textureImage2, sourceMeanGuideTexture);

			gs_draw_sprite(nullptr, 0, width, height);
		}
	}

	void finalizeGuidedFilter(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
				  gs_texture_t *sourceGuideTexture, gs_texture_t *sourceATexture,
				  gs_texture_t *sourceBTexture) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		while (gs_effect_loop(effect.get(), "FinalizeGuidedFilter")) {
			gs_effect_set_texture(textureImage, sourceGuideTexture);
			gs_effect_set_texture(textureImage1, sourceATexture);
			gs_effect_set_texture(textureImage2, sourceBTexture);

			gs_draw_sprite(nullptr, 0, width, height);
		}
	}
};

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
