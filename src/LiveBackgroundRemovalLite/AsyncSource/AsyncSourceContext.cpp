/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - AsyncSource Module
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

#include "AsyncSourceContext.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include <obs-module.h>

#include <KaitoTokyo/ObsBridgeUtils/GsUnique.hpp>
#include <KaitoTokyo/SelfieSegmenter/BoundingBox.hpp>
#include <KaitoTokyo/SelfieSegmenter/NcnnSelfieSegmenter.hpp>

#include <MainEffect.hpp>

extern "C" const unsigned char mediapipe_selfie_segmentation_landscape_int8_ncnn_bin[];
extern "C" const unsigned int mediapipe_selfie_segmentation_landscape_int8_ncnn_bin_len;
extern "C" const char mediapipe_selfie_segmentation_landscape_int8_ncnn_param_text[];

using namespace KaitoTokyo::Logger;
using namespace KaitoTokyo::ObsBridgeUtils;

namespace KaitoTokyo::LiveBackgroundRemovalLite::AsyncSource {

namespace {

inline uint32_t bit_ceil(uint32_t x)
{
	if (x == 0) {
		return 1;
	}
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

} // anonymous namespace

AsyncSourceContext::AsyncSourceContext(obs_data_t *settings, obs_source_t *source,
				       std::shared_ptr<Global::PluginConfig> pluginConfig,
				       std::shared_ptr<Global::GlobalContext> globalContext)
	: source_{source ? source : throw std::invalid_argument("SourceIsNullError(AsyncSourceContext)")},
	  pluginConfig_{pluginConfig ? std::move(pluginConfig)
				     : throw std::invalid_argument("PluginConfigIsNullError(AsyncSourceContext)")},
	  globalContext_{globalContext ? std::move(globalContext)
				       : throw std::invalid_argument("GlobalContextIsNullError(AsyncSourceContext)")},
	  logger_{globalContext_->getLogger() ? globalContext_->getLogger()
					      : throw std::invalid_argument("LoggerIsNullError(AsyncSourceContext)")},
	  mainEffect_(logger_, unique_obs_module_file("effects/main.effect")),
	  selfieSegmenterTaskQueue_(logger_, 1)
{
	obs_audio_info audioInfo{};
	if (obs_get_audio_info(&audioInfo)) {
		audioSamplesPerSec_ = audioInfo.samples_per_sec;
		audioSpeakers_ = audioInfo.speakers;
	}

	update(settings);
}

void AsyncSourceContext::shutdown() noexcept
{
	{
		std::lock_guard<std::mutex> lock(settingsMutex_);
		detachAudioSource();
		detachVideoSource();
	}

	selfieSegmenterTaskQueue_.shutdown();

	{
		std::lock_guard<std::mutex> lock(renderMutex_);
		clearRenderingResources();
	}
}

AsyncSourceContext::~AsyncSourceContext() noexcept {}

uint32_t AsyncSourceContext::getWidth() const noexcept
{
	std::lock_guard<std::mutex> lock(renderMutex_);
	return renderWidth_;
}

uint32_t AsyncSourceContext::getHeight() const noexcept
{
	std::lock_guard<std::mutex> lock(renderMutex_);
	return renderHeight_;
}

void AsyncSourceContext::getDefaults(obs_data_t *data)
{
	obs_data_set_default_string(data, "videoSourceName", "");
	obs_data_set_default_string(data, "audioSourceName", "");
	obs_data_set_default_int(data, "delayMs", 0);

	MainFilter::PluginProperty defaultProperty;
	obs_data_set_default_int(data, "filterLevel", static_cast<int>(defaultProperty.filterLevel));
	obs_data_set_default_double(data, "motionIntensityThresholdPowDb",
				    defaultProperty.motionIntensityThresholdPowDb);
	obs_data_set_default_double(data, "timeAveragedFilteringAlpha", defaultProperty.timeAveragedFilteringAlpha);
	obs_data_set_default_bool(data, "advancedSettings", false);
	obs_data_set_default_double(data, "guidedFilterEpsPowDb", defaultProperty.guidedFilterEpsPowDb);
	obs_data_set_default_int(data, "blurSize", defaultProperty.blurSize);
	obs_data_set_default_bool(data, "enableCenterFrame", defaultProperty.enableCenterFrame);
	obs_data_set_default_double(data, "maskGamma", defaultProperty.maskGamma);
	obs_data_set_default_double(data, "maskLowerBoundAmpDb", defaultProperty.maskLowerBoundAmpDb);
	obs_data_set_default_double(data, "maskUpperBoundMarginAmpDb", defaultProperty.maskUpperBoundMarginAmpDb);
}

obs_properties_t *AsyncSourceContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	// Video source selection
	obs_property_t *videoSourceProp = obs_properties_add_list(props, "videoSourceName",
								  obs_module_text("asyncSourceVideoSource"),
								  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(videoSourceProp, obs_module_text("asyncSourceNone"), "");
	obs_enum_sources(
		[](void *data, obs_source_t *src) -> bool {
			auto *prop = static_cast<obs_property_t *>(data);
			uint32_t flags = obs_source_get_output_flags(src);
			if (flags & OBS_SOURCE_VIDEO) {
				obs_property_list_add_string(prop, obs_source_get_name(src), obs_source_get_name(src));
			}
			return true;
		},
		videoSourceProp);

	// Audio source selection
	obs_property_t *audioSourceProp = obs_properties_add_list(props, "audioSourceName",
								  obs_module_text("asyncSourceAudioSource"),
								  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(audioSourceProp, obs_module_text("asyncSourceNone"), "");
	obs_enum_sources(
		[](void *data, obs_source_t *src) -> bool {
			auto *prop = static_cast<obs_property_t *>(data);
			uint32_t flags = obs_source_get_output_flags(src);
			if (flags & OBS_SOURCE_AUDIO) {
				obs_property_list_add_string(prop, obs_source_get_name(src), obs_source_get_name(src));
			}
			return true;
		},
		audioSourceProp);

	// Delay
	obs_properties_add_int(props, "delayMs", obs_module_text("asyncSourceDelayMs"), 0, 5000, 10);

	obs_property_t *pDesc = obs_properties_add_text(props, "separatorAfterDelay", "<hr />", OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	// Filter level
	pDesc = obs_properties_add_text(props, "filterLevelDescription", obs_module_text("filterLevelDescription"),
					OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	obs_property_t *propFilterLevel =
		obs_properties_add_list(props, "filterLevel", "", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelDefault"),
				  static_cast<int>(MainFilter::FilterLevel::Default));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelPassthrough"),
				  static_cast<int>(MainFilter::FilterLevel::Passthrough));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelSegmentation"),
				  static_cast<int>(MainFilter::FilterLevel::Segmentation));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelMotionIntensityThresholding"),
				  static_cast<int>(MainFilter::FilterLevel::MotionIntensityThresholding));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelGuidedFilter"),
				  static_cast<int>(MainFilter::FilterLevel::GuidedFilter));
	obs_property_list_add_int(propFilterLevel, obs_module_text("filterLevelTimeAveragedFilter"),
				  static_cast<int>(MainFilter::FilterLevel::TimeAveragedFilter));

