/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - MainFilter Module
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

#include <cstdint>

#include <obs.h>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/GsUnique.hpp>
#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

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

	explicit TextureRenderGuard(const ObsBridgeUtils::unique_gs_texture_t &targetTexture)
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
private:
	gs_eparam_t *getEffectParam(const char *name)
	{
		gs_eparam_t *param = gs_effect_get_param_by_name(gsEffect_.get(), name);

		if (!param) {
			logger_->error("EffectParamNotFoundError", {{"param_name", name}});
			throw std::runtime_error("EffectParamNotFoundError(MainEffect:getEffectParam)");
		}

		return param;
	}

public:
	MainEffect(std::shared_ptr<const Logger::ILogger> logger, const ObsBridgeUtils::unique_bfree_char_t &effectPath)
		: logger_(std::move(logger)),
		  gsEffect_(ObsBridgeUtils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage_(getEffectParam("image")),
		  floatTexelWidth_(getEffectParam("texelWidth")),
		  floatTexelHeight_(getEffectParam("texelHeight")),
		  textureImage1_(getEffectParam("image1")),
		  textureImage2_(getEffectParam("image2")),
		  textureImage3_(getEffectParam("image3")),
		  floatEps_(getEffectParam("eps")),
		  floatGamma_(getEffectParam("gamma")),
		  floatLowerBound_(getEffectParam("lowerBound")),
		  floatUpperBound_(getEffectParam("upperBound")),
		  floatAlpha_(getEffectParam("alpha"))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	void drawSource(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
			obs_source_t *const source) const noexcept
	{
		TextureRenderGuard textureRenderGuard(targetTexture);

		obs_source_t *target = obs_filter_get_target(source);
		if (target == nullptr) {
			logger_->error("Failed to get target source for drawing");
			return;
		}

		gs_set_render_target_with_color_space(targetTexture.get(), nullptr, GS_CS_709_EXTENDED);
		while (gs_effect_loop(gsEffect_.get(), "Draw")) {
			obs_source_video_render(target);
		}
	}

	void drawRoi(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
		     const ObsBridgeUtils::unique_gs_texture_t &sourceTexture, const vec4 *color,
		     const std::uint32_t width = 0, const std::uint32_t height = 0, const float x = 0.0f,
		     const float y = 0.0f) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		gs_clear(GS_CLEAR_COLOR, color, 1.0f, 0);
		gs_matrix_translate3f(x, y, 0.0f);

		while (gs_effect_loop(gsEffect_.get(), "Draw")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, static_cast<uint32_t>(width),
				       static_cast<uint32_t>(height));
		}
	}

	void convertToLuma(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
			   const ObsBridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect_.get(), "ConvertToGrayscale")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void resampleByNearestR8(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
				 const ObsBridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect_.get(), "ResampleByNearestR8")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_draw_sprite(targetTexture.get(), 0, 0u, 0u);
		}
	}

	void calculateSquaredMotion(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
				    const ObsBridgeUtils::unique_gs_texture_t &currentLumaTexture,
				    const ObsBridgeUtils::unique_gs_texture_t &lastLumaTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect_.get(), "CalculateSquaredMotion")) {
			gs_effect_set_texture(textureImage_, currentLumaTexture.get());
			gs_effect_set_texture(textureImage1_, lastLumaTexture.get());
			gs_draw_sprite(currentLumaTexture.get(), 0, 0, 0);
		}
	}

	void reduce(const std::vector<ObsBridgeUtils::unique_gs_texture_t> &reductionPyramidTextures,
		    const ObsBridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		using ObsBridgeUtils::unique_gs_texture_t;

		gs_texture_t *currentSourceTexture = sourceTexture.get();

		for (const unique_gs_texture_t &currentTargetTexture : reductionPyramidTextures) {
			TextureRenderGuard renderTargetGuard(currentTargetTexture);

			const std::uint32_t targetWidth = gs_texture_get_width(currentTargetTexture.get());
			const std::uint32_t targetHeight = gs_texture_get_height(currentTargetTexture.get());

			while (gs_effect_loop(gsEffect_.get(), "Reduce")) {
				gs_effect_set_texture(textureImage_, currentSourceTexture);
				gs_draw_sprite(nullptr, 0, targetWidth, targetHeight);
			}

			currentSourceTexture = currentTargetTexture.get();
		}
	}

	void applyBoxFilterR8KS17(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
				  const ObsBridgeUtils::unique_gs_texture_t &sourceTexture,
				  const ObsBridgeUtils::unique_gs_texture_t &intermediateTexture) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(intermediateTexture);

			float texelWidth = 1.0f / static_cast<float>(gs_texture_get_width(sourceTexture.get()));

			while (gs_effect_loop(gsEffect_.get(), "HorizontalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage_, sourceTexture.get());
				gs_effect_set_float(floatTexelWidth_, texelWidth);
				gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetTexture);

			float texelHeight = 1.0f / static_cast<float>(gs_texture_get_height(intermediateTexture.get()));

			while (gs_effect_loop(gsEffect_.get(), "VerticalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage_, intermediateTexture.get());
				gs_effect_set_float(floatTexelHeight_, texelHeight);
				gs_draw_sprite(intermediateTexture.get(), 0, 0u, 0u);
			}
		}
	}

	void applyBoxFilterWithMulR8KS17(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
					 const ObsBridgeUtils::unique_gs_texture_t &sourceTexture1,
					 const ObsBridgeUtils::unique_gs_texture_t &sourceTexture2,
					 const ObsBridgeUtils::unique_gs_texture_t &intermediateTexture) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(intermediateTexture);

			float texelWidth = 1.0f / static_cast<float>(gs_texture_get_width(sourceTexture1.get()));

			while (gs_effect_loop(gsEffect_.get(), "HorizontalBoxFilterWithMulR8KS17")) {
				gs_effect_set_texture(textureImage_, sourceTexture1.get());
				gs_effect_set_texture(textureImage1_, sourceTexture2.get());
				gs_effect_set_float(floatTexelWidth_, texelWidth);
				gs_draw_sprite(sourceTexture1.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetTexture);

			float texelHeight = 1.0f / static_cast<float>(gs_texture_get_height(intermediateTexture.get()));

			while (gs_effect_loop(gsEffect_.get(), "VerticalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage_, intermediateTexture.get());
				gs_effect_set_float(floatTexelHeight_, texelHeight);
				gs_draw_sprite(intermediateTexture.get(), 0, 0u, 0u);
			}
		}
	}

	void applyBoxFilterWithSqR8KS17(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
					const ObsBridgeUtils::unique_gs_texture_t &sourceTexture,
					const ObsBridgeUtils::unique_gs_texture_t &intermediateTexture) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(intermediateTexture);

			float texelWidth = 1.0f / static_cast<float>(gs_texture_get_width(sourceTexture.get()));

			while (gs_effect_loop(gsEffect_.get(), "HorizontalBoxFilterWithSqR8KS17")) {
				gs_effect_set_texture(textureImage_, sourceTexture.get());
				gs_effect_set_float(floatTexelWidth_, texelWidth);
				gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetTexture);

			float texelHeight = 1.0f / static_cast<float>(gs_texture_get_height(intermediateTexture.get()));

			while (gs_effect_loop(gsEffect_.get(), "VerticalBoxFilterR8KS17")) {
				gs_effect_set_texture(textureImage_, intermediateTexture.get());
				gs_effect_set_float(floatTexelHeight_, texelHeight);
				gs_draw_sprite(intermediateTexture.get(), 0, 0u, 0u);
			}
		}
	}

	void calculateGuidedFilterAAndB(const ObsBridgeUtils::unique_gs_texture_t &targetATexture,
					const ObsBridgeUtils::unique_gs_texture_t &targetBTexture,
					const ObsBridgeUtils::unique_gs_texture_t &sourceMeanGuideSqTexture,
					const ObsBridgeUtils::unique_gs_texture_t &sourceMeanGuideTexture,
					const ObsBridgeUtils::unique_gs_texture_t &sourceMeanGuideSourceTexture,
					const ObsBridgeUtils::unique_gs_texture_t &sourceMeanSourceTexture,
					const float eps) const noexcept
	{
		{
			TextureRenderGuard renderTargetGuard(targetATexture);

			while (gs_effect_loop(gsEffect_.get(), "CalculateGuidedFilterA")) {
				gs_effect_set_texture(textureImage_, sourceMeanGuideSqTexture.get());
				gs_effect_set_texture(textureImage1_, sourceMeanGuideTexture.get());
				gs_effect_set_texture(textureImage2_, sourceMeanGuideSourceTexture.get());
				gs_effect_set_texture(textureImage3_, sourceMeanSourceTexture.get());
				gs_effect_set_float(floatEps_, eps);

				gs_draw_sprite(sourceMeanGuideSqTexture.get(), 0, 0u, 0u);
			}
		}

		{
			TextureRenderGuard renderTargetGuard(targetBTexture);

			while (gs_effect_loop(gsEffect_.get(), "CalculateGuidedFilterB")) {
				gs_effect_set_texture(textureImage_, targetATexture.get());
				gs_effect_set_texture(textureImage1_, sourceMeanSourceTexture.get());
				gs_effect_set_texture(textureImage2_, sourceMeanGuideTexture.get());

				gs_draw_sprite(targetATexture.get(), 0, 0u, 0u);
			}
		}
	}

	void finalizeGuidedFilter(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
				  const ObsBridgeUtils::unique_gs_texture_t &sourceGuideTexture,
				  const ObsBridgeUtils::unique_gs_texture_t &sourceATexture,
				  const ObsBridgeUtils::unique_gs_texture_t &sourceBTexture) const noexcept
	{
		TextureRenderGuard renderTargetGuard(targetTexture);

		while (gs_effect_loop(gsEffect_.get(), "FinalizeGuidedFilter")) {
			gs_effect_set_texture(textureImage_, sourceGuideTexture.get());
			gs_effect_set_texture(textureImage1_, sourceATexture.get());
			gs_effect_set_texture(textureImage2_, sourceBTexture.get());

			gs_draw_sprite(sourceGuideTexture.get(), 0, 0u, 0u);
		}
	}

	void timeAveragedFiltering(const ObsBridgeUtils::unique_gs_texture_t &targetTexture,
				   const ObsBridgeUtils::unique_gs_texture_t &previousMaskTexture,
				   const ObsBridgeUtils::unique_gs_texture_t &sourceTexture,
				   const float alpha) const noexcept
	{
		TextureRenderGuard textureRenderGuard(targetTexture);

		while (gs_effect_loop(gsEffect_.get(), "TimeAveragedFilter")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_effect_set_texture(textureImage1_, previousMaskTexture.get());
			gs_effect_set_float(floatAlpha_, alpha);

			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void directDraw(const ObsBridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		while (gs_effect_loop(gsEffect_.get(), "Draw")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void directDrawWithMask(const ObsBridgeUtils::unique_gs_texture_t &sourceTexture,
				const ObsBridgeUtils::unique_gs_texture_t &maskTexture) const noexcept
	{
		while (gs_effect_loop(gsEffect_.get(), "DrawWithMask")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_effect_set_texture(textureImage1_, maskTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	void directDrawWithRefinedMask(const ObsBridgeUtils::unique_gs_texture_t &sourceTexture,
				       const ObsBridgeUtils::unique_gs_texture_t &maskTexture, const double gamma,
				       const double lowerBound, const double upperBoundMargin) const noexcept
	{
		while (gs_effect_loop(gsEffect_.get(), "DrawWithRefinedMask")) {
			gs_effect_set_texture(textureImage_, sourceTexture.get());
			gs_effect_set_texture(textureImage1_, maskTexture.get());

			gs_effect_set_float(floatGamma_, static_cast<float>(gamma));
			gs_effect_set_float(floatLowerBound_, static_cast<float>(lowerBound));
			gs_effect_set_float(floatUpperBound_, static_cast<float>(1.0 - upperBoundMargin));

			gs_draw_sprite(sourceTexture.get(), 0, 0u, 0u);
		}
	}

	const std::shared_ptr<const Logger::ILogger> logger_;
	const ObsBridgeUtils::unique_gs_effect_t gsEffect_ = nullptr;

	gs_eparam_t *const textureImage_ = nullptr;

	gs_eparam_t *const floatTexelWidth_ = nullptr;
	gs_eparam_t *const floatTexelHeight_ = nullptr;

	gs_eparam_t *const textureImage1_ = nullptr;
	gs_eparam_t *const textureImage2_ = nullptr;
	gs_eparam_t *const textureImage3_ = nullptr;

	gs_eparam_t *const floatEps_ = nullptr;
	gs_eparam_t *const floatGamma_ = nullptr;
	gs_eparam_t *const floatLowerBound_ = nullptr;
	gs_eparam_t *const floatUpperBound_ = nullptr;
	gs_eparam_t *const floatAlpha_ = nullptr;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
