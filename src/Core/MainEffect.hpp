/*
Background Removal Lite
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

inline gs_technique_t *getEffectTech(const BridgeUtils::unique_gs_effect_t &effect, const char *name,
				     const BridgeUtils::ILogger &logger)
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

	gs_technique_t *const techDraw = nullptr;
	gs_technique_t *const techDrawWithMask = nullptr;
	gs_technique_t *const techDrawWithRefinedMask = nullptr;
	gs_technique_t *const techResampleByNearestR8 = nullptr;
	gs_technique_t *const techConvertToGrayscale = nullptr;
	gs_technique_t *const techCalculateDifferenceWithMask = nullptr;
	gs_technique_t *const techReduce = nullptr;
	gs_technique_t *const techHorizontalBoxFilterR8KS17 = nullptr;
	gs_technique_t *const techHorizontalBoxFilterWithMulR8KS17 = nullptr;
	gs_technique_t *const techHorizontalBoxFilterWithSqR8KS17 = nullptr;
	gs_technique_t *const techVerticalBoxFilterR8KS17 = nullptr;
	gs_technique_t *const techMultiplyR8 = nullptr;
	gs_technique_t *const techSquareR8 = nullptr;
	gs_technique_t *const techCalculateGuidedFilterA = nullptr;
	gs_technique_t *const techCalculateGuidedFilterB = nullptr;
	gs_technique_t *const techFinalizeGuidedFilter = nullptr;

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
		  floatUpperBound(main_effect_detail::getEffectParam(effect, "upperBound", logger)),
		  techDraw(main_effect_detail::getEffectTech(effect, "Draw", logger)),
		  techDrawWithMask(main_effect_detail::getEffectTech(effect, "DrawWithMask", logger)),
		  techDrawWithRefinedMask(main_effect_detail::getEffectTech(effect, "DrawWithRefinedMask", logger)),
		  techResampleByNearestR8(main_effect_detail::getEffectTech(effect, "ResampleByNearestR8", logger)),
		  techConvertToGrayscale(main_effect_detail::getEffectTech(effect, "ConvertToGrayscale", logger)),
		  techReduce(main_effect_detail::getEffectTech(effect, "Reduce", logger)),
		  techCalculateDifferenceWithMask(
			  main_effect_detail::getEffectTech(effect, "CalculateDifferenceWithMask", logger)),
		  techHorizontalBoxFilterR8KS17(
			  main_effect_detail::getEffectTech(effect, "HorizontalBoxFilterR8KS17", logger)),
		  techHorizontalBoxFilterWithMulR8KS17(
			  main_effect_detail::getEffectTech(effect, "HorizontalBoxFilterWithMulR8KS17", logger)),
		  techHorizontalBoxFilterWithSqR8KS17(
			  main_effect_detail::getEffectTech(effect, "HorizontalBoxFilterWithSqR8KS17", logger)),
		  techVerticalBoxFilterR8KS17(
			  main_effect_detail::getEffectTech(effect, "VerticalBoxFilterR8KS17", logger)),
		  techCalculateGuidedFilterA(
			  main_effect_detail::getEffectTech(effect, "CalculateGuidedFilterA", logger)),
		  techCalculateGuidedFilterB(
			  main_effect_detail::getEffectTech(effect, "CalculateGuidedFilterB", logger)),
		  techFinalizeGuidedFilter(main_effect_detail::getEffectTech(effect, "FinalizeGuidedFilter", logger))
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
				gs_effect_set_texture(textureImage1, maskTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void drawWithRefinedMask(std::uint32_t width, std::uint32_t height, gs_texture_t *sourceTexture,
				 gs_texture_t *maskTexture, double gamma, double lowerBound,
				 double upperBound) const noexcept
	{
		gs_technique_t *tech = techDrawWithRefinedMask;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);
				gs_effect_set_texture(textureImage1, maskTexture);

				gs_effect_set_float(floatGamma, static_cast<float>(gamma));
				gs_effect_set_float(floatLowerBound, static_cast<float>(lowerBound));
				gs_effect_set_float(floatUpperBound, static_cast<float>(upperBound));

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
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
		gs_technique_t *tech = techResampleByNearestR8;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_draw_sprite(nullptr, 0, targetWidth, targetHeight);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
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

	void calculateDifferenceWithMask(std::uint32_t width, std::uint32_t height,
					 const BridgeUtils::unique_gs_texture_t &targetTexture,
					 const BridgeUtils::unique_gs_texture_t &currentGrayscaleTexture,
					 const BridgeUtils::unique_gs_texture_t &lastGrayscaleTexture,
					 const BridgeUtils::unique_gs_texture_t &segmentationMaskTexture) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(targetTexture.get(), nullptr, GS_CS_SRGB);
		gs_technique_t *tech = techCalculateDifferenceWithMask;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, currentGrayscaleTexture.get());
				gs_effect_set_texture(textureImage1, lastGrayscaleTexture.get());
				gs_effect_set_texture(textureImage2, segmentationMaskTexture.get());

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void reduce(const std::vector<BridgeUtils::unique_gs_texture_t> &reductionPyramidTextures,
		    const BridgeUtils::unique_gs_texture_t &sourceTexture) const noexcept
	{
		RenderTargetGuard renderTargetGuard;
		TransformStateGuard transformStateGuard;

		gs_technique_t *tech = techReduce;

		gs_texture_t *currentSource = sourceTexture.get();

		for (const auto &currentTargetTexture : reductionPyramidTextures) {
			gs_texture_t *currentTarget = currentTargetTexture.get();
			const std::uint32_t targetWidth = gs_texture_get_width(currentTarget);
			const std::uint32_t targetHeight = gs_texture_get_height(currentTarget);

			gs_set_render_target_with_color_space(currentTarget, nullptr, GS_CS_SRGB);
			gs_set_viewport(0, 0, targetWidth, targetHeight);
			gs_ortho(0.0f, static_cast<float>(targetWidth), 0.0f, static_cast<float>(targetHeight), -100.0f,
				 100.0f);
			gs_matrix_identity();

			std::size_t passes = gs_technique_begin(tech);
			for (std::size_t i = 0; i < passes; i++) {
				if (gs_technique_begin_pass(tech, i)) {
					gs_effect_set_texture(textureImage, currentSource);

					gs_draw_sprite(nullptr, 0, targetWidth, targetHeight);

					gs_technique_end_pass(tech);
				}
			}
			gs_technique_end(tech);

			currentSource = currentTarget;
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
		std::size_t passesHorizontal = gs_technique_begin(techHorizontalBoxFilterR8KS17);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(techHorizontalBoxFilterR8KS17, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techHorizontalBoxFilterR8KS17);
			}
		}
		gs_technique_end(techHorizontalBoxFilterR8KS17);

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		std::size_t passesVertical = gs_technique_begin(techVerticalBoxFilterR8KS17);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(techVerticalBoxFilterR8KS17, i)) {
				gs_effect_set_texture(textureImage, temporaryTexture1);

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techVerticalBoxFilterR8KS17);
			}
		}
		gs_technique_end(techVerticalBoxFilterR8KS17);
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
		std::size_t passesHorizontal = gs_technique_begin(techHorizontalBoxFilterWithMulR8KS17);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(techHorizontalBoxFilterWithMulR8KS17, i)) {
				gs_effect_set_texture(textureImage, source1Texture);
				gs_effect_set_texture(textureImage1, source2Texture);

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techHorizontalBoxFilterWithMulR8KS17);
			}
		}
		gs_technique_end(techHorizontalBoxFilterWithMulR8KS17);

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		std::size_t passesVertical = gs_technique_begin(techVerticalBoxFilterR8KS17);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(techVerticalBoxFilterR8KS17, i)) {
				gs_effect_set_texture(textureImage, temporaryTexture1);

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techVerticalBoxFilterR8KS17);
			}
		}
		gs_technique_end(techVerticalBoxFilterR8KS17);
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
		std::size_t passesHorizontal = gs_technique_begin(techHorizontalBoxFilterWithSqR8KS17);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(techHorizontalBoxFilterWithSqR8KS17, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techHorizontalBoxFilterWithSqR8KS17);
			}
		}
		gs_technique_end(techHorizontalBoxFilterWithSqR8KS17);

		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		std::size_t passesVertical = gs_technique_begin(techVerticalBoxFilterR8KS17);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(techVerticalBoxFilterR8KS17, i)) {
				gs_effect_set_texture(textureImage, temporaryTexture1);

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techVerticalBoxFilterR8KS17);
			}
		}
		gs_technique_end(techVerticalBoxFilterR8KS17);
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
		std::size_t passesA = gs_technique_begin(techCalculateGuidedFilterA);
		for (std::size_t i = 0; i < passesA; i++) {
			if (gs_technique_begin_pass(techCalculateGuidedFilterA, i)) {
				gs_effect_set_texture(textureImage, sourceMeanGuideSqTexture);
				gs_effect_set_texture(textureImage1, sourceMeanGuideTexture);
				gs_effect_set_texture(textureImage2, sourceMeanGuideSourceTexture);
				gs_effect_set_texture(textureImage3, sourceMeanSourceTexture);
				gs_effect_set_float(floatEps, eps);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techCalculateGuidedFilterA);
			}
		}
		gs_technique_end(techCalculateGuidedFilterA);

		gs_set_render_target_with_color_space(targetBTexture, nullptr, GS_CS_SRGB);
		std::size_t passesB = gs_technique_begin(techCalculateGuidedFilterB);
		for (std::size_t i = 0; i < passesB; i++) {
			if (gs_technique_begin_pass(techCalculateGuidedFilterB, i)) {
				gs_effect_set_texture(textureImage, targetATexture);
				gs_effect_set_texture(textureImage1, sourceMeanSourceTexture);
				gs_effect_set_texture(textureImage2, sourceMeanGuideTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techCalculateGuidedFilterB);
			}
		}
		gs_technique_end(techCalculateGuidedFilterB);
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

		gs_technique_t *tech = techFinalizeGuidedFilter;
		gs_set_render_target_with_color_space(targetTexture, nullptr, GS_CS_SRGB);
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceGuideTexture);
				gs_effect_set_texture(textureImage1, sourceATexture);
				gs_effect_set_texture(textureImage2, sourceBTexture);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}
};

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
