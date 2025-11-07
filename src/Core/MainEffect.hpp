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

#include <ILogger.hpp>

#include <GsUnique.hpp>
#include <ObsUnique.hpp>

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

namespace main_effect_detail {

inline gs_eparam_t *getEffectParam(const BridgeUtils::unique_gs_effect_t &effect, const char *name,
				   const Logger::ILogger &logger)
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

struct TextureRenderGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	explicit TextureRenderGuard(const BridgeUtils::unique_gs_texture_t &targetTexture)
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
		gs_set_render_target_with_color_space(targetTexture.get(), nullptr, GS_CS_709_EXTENDED);

		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();
		gs_blend_state_push();

		std::uint32_t targetWidth = gs_texture_get_width(targetTexture.get());
		std::uint32_t targetHeight = gs_texture_get_height(targetTexture.get());
		gs_set_viewport(0, 0, static_cast<int>(targetWidth), static_cast<int>(targetHeight));
		gs_ortho(0.0f, static_cast<float>(targetWidth), 0.0f, static_cast<float>(targetHeight), -100.0f,
			 100.0f);
		gs_matrix_identity();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	}

	~TextureRenderGuard()
	{
		gs_blend_state_pop();
		gs_matrix_pop();
		gs_projection_pop();
		gs_viewport_pop();

		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

struct MainEffect {
public:
	MainEffect(const BridgeUtils::unique_bfree_char_t &effectPath, const Logger::ILogger &logger)
		: gsEffect(BridgeUtils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage(main_effect_detail::getEffectParam(gsEffect, "image", logger)),
		  floatTexelWidth(main_effect_detail::getEffectParam(gsEffect, "texelWidth", logger)),
		  floatTexelHeight(main_effect_detail::getEffectParam(gsEffect, "texelHeight", logger)),
		  textureImage1(main_effect_detail::getEffectParam(gsEffect, "image1", logger)),
		  textureImage2(main_effect_detail::getEffectParam(gsEffect, "image2", logger)),
		  textureImage3(main_effect_detail::getEffectParam(gsEffect, "image3", logger)),
		  floatEps(main_effect_detail::getEffectParam(gsEffect, "eps", logger)),
		  floatGamma(main_effect_detail::getEffectParam(gsEffect, "gamma", logger)),
		  floatLowerBound(main_effect_detail::getEffectParam(gsEffect, "lowerBound", logger)),
		  floatUpperBound(main_effect_detail::getEffectParam(gsEffect, "upperBound", logger)),
		  floatAlpha(main_effect_detail::getEffectParam(gsEffect, "alpha", logger))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	void drawSource(const BridgeUtils::unique_gs_texture_t &targetTexture,
			obs_source_t *const source) const noexcept
	{
		TextureRenderGuard textureRenderGuard(targetTexture);

		obs_source_t *target = obs_filter_get_target(source);
		gs_set_render_target_with_color_space(targetTexture.get(), nullptr, GS_CS_709_EXTENDED);
		while (gs_effect_loop(gsEffect.get(), "Draw")) {
			obs_source_video_render(target);
		}
	}

	void drawRoi(const BridgeUtils::unique_gs_texture_t &targetTexture,
		     const BridgeUtils::unique_gs_texture_t &sourceTexture, const vec4 *color,
		     const std::uint32_t width = 0, const std::uint32_t height = 0, const float x = 0.0f,
		     const float y = 0.0f) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		gs_clear(GS_CLEAR_COLOR, color, 1.0f, 0);
		gs_matrix_translate3f(-x, -y, 0.0f);

		while (gs_effect_loop(gsEffect.get(), "Draw")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, static_cast<uint32_t>(width),
				       static_cast<uint32_t>(height));
		}
	}

	void convertToLuma(const BridgeUtils::unique_gs_texture_t &targetTexture,
			   const BridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect.get(), "ConvertToGrayscale")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void resampleByNearestR8(const BridgeUtils::unique_gs_texture_t &targetTexture,
				 const BridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect.get(), "ResampleByNearestR8")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_draw_sprite(targetTexture.get(), 0, 0u, 0u);
		}
	}

	void calculateSquaredMotion(const BridgeUtils::unique_gs_texture_t &targetTexture,
				    const BridgeUtils::unique_gs_texture_t &currentLumaTexture,
				    const BridgeUtils::unique_gs_texture_t &lastLumaTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect.get(), "CalculateSquaredMotion")) {
			gs_effect_set_texture(textureImage, currentLumaTexture.get());
			gs_effect_set_texture(textureImage1, lastLumaTexture.get());
			gs_draw_sprite(currentLumaTexture.get(), 0, 0, 0);
		}
	}

	void reduce(const std::vector<BridgeUtils::unique_gs_texture_t> &reductionPyramidTextures,
		    const BridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		using BridgeUtils::unique_gs_texture_t;

		gs_texture_t *currentSourceTexture = sourceTexture.get();

		for (const unique_gs_texture_t &currentTargetTexture : reductionPyramidTextures) {
			TextureRenderGuard renderTargetGuard(currentTargetTexture);

			const std::uint32_t targetWidth = gs_texture_get_width(currentTargetTexture.get());
			const std::uint32_t targetHeight = gs_texture_get_height(currentTargetTexture.get());

			while (gs_effect_loop(gsEffect.get(), "Reduce")) {
				gs_effect_set_texture(textureImage, currentSourceTexture);
				gs_draw_sprite(nullptr, 0, targetWidth, targetHeight);
			}

			currentSourceTexture = currentTargetTexture.get();
		}
	}

	void applyBoxFilterR8KS17(const BridgeUtils::unique_gs_texture_t &targetTexture,
				  const BridgeUtils::unique_gs_texture_t &sourceTexture,
				  const BridgeUtils::unique_gs_texture_t &intermediateTexture) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(intermediateTexture);

			float texelWidth = 1.0f / static_cast<float>(gs_texture_get_width(sourceTexture.get()));

			while (gs_effect_loop(gsEffect.get(), "HorizontalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage, sourceTexture.get());
				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetTexture);

			float texelHeight = 1.0f / static_cast<float>(gs_texture_get_height(intermediateTexture.get()));

			while (gs_effect_loop(gsEffect.get(), "VerticalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage, intermediateTexture.get());
				gs_effect_set_float(floatTexelHeight, texelHeight);
				gs_draw_sprite(intermediateTexture.get(), 0, 0u, 0u);
			}
		}
	}

	void applyBoxFilterWithMulR8KS17(const BridgeUtils::unique_gs_texture_t &targetTexture,
					 const BridgeUtils::unique_gs_texture_t &sourceTexture1,
					 const BridgeUtils::unique_gs_texture_t &sourceTexture2,
					 const BridgeUtils::unique_gs_texture_t &intermediateTexture) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(intermediateTexture);

			float texelWidth = 1.0f / static_cast<float>(gs_texture_get_width(sourceTexture1.get()));

			while (gs_effect_loop(gsEffect.get(), "HorizontalBoxFilterWithMulR8KS17")) {
				gs_effect_set_texture(textureImage, sourceTexture1.get());
				gs_effect_set_texture(textureImage1, sourceTexture2.get());
				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_draw_sprite(sourceTexture1.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetTexture);

			float texelHeight = 1.0f / static_cast<float>(gs_texture_get_height(intermediateTexture.get()));

			while (gs_effect_loop(gsEffect.get(), "VerticalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage, intermediateTexture.get());
				gs_effect_set_float(floatTexelHeight, texelHeight);
				gs_draw_sprite(intermediateTexture.get(), 0, 0u, 0u);
			}
		}
	}

	void applyBoxFilterWithSqR8KS17(const BridgeUtils::unique_gs_texture_t &targetTexture,
					const BridgeUtils::unique_gs_texture_t &sourceTexture,
					const BridgeUtils::unique_gs_texture_t &intermediateTexture) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(intermediateTexture);

			float texelWidth = 1.0f / static_cast<float>(gs_texture_get_width(sourceTexture.get()));

			while (gs_effect_loop(gsEffect.get(), "HorizontalBoxFilterWithSqR8KS17")) {
				gs_effect_set_texture(textureImage, sourceTexture.get());
				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetTexture);

			float texelHeight = 1.0f / static_cast<float>(gs_texture_get_height(intermediateTexture.get()));

			while (gs_effect_loop(gsEffect.get(), "VerticalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage, intermediateTexture.get());
				gs_effect_set_float(floatTexelHeight, texelHeight);
				gs_draw_sprite(intermediateTexture.get(), 0, 0u, 0u);
			}
		}
	}

	void calculateGuidedFilterAAndB(const BridgeUtils::unique_gs_texture_t &targetATexture,
					const BridgeUtils::unique_gs_texture_t &targetBTexture,
					const BridgeUtils::unique_gs_texture_t &sourceMeanGuideSqTexture,
					const BridgeUtils::unique_gs_texture_t &sourceMeanGuideTexture,
					const BridgeUtils::unique_gs_texture_t &sourceMeanGuideSourceTexture,
					const BridgeUtils::unique_gs_texture_t &sourceMeanSourceTexture,
					const float eps) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(targetATexture);

			while (gs_effect_loop(gsEffect.get(), "CalculateGuidedFilterA")) {
				gs_effect_set_texture(textureImage, sourceMeanGuideSqTexture.get());
				gs_effect_set_texture(textureImage1, sourceMeanGuideTexture.get());
				gs_effect_set_texture(textureImage2, sourceMeanGuideSourceTexture.get());
				gs_effect_set_texture(textureImage3, sourceMeanSourceTexture.get());
				gs_effect_set_float(floatEps, eps);

				gs_draw_sprite(sourceMeanGuideSqTexture.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetBTexture);

			while (gs_effect_loop(gsEffect.get(), "CalculateGuidedFilterB")) {
				gs_effect_set_texture(textureImage, targetATexture.get());
				gs_effect_set_texture(textureImage1, sourceMeanSourceTexture.get());
				gs_effect_set_texture(textureImage2, sourceMeanGuideTexture.get());

				gs_draw_sprite(targetATexture.get(), 0, 0u, 0u);
			}
		}
	}

	void finalizeGuidedFilter(const BridgeUtils::unique_gs_texture_t &targetTexture,
				  const BridgeUtils::unique_gs_texture_t &sourceGuideTexture,
				  const BridgeUtils::unique_gs_texture_t &sourceATexture,
				  const BridgeUtils::unique_gs_texture_t &sourceBTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect.get(), "FinalizeGuidedFilter")) {
			gs_effect_set_texture(textureImage, sourceGuideTexture.get());
			gs_effect_set_texture(textureImage1, sourceATexture.get());
			gs_effect_set_texture(textureImage2, sourceBTexture.get());

			gs_draw_sprite(sourceGuideTexture.get(), 0, 0u, 0u);
		}
	}

	void timeAveragedFiltering(const BridgeUtils::unique_gs_texture_t &targetTexture,
				   const BridgeUtils::unique_gs_texture_t &previousMaskTexture,
				   const BridgeUtils::unique_gs_texture_t &sourceTexture,
				   const float alpha) const noexcept
	{
		TextureRenderGuard textureRenderGuard(targetTexture);

		while (gs_effect_loop(gsEffect.get(), "TimeAveragedFilter")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_effect_set_texture(textureImage1, previousMaskTexture.get());
			gs_effect_set_float(floatAlpha, alpha);

			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void directDraw(const BridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		while (gs_effect_loop(gsEffect.get(), "Draw")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void directDrawWithMask(const BridgeUtils::unique_gs_texture_t &sourceTexture,
				const BridgeUtils::unique_gs_texture_t &maskTexture) const noexcept
	{
		while (gs_effect_loop(gsEffect.get(), "DrawWithMask")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_effect_set_texture(textureImage1, maskTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void directDrawWithRefinedMask(const BridgeUtils::unique_gs_texture_t &sourceTexture,
				       const BridgeUtils::unique_gs_texture_t &maskTexture, const double gamma,
				       const double lowerBound, const double upperBoundMargin) const noexcept
	{
		while (gs_effect_loop(gsEffect.get(), "DrawWithRefinedMask")) {
			gs_effect_set_texture(textureImage, sourceTexture.get());
			gs_effect_set_texture(textureImage1, maskTexture.get());

			gs_effect_set_float(floatGamma, static_cast<float>(gamma));
			gs_effect_set_float(floatLowerBound, static_cast<float>(lowerBound));
			gs_effect_set_float(floatUpperBound, static_cast<float>(1.0 - upperBoundMargin));

			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	const BridgeUtils::unique_gs_effect_t gsEffect = nullptr;

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
	gs_eparam_t *const floatAlpha = nullptr;
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