	pDesc = obs_properties_add_text(props, "filterLevelDescription400",
					obs_module_text("filterLevelDescription400"), OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	pDesc = obs_properties_add_text(props, "filterLevelDescription500",
					obs_module_text("filterLevelDescription500"), OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	pDesc = obs_properties_add_text(props, "separatorAfterFilterLevel", "<hr />", OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	// Motion intensity threshold
	pDesc = obs_properties_add_text(props, "motionIntensityThresholdPowDbDescription",
					obs_module_text("motionIntensityThresholdPowDbDescription"), OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);
	obs_properties_add_float_slider(props, "motionIntensityThresholdPowDb", "", -100.0, 0.0, 0.1);

	pDesc = obs_properties_add_text(props, "separatorAfterMotionIntensityThreshold", "<hr />", OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	// Time-averaged filtering
	pDesc = obs_properties_add_text(props, "timeAveragedFilteringAlphaDescription",
					obs_module_text("timeAveragedFilteringAlphaDescription"), OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);
	obs_properties_add_float_slider(props, "timeAveragedFilteringAlpha", "", 0.0f, 1.0f, 0.01f);

	pDesc = obs_properties_add_text(props, "separatorAfterTimeAveragedFilteringAlpha", "<hr />", OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);

	// Blur size
	pDesc = obs_properties_add_text(props, "blurSizeDescription", obs_module_text("blurSizeDescription"),
					OBS_TEXT_INFO);
	obs_property_text_set_info_word_wrap(pDesc, false);
	obs_properties_add_int_slider(props, "blurSize", "", 0, 10, 1);

	// Center frame
	obs_properties_add_bool(props, "enableCenterFrame", obs_module_text("enableCenterFrame"));

	// Advanced settings group
	obs_properties_t *propsAdvancedSettings = obs_properties_create();
	obs_properties_add_group(props, "advancedSettings", obs_module_text("advancedSettings"), OBS_GROUP_CHECKABLE,
				 propsAdvancedSettings);

	obs_properties_add_float_slider(propsAdvancedSettings, "guidedFilterEpsPowDb",
					obs_module_text("guidedFilterEpsPowDb"), -60.0, -20.0, 0.1);

	obs_properties_add_float_slider(propsAdvancedSettings, "maskGamma", obs_module_text("maskGamma"), 0.5, 3.0,
					0.01);
	obs_properties_add_float_slider(propsAdvancedSettings, "maskLowerBoundAmpDb",
					obs_module_text("maskLowerBoundAmpDb"), -80.0, -10.0, 0.1);
	obs_properties_add_float_slider(propsAdvancedSettings, "maskUpperBoundMarginAmpDb",
					obs_module_text("maskUpperBoundMarginAmpDb"), -80.0, -10.0, 0.1);

	return props;
}

void AsyncSourceContext::update(obs_data_t *settings)
{
	std::string newVideoSourceName = obs_data_get_string(settings, "videoSourceName");
	std::string newAudioSourceName = obs_data_get_string(settings, "audioSourceName");
	int64_t newDelayMs = obs_data_get_int(settings, "delayMs");

	MainFilter::PluginProperty newPluginProperty = pluginProperty_;

	newPluginProperty.filterLevel = static_cast<MainFilter::FilterLevel>(obs_data_get_int(settings, "filterLevel"));
	newPluginProperty.motionIntensityThresholdPowDb =
		obs_data_get_double(settings, "motionIntensityThresholdPowDb");
	newPluginProperty.timeAveragedFilteringAlpha = obs_data_get_double(settings, "timeAveragedFilteringAlpha");

	bool advancedSettingsEnabled = obs_data_get_bool(settings, "advancedSettings");
	if (advancedSettingsEnabled) {
		newPluginProperty.guidedFilterEpsPowDb = obs_data_get_double(settings, "guidedFilterEpsPowDb");
		newPluginProperty.maskGamma = obs_data_get_double(settings, "maskGamma");
		newPluginProperty.maskLowerBoundAmpDb = obs_data_get_double(settings, "maskLowerBoundAmpDb");
		newPluginProperty.maskUpperBoundMarginAmpDb =
			obs_data_get_double(settings, "maskUpperBoundMarginAmpDb");
	}

	int newBlurSize = obs_data_get_int(settings, "blurSize");
	bool blurSizeChanged = (newPluginProperty.blurSize != newBlurSize);
	newPluginProperty.blurSize = newBlurSize;
	newPluginProperty.enableCenterFrame = obs_data_get_bool(settings, "enableCenterFrame");

	MainFilter::FilterLevel newFilterLevel = (newPluginProperty.filterLevel == MainFilter::FilterLevel::Default)
							 ? MainFilter::FilterLevel::TimeAveragedFilter
							 : newPluginProperty.filterLevel;

	float newMotionIntensityThreshold =
		static_cast<float>(std::pow(10.0, newPluginProperty.motionIntensityThresholdPowDb / 10.0));
	float newGuidedFilterEps = static_cast<float>(std::pow(10.0, newPluginProperty.guidedFilterEpsPowDb / 10.0));
	float newTimeAveragedFilteringAlpha = static_cast<float>(newPluginProperty.timeAveragedFilteringAlpha);
	float newMaskGamma = static_cast<float>(newPluginProperty.maskGamma);
	float newMaskLowerBound = static_cast<float>(std::pow(10.0, newPluginProperty.maskLowerBoundAmpDb / 20.0));
	float newMaskUpperBoundMargin =
		static_cast<float>(std::pow(10.0, newPluginProperty.maskUpperBoundMarginAmpDb / 20.0));

	filterLevel_.store(newFilterLevel, std::memory_order_relaxed);
	motionIntensityThreshold_.store(newMotionIntensityThreshold, std::memory_order_relaxed);
	guidedFilterEps_.store(newGuidedFilterEps, std::memory_order_relaxed);
	timeAveragedFilteringAlpha_.store(newTimeAveragedFilteringAlpha, std::memory_order_relaxed);
	maskGamma_.store(newMaskGamma, std::memory_order_relaxed);
	maskLowerBound_.store(newMaskLowerBound, std::memory_order_relaxed);
	maskUpperBoundMargin_.store(newMaskUpperBoundMargin, std::memory_order_relaxed);
	enableCenterFrame_.store(newPluginProperty.enableCenterFrame, std::memory_order_relaxed);

	{
		std::lock_guard<std::mutex> lock(settingsMutex_);

		if (newVideoSourceName != videoSourceName_) {
			detachVideoSource();
			videoSourceName_ = newVideoSourceName;
			if (!videoSourceName_.empty()) {
				attachVideoSource(videoSourceName_);
			}
		}

		if (newAudioSourceName != audioSourceName_) {
			detachAudioSource();
			audioSourceName_ = newAudioSourceName;
			if (!audioSourceName_.empty()) {
				attachAudioSource(audioSourceName_);
			}
		}

		delayNs_ = static_cast<uint64_t>(newDelayMs) * 1000000ULL;
	}

	pluginProperty_ = newPluginProperty;

	if (blurSizeChanged) {
		std::lock_guard<std::mutex> lock(renderMutex_);
		if (renderWidth_ > 0 && renderHeight_ > 0) {
			GraphicsContextGuard graphicsContextGuard;
			clearRenderingResources();
		}
	}

	shouldForceProcessFrame_.store(true, std::memory_order_relaxed);
}

void AsyncSourceContext::videoTick(float)
{
	obs_source_t *videoSource = nullptr;
	{
		std::lock_guard<std::mutex> lock(settingsMutex_);
		if (videoSourceWeak_) {
			videoSource = obs_weak_source_get_source(videoSourceWeak_);
		}
	}

	if (!videoSource) {
		outputBufferedFrames();
		outputBufferedAudio();
		return;
	}

	uint32_t width = obs_source_get_base_width(videoSource);
	uint32_t height = obs_source_get_base_height(videoSource);

	if (width == 0 || height == 0) {
		obs_source_release(videoSource);
		outputBufferedFrames();
		outputBufferedAudio();
		return;
	}

	const uint32_t minSize = 2 * static_cast<uint32_t>(pluginProperty_.subsamplingRate);
	if (width < minSize || height < minSize) {
		obs_source_release(videoSource);
		outputBufferedFrames();
		outputBufferedAudio();
		return;
	}

	processVideoFrame(videoSource, width, height);
	obs_source_release(videoSource);

	outputBufferedFrames();
	outputBufferedAudio();
}

void AsyncSourceContext::processVideoFrame(obs_source_t *videoSource, uint32_t width, uint32_t height)
{
	bool resourcesReady = false;
	{
		std::lock_guard<std::mutex> lock(renderMutex_);
		resourcesReady = ensureRenderingResources(width, height);
	}

	if (!resourcesReady) {
		return;
	}

	const MainFilter::FilterLevel filterLevel = filterLevel_.load(std::memory_order_relaxed);
	const float motionIntensityThreshold = motionIntensityThreshold_.load(std::memory_order_relaxed);
	const float guidedFilterEps = guidedFilterEps_.load(std::memory_order_relaxed);
	const float maskGamma = maskGamma_.load(std::memory_order_relaxed);
	const float maskLowerBound = maskLowerBound_.load(std::memory_order_relaxed);
	const float maskUpperBoundMargin = maskUpperBoundMargin_.load(std::memory_order_relaxed);
	const float timeAveragedFilteringAlpha = timeAveragedFilteringAlpha_.load(std::memory_order_relaxed);
	const bool enableCenterFrame = enableCenterFrame_.load(std::memory_order_relaxed);
	const bool forceProcessingFrame = shouldForceProcessFrame_.exchange(false, std::memory_order_acquire);

	GraphicsContextGuard graphicsContextGuard;

	// Step 1: Render source into bgrxSource_
	if (filterLevel >= MainFilter::FilterLevel::Passthrough) {
		mainEffect_.renderSourceToTexture(bgrxSource_, videoSource);
	}

	// Step 2: Prepare segmenter input
	if (filterLevel >= MainFilter::FilterLevel::Segmentation) {
		const double targetW = static_cast<double>(selfieSegmenter_->getWidth());
		const double targetH = static_cast<double>(selfieSegmenter_->getHeight());
		constexpr vec4 blackColor = {0.0f, 0.0f, 0.0f, 1.0f};

		double fitScaleX = targetW / static_cast<double>(segmenterRoi_.width);
		double fitScaleY = targetH / static_cast<double>(segmenterRoi_.height);
		double scale = std::min(fitScaleX, fitScaleY);

		uint32_t scaledW = static_cast<uint32_t>(std::round(renderWidth_ * scale));
		uint32_t scaledH = static_cast<uint32_t>(std::round(renderHeight_ * scale));

		double roiCenterX = segmenterRoi_.x + segmenterRoi_.width / 2.0;
		double roiCenterY = segmenterRoi_.y + segmenterRoi_.height / 2.0;

		float x = static_cast<float>((targetW / 2.0) - (roiCenterX * scale));
		float y = static_cast<float>((targetH / 2.0) - (roiCenterY * scale));

		mainEffect_.drawRoi(bgrxSegmenterInput_, bgrxSource_, &blackColor, scaledW, scaledH, x, y);
		bgrxSegmenterInputReader_->stage(bgrxSegmenterInput_);
	}

	// Step 3: Motion intensity
	float motionIntensity = (filterLevel < MainFilter::FilterLevel::MotionIntensityThresholding) ? 1.0f : 0.0f;

	if (filterLevel >= MainFilter::FilterLevel::MotionIntensityThresholding) {
		mainEffect_.convertToLuma(r32fLuma_, bgrxSource_);

		const auto &lastSubLuma = r32fSubLumas_[currentSubLumaIndex_];
		const auto &currentSubLuma = r32fSubLumas_[1 - currentSubLumaIndex_];
		mainEffect_.resampleByNearestR8(currentSubLuma, r32fLuma_);

		mainEffect_.calculateSquaredMotion(r32fSubPaddedSquaredMotion_, currentSubLuma, lastSubLuma);

		currentSubLumaIndex_ = 1 - currentSubLumaIndex_;

		mainEffect_.reduce(r32fMeanSquaredMotionReductionPyramid_, r32fSubPaddedSquaredMotion_);
		r32fReducedMeanSquaredMotionReader_->stage(r32fMeanSquaredMotionReductionPyramid_.back());
	}

	// Step 4: Blur
	if (filterLevel >= MainFilter::FilterLevel::Segmentation && pluginProperty_.blurSize > 0) {
		gs_copy_texture(bgrxDualKawaseBlurReductionPyramid_[0].get(), bgrxSource_.get());
		mainEffect_.dualKawaseBlur(bgrxDualKawaseBlurReductionPyramid_, pluginProperty_.blurSize);
	}

	// Step 5: Sync texture reads
	if (filterLevel >= MainFilter::FilterLevel::Segmentation) {
		try {
			bgrxSegmenterInputReader_->sync();
		} catch (const std::exception &e) {
			logger_->error("TextureSyncError", {{"message", e.what()}});
		}
	}

	if (filterLevel >= MainFilter::FilterLevel::MotionIntensityThresholding) {
		r32fReducedMeanSquaredMotionReader_->sync();
		const float *meanSquaredMotionPtr =
			reinterpret_cast<const float *>(r32fReducedMeanSquaredMotionReader_->getBuffer().data());
		motionIntensity = *meanSquaredMotionPtr / (subWidth_ * subHeight_);
	}

	const bool isCurrentMotionIntense = (motionIntensity >= motionIntensityThreshold);

	// Step 6: Guided filter
	if (filterLevel >= MainFilter::FilterLevel::GuidedFilter) {
		const auto &currentSubLuma = r32fSubLumas_[currentSubLumaIndex_];
		mainEffect_.resampleByNearestR8(r32fSubGFSource_, r8SegmentationMask_);

		mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanGuide_, currentSubLuma, r32fSubGFIntermediate_);
		mainEffect_.applyBoxFilterR8KS17(r32fSubGFMeanSource_, r32fSubGFSource_, r32fSubGFIntermediate_);

		mainEffect_.applyBoxFilterWithMulR8KS17(r32fSubGFMeanGuideSource_, currentSubLuma, r32fSubGFSource_,
							r32fSubGFIntermediate_);
		mainEffect_.applyBoxFilterWithSqR8KS17(r32fSubGFMeanGuideSq_, currentSubLuma, r32fSubGFIntermediate_);

		mainEffect_.calculateGuidedFilterAAndB(r32fSubGFA_, r32fSubGFB_, r32fSubGFMeanGuideSq_,
						       r32fSubGFMeanGuide_, r32fSubGFMeanGuideSource_,
						       r32fSubGFMeanSource_, guidedFilterEps);

		mainEffect_.finalizeGuidedFilter(r8GuidedFilterResult_, r32fLuma_, r32fSubGFA_, r32fSubGFB_);
	}

	// Step 7: Time-averaged filter
	if (filterLevel >= MainFilter::FilterLevel::TimeAveragedFilter) {
		std::size_t nextIndex = 1 - currentTimeAveragedMaskIndex_;
		mainEffect_.timeAveragedFiltering(r8TimeAveragedMasks_[nextIndex],
						  r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_],
						  r8GuidedFilterResult_, timeAveragedFilteringAlpha);
		currentTimeAveragedMaskIndex_ = nextIndex;
	}

	// Step 8: Run segmentation
	if (filterLevel >= MainFilter::FilterLevel::Segmentation && (isCurrentMotionIntense || forceProcessingFrame)) {
		auto &bgrxSegmenterInputReaderBuffer = bgrxSegmenterInputReader_->getBuffer();
		auto segmenterInputBuffer = segmenterMemoryPool_->acquire();
		if (!segmenterInputBuffer) {
			logger_->error("MemoryBlockAcquisitionError");
			return;
		}

		std::copy(bgrxSegmenterInputReaderBuffer.begin(), bgrxSegmenterInputReaderBuffer.end(),
			  segmenterInputBuffer->begin());

		selfieSegmenter_->process(segmenterInputBuffer.get()->data());

		if (enableCenterFrame) {
			SelfieSegmenter::BoundingBox bb;
			bb.calculateBoundingBoxFrom256x144(selfieSegmenter_->getMask(), 200);

			const uint64_t bbX = static_cast<uint64_t>(bb.x);
			const uint64_t bbY = static_cast<uint64_t>(bb.y);
			const uint64_t bbW = static_cast<uint64_t>(bb.width);
			const uint64_t bbH = static_cast<uint64_t>(bb.height);

			const uint64_t roiX = static_cast<uint64_t>(segmenterRoi_.x);
			const uint64_t roiY = static_cast<uint64_t>(segmenterRoi_.y);
			const uint64_t roiW = static_cast<uint64_t>(segmenterRoi_.width);
			const uint64_t roiH = static_cast<uint64_t>(segmenterRoi_.height);

			const uint64_t baseW = static_cast<uint64_t>(selfieSegmenter_->getWidth());
			const uint64_t baseH = static_cast<uint64_t>(selfieSegmenter_->getHeight());

			if (baseW > 0 && baseH > 0) {
				sourceRoi_.x = static_cast<uint32_t>(((bbX * roiW) + (baseW / 2)) / baseW + roiX);
				sourceRoi_.y = static_cast<uint32_t>(((bbY * roiH) + (baseH / 2)) / baseH + roiY);
				sourceRoi_.width = static_cast<uint32_t>(((bbW * roiW) + (baseW / 2)) / baseW);
				sourceRoi_.height = static_cast<uint32_t>(((bbH * roiH) + (baseH / 2)) / baseH);
			}
		}

		const uint8_t *segmentationMaskData =
			selfieSegmenter_->getMask() + (maskRoi_.y * selfieSegmenter_->getWidth() + maskRoi_.x);

		gs_texture_set_image(r8SegmentationMask_.get(), segmentationMaskData,
				     static_cast<uint32_t>(selfieSegmenter_->getWidth()), 0);
	}

	// Step 9: Render to output texture
	{
		MainFilter::TextureRenderGuard outputRenderGuard(bgrxOutput_);

		if (enableCenterFrame) {
			gs_matrix_push();

			float scaleFactor;
			if (sourceRoi_.width > 0 && sourceRoi_.height > 0) {
				float widthScale =
					static_cast<float>(renderWidth_) / static_cast<float>(sourceRoi_.width);
				float heightScale =
					static_cast<float>(renderHeight_) / static_cast<float>(sourceRoi_.height);
				scaleFactor = std::min(widthScale, heightScale);
			} else {
				scaleFactor = 1.0f;
			}

			float displayW = static_cast<float>(sourceRoi_.width) * scaleFactor;
			float displayH = static_cast<float>(sourceRoi_.height) * scaleFactor;

			float offsetX = (static_cast<float>(renderWidth_) - displayW) / 2.0f;
			float offsetY = (static_cast<float>(renderHeight_) - displayH);

			vec3 vecDest{offsetX, offsetY, 0.0f};
			gs_matrix_translate(&vecDest);

			vec3 vecScale{scaleFactor, scaleFactor, 1.0f};
			gs_matrix_scale(&vecScale);

			vec3 vecSource{-static_cast<float>(sourceRoi_.x), -static_cast<float>(sourceRoi_.y), 0.0f};
			gs_matrix_translate(&vecSource);
		}

		if (filterLevel == MainFilter::FilterLevel::Passthrough) {
			mainEffect_.directDraw(bgrxSource_);
		} else if (filterLevel == MainFilter::FilterLevel::Segmentation ||
			   filterLevel == MainFilter::FilterLevel::MotionIntensityThresholding) {
			if (pluginProperty_.blurSize > 0) {
				mainEffect_.directDrawWithBlurredBackground(bgrxSource_, r8SegmentationMask_,
									    bgrxDualKawaseBlurReductionPyramid_[0]);
			} else {
				mainEffect_.directDrawWithMask(bgrxSource_, r8SegmentationMask_);
			}
		} else if (filterLevel == MainFilter::FilterLevel::GuidedFilter) {
			if (pluginProperty_.blurSize > 0) {
				mainEffect_.directDrawWithRefinedBlurredBackground(
					bgrxSource_, r8GuidedFilterResult_, maskGamma, maskLowerBound,
					maskUpperBoundMargin, bgrxDualKawaseBlurReductionPyramid_[0]);
			} else {
				mainEffect_.directDrawWithRefinedMask(bgrxSource_, r8GuidedFilterResult_, maskGamma,
								      maskLowerBound, maskUpperBoundMargin);
			}
		} else if (filterLevel == MainFilter::FilterLevel::TimeAveragedFilter) {
			if (pluginProperty_.blurSize > 0) {
				mainEffect_.directDrawWithRefinedBlurredBackground(
					bgrxSource_, r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_], maskGamma,
					maskLowerBound, maskUpperBoundMargin, bgrxDualKawaseBlurReductionPyramid_[0]);
			} else {
				mainEffect_.directDrawWithRefinedMask(
					bgrxSource_, r8TimeAveragedMasks_[currentTimeAveragedMaskIndex_], maskGamma,
					maskLowerBound, maskUpperBoundMargin);
			}
		} else {
			// Draw nothing
		}

		if (enableCenterFrame) {
			gs_matrix_pop();
		}
	}

	// Step 10: Stage output for readback
	bgrxOutputReader_->stage(bgrxOutput_);

	// Step 11: Sync output (reads data staged in previous tick)
	try {
		bgrxOutputReader_->sync();
	} catch (const std::exception &e) {
		logger_->error("OutputTextureSyncError", {{"message", e.what()}});
		return;
	}

	GsUnique::drain();

	// Step 12: Copy to CPU buffer and enqueue for delayed output
	const auto &outputBuffer = bgrxOutputReader_->getBuffer();

	uint64_t captureTimestamp = os_gettime_ns();
	uint64_t delayNs = delayNs_;

	BufferedVideoFrame frame;
	frame.data = outputBuffer;
	frame.width = renderWidth_;
	frame.height = renderHeight_;
	frame.linesize = renderWidth_ * 4; // 4 bytes per BGRX pixel
	frame.outputTimestamp = captureTimestamp + delayNs;

	{
		std::lock_guard<std::mutex> lock(videoQueueMutex_);
		videoQueue_.push_back(std::move(frame));
	}
}

void AsyncSourceContext::outputBufferedFrames()
{
	uint64_t now = os_gettime_ns();

	std::deque<BufferedVideoFrame> toOutput;
	{
		std::lock_guard<std::mutex> lock(videoQueueMutex_);
		while (!videoQueue_.empty() && videoQueue_.front().outputTimestamp <= now) {
			toOutput.push_back(std::move(videoQueue_.front()));
			videoQueue_.pop_front();
		}
	}

	for (auto &frame : toOutput) {
		obs_source_frame2 obsFrame{};
		obsFrame.width = frame.width;
		obsFrame.height = frame.height;
		obsFrame.format = VIDEO_FORMAT_BGRX;
		obsFrame.timestamp = frame.outputTimestamp;
		obsFrame.data[0] = frame.data.data();
		obsFrame.linesize[0] = frame.linesize;
		obsFrame.range = VIDEO_RANGE_DEFAULT;
		obs_source_output_video2(source_, &obsFrame);
	}
}

void AsyncSourceContext::outputBufferedAudio()
{
	uint64_t now = os_gettime_ns();

	std::deque<BufferedAudioFrame> toOutput;
	{
		std::lock_guard<std::mutex> lock(audioQueueMutex_);
		while (!audioQueue_.empty() && audioQueue_.front().outputTimestamp <= now) {
			toOutput.push_back(std::move(audioQueue_.front()));
			audioQueue_.pop_front();
		}
	}

	for (auto &frame : toOutput) {
		obs_source_audio audioOut{};
		audioOut.frames = frame.frames;
		audioOut.format = AUDIO_FORMAT_FLOAT_PLANAR;
		audioOut.samples_per_sec = audioSamplesPerSec_;
		audioOut.speakers = audioSpeakers_;
		audioOut.timestamp = frame.outputTimestamp;

		for (int i = 0; i < MAX_AV_PLANES; ++i) {
			if (!frame.data[i].empty()) {
				audioOut.data[i] = frame.data[i].data();
			}
		}

		obs_source_output_audio(source_, &audioOut);
	}
}

void AsyncSourceContext::audioCaptureCallbackStatic(void *data, obs_source_t *, const struct audio_data *audioData,
						    bool muted)
{
	if (muted || !data || !audioData) {
		return;
	}
	auto *self = static_cast<AsyncSourceContext *>(data);
	self->audioCaptureCallback(audioData);
}

void AsyncSourceContext::audioCaptureCallback(const struct audio_data *audioData)
{
	if (!audioData || audioData->frames == 0) {
		return;
	}

	uint64_t delayNs;
	{
		std::lock_guard<std::mutex> lock(settingsMutex_);
		delayNs = delayNs_;
	}

	// Audio from obs_source_add_audio_capture_callback is in AUDIO_FORMAT_FLOAT_PLANAR
	// (4 bytes per float sample) at the global OBS audio sample rate.
	constexpr uint32_t bytesPerSample = 4;
	uint32_t bytesPerPlane = audioData->frames * bytesPerSample;

	int numChannels = get_audio_channels(audioSpeakers_);

	BufferedAudioFrame frame;
	frame.frames = audioData->frames;
	frame.outputTimestamp = audioData->timestamp + delayNs;

	for (int i = 0; i < numChannels && i < MAX_AV_PLANES; ++i) {
		if (audioData->data[i]) {
			frame.data[i].resize(bytesPerPlane);
			std::memcpy(frame.data[i].data(), audioData->data[i], bytesPerPlane);
		}
	}

	{
		std::lock_guard<std::mutex> lock(audioQueueMutex_);
		audioQueue_.push_back(std::move(frame));
	}
}

void AsyncSourceContext::attachVideoSource(const std::string &name)
{
	obs_source_t *source = obs_get_source_by_name(name.c_str());
	if (!source) {
		logger_->debug("VideoSourceNotFound", {{"name", name}});
		return;
	}

	videoSourceWeak_ = obs_source_get_weak_source(source);
	obs_source_release(source);

	{
		std::lock_guard<std::mutex> lock(renderMutex_);
		clearRenderingResources();
	}
	shouldForceProcessFrame_.store(true, std::memory_order_relaxed);
}

void AsyncSourceContext::detachVideoSource() noexcept
{
	if (videoSourceWeak_) {
		obs_weak_source_release(videoSourceWeak_);
		videoSourceWeak_ = nullptr;
	}
}

void AsyncSourceContext::attachAudioSource(const std::string &name)
{
	obs_source_t *source = obs_get_source_by_name(name.c_str());
	if (!source) {
		logger_->debug("AudioSourceNotFound", {{"name", name}});
		return;
	}

	audioSourceWeak_ = obs_source_get_weak_source(source);
	obs_source_add_audio_capture_callback(source, audioCaptureCallbackStatic, this);
	obs_source_release(source);
}

void AsyncSourceContext::detachAudioSource() noexcept
{
	if (audioSourceWeak_) {
		obs_source_t *source = obs_weak_source_get_source(audioSourceWeak_);
		if (source) {
			obs_source_remove_audio_capture_callback(source, audioCaptureCallbackStatic, this);
			obs_source_release(source);
		}
		obs_weak_source_release(audioSourceWeak_);
		audioSourceWeak_ = nullptr;
	}
}

ObsBridgeUtils::unique_gs_texture_t AsyncSourceContext::makeTexture(uint32_t width, uint32_t height,
								    gs_color_format colorFormat,
								    uint32_t flags) const noexcept
{
	ObsBridgeUtils::unique_gs_texture_t texture =
		ObsBridgeUtils::make_unique_gs_texture(width, height, colorFormat, 1, nullptr, flags);

	if ((flags & GS_RENDER_TARGET) == GS_RENDER_TARGET) {
		MainFilter::TextureRenderGuard renderTargetGuard(texture);
		vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
		gs_clear(GS_CLEAR_COLOR, &clearColor, 0.0f, 0);
	} else if ((flags & GS_DYNAMIC) == GS_DYNAMIC) {
		std::vector<uint8_t> zeroData(
			static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
				static_cast<std::size_t>(
					ObsBridgeUtils::AsyncTextureReader::getBytesPerPixel(colorFormat)),
			0);
		gs_texture_set_image(texture.get(), zeroData.data(),
				     width * ObsBridgeUtils::AsyncTextureReader::getBytesPerPixel(colorFormat), 0);
	}
	return texture;
}

std::vector<ObsBridgeUtils::unique_gs_texture_t> AsyncSourceContext::createReductionPyramid(uint32_t width,
											    uint32_t height) const
{
	std::vector<ObsBridgeUtils::unique_gs_texture_t> pyramid;
	uint32_t currentWidth = width;
	uint32_t currentHeight = height;

	while (currentWidth > 1 || currentHeight > 1) {
		currentWidth = std::max(1u, (currentWidth + 1) / 2);
		currentHeight = std::max(1u, (currentHeight + 1) / 2);
		pyramid.push_back(makeTexture(currentWidth, currentHeight, GS_R32F, GS_RENDER_TARGET));
	}

	return pyramid;
}

std::vector<ObsBridgeUtils::unique_gs_texture_t>
AsyncSourceContext::createDualKawasePyramid(uint32_t width, uint32_t height, int blurSize) const
{
	std::vector<ObsBridgeUtils::unique_gs_texture_t> pyramid;
	pyramid.push_back(makeTexture(width, height, GS_BGRX, GS_RENDER_TARGET));

	uint32_t currentWidth = width;
	uint32_t currentHeight = height;

	for (int i = 0; i < blurSize; ++i) {
		currentWidth = std::max(1u, (currentWidth + 1) / 2);
		currentHeight = std::max(1u, (currentHeight + 1) / 2);
		pyramid.push_back(makeTexture(currentWidth, currentHeight, GS_BGRX, GS_RENDER_TARGET));
	}

	return pyramid;
}

AsyncSourceRegion AsyncSourceContext::getMaskRoiPosition(uint32_t regionWidth, uint32_t regionHeight) const noexcept
{
	uint32_t segW = static_cast<uint32_t>(selfieSegmenter_->getWidth());
	uint32_t segH = static_cast<uint32_t>(selfieSegmenter_->getHeight());

	double widthScale = static_cast<double>(segW) / static_cast<double>(regionWidth);
	double heightScale = static_cast<double>(segH) / static_cast<double>(regionHeight);
	double scale = std::min(widthScale, heightScale);

	uint32_t scaledW = static_cast<uint32_t>(std::round(regionWidth * scale));
	uint32_t scaledH = static_cast<uint32_t>(std::round(regionHeight * scale));
	uint32_t offsetX = (segW - scaledW) / 2;
	uint32_t offsetY = (segH - scaledH) / 2;

	return {offsetX, offsetY, scaledW, scaledH};
}

bool AsyncSourceContext::ensureRenderingResources(uint32_t width, uint32_t height)
{
	if (renderWidth_ == width && renderHeight_ == height && bgrxSource_) {
		return true;
	}

	clearRenderingResources();

	uint32_t subsamplingRate = static_cast<uint32_t>(pluginProperty_.subsamplingRate);
	uint32_t newSubW = (width / subsamplingRate >= 2) ? (width / subsamplingRate) & ~1u : 0;
	uint32_t newSubH = (height / subsamplingRate >= 2) ? (height / subsamplingRate) & ~1u : 0;

	if (newSubW == 0 || newSubH == 0) {
		logger_->debug("SourceTooSmallForSubsampling");
		return false;
	}

	GraphicsContextGuard graphicsContextGuard;

	subWidth_ = newSubW;
	subHeight_ = newSubH;
	subPaddedWidth_ = bit_ceil(newSubW);
	subPaddedHeight_ = bit_ceil(newSubH);

	selfieSegmenter_ = std::make_unique<SelfieSegmenter::NcnnSelfieSegmenter>(
		mediapipe_selfie_segmentation_landscape_int8_ncnn_param_text,
		static_cast<int>(mediapipe_selfie_segmentation_landscape_int8_ncnn_bin_len),
		mediapipe_selfie_segmentation_landscape_int8_ncnn_bin, pluginProperty_.numThreads);

	segmenterMemoryPool_ = Memory::MemoryBlockPool::create(logger_, selfieSegmenter_->getPixelCount() * 4);

	maskRoi_ = getMaskRoiPosition(width, height);
	segmenterRoi_ = {0, 0, width, height};
	sourceRoi_ = {0, 0, width, height};

	bgrxSource_ = makeTexture(width, height, GS_BGRX, GS_RENDER_TARGET);
	r32fLuma_ = makeTexture(width, height, GS_R32F, GS_RENDER_TARGET);
	r32fSubLumas_[0] = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubLumas_[1] = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	currentSubLumaIndex_ = 0;

	r32fSubPaddedSquaredMotion_ = makeTexture(subPaddedWidth_, subPaddedHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fMeanSquaredMotionReductionPyramid_ = createReductionPyramid(subPaddedWidth_, subPaddedHeight_);
	r32fReducedMeanSquaredMotionReader_ = std::make_unique<ObsBridgeUtils::AsyncTextureReader>(1, 1, GS_R32F);

	uint32_t segW = static_cast<uint32_t>(selfieSegmenter_->getWidth());
	uint32_t segH = static_cast<uint32_t>(selfieSegmenter_->getHeight());
	bgrxSegmenterInput_ = makeTexture(segW, segH, GS_BGRX, GS_RENDER_TARGET);
	bgrxSegmenterInputReader_ = std::make_unique<ObsBridgeUtils::AsyncTextureReader>(segW, segH, GS_BGRX);
	segmenterInputBuffer_.resize(selfieSegmenter_->getPixelCount() * 4);

	r8SegmentationMask_ = makeTexture(maskRoi_.width, maskRoi_.height, GS_R8, GS_DYNAMIC);

	r32fSubGFIntermediate_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFSource_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFMeanGuide_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFMeanSource_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFMeanGuideSource_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFMeanGuideSq_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFA_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r32fSubGFB_ = makeTexture(subWidth_, subHeight_, GS_R32F, GS_RENDER_TARGET);
	r8GuidedFilterResult_ = makeTexture(width, height, GS_R8, GS_RENDER_TARGET);

	r8TimeAveragedMasks_[0] = makeTexture(width, height, GS_R8, GS_RENDER_TARGET);
	r8TimeAveragedMasks_[1] = makeTexture(width, height, GS_R8, GS_RENDER_TARGET);
	currentTimeAveragedMaskIndex_ = 0;

	bgrxDualKawaseBlurReductionPyramid_ = createDualKawasePyramid(width, height, pluginProperty_.blurSize);

	bgrxOutput_ = makeTexture(width, height, GS_BGRX, GS_RENDER_TARGET);
	bgrxOutputReader_ = std::make_unique<ObsBridgeUtils::AsyncTextureReader>(width, height, GS_BGRX);

	renderWidth_ = width;
	renderHeight_ = height;

	shouldForceProcessFrame_.store(true, std::memory_order_relaxed);

	return true;
}

void AsyncSourceContext::clearRenderingResources() noexcept
{
	renderWidth_ = 0;
	renderHeight_ = 0;
	subWidth_ = 0;
	subHeight_ = 0;
	subPaddedWidth_ = 0;
	subPaddedHeight_ = 0;

	selfieSegmenter_.reset();
	segmenterMemoryPool_.reset();

	bgrxSource_.reset();
	r32fLuma_.reset();
	r32fSubLumas_[0].reset();
	r32fSubLumas_[1].reset();

	r32fSubPaddedSquaredMotion_.reset();
	r32fMeanSquaredMotionReductionPyramid_.clear();
	r32fReducedMeanSquaredMotionReader_.reset();

	bgrxSegmenterInput_.reset();
	bgrxSegmenterInputReader_.reset();
	segmenterInputBuffer_.clear();

	r8SegmentationMask_.reset();

	r32fSubGFIntermediate_.reset();
	r32fSubGFSource_.reset();
	r32fSubGFMeanGuide_.reset();
	r32fSubGFMeanSource_.reset();
	r32fSubGFMeanGuideSource_.reset();
	r32fSubGFMeanGuideSq_.reset();
	r32fSubGFA_.reset();
	r32fSubGFB_.reset();
	r8GuidedFilterResult_.reset();

	r8TimeAveragedMasks_[0].reset();
	r8TimeAveragedMasks_[1].reset();

	bgrxDualKawaseBlurReductionPyramid_.clear();

	bgrxOutput_.reset();
	bgrxOutputReader_.reset();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::AsyncSource
