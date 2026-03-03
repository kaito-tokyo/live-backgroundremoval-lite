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

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <obs.h>
#include <obs-module.h>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/Memory/MemoryBlockPool.hpp>
#include <KaitoTokyo/ObsBridgeUtils/AsyncTextureReader.hpp>
#include <KaitoTokyo/ObsBridgeUtils/GsUnique.hpp>
#include <KaitoTokyo/SelfieSegmenter/ISelfieSegmenter.hpp>
#include <KaitoTokyo/TaskQueue/ThrottledTaskQueue.hpp>

#include <GlobalContext.hpp>
#include <PluginConfig.hpp>

#include <MainEffect.hpp>
#include <PluginProperty.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::AsyncSource {

struct AsyncSourceRegion {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};

struct BufferedAudioFrame {
	std::array<std::vector<uint8_t>, MAX_AV_PLANES> data;
	uint32_t frames;
	uint64_t outputTimestamp;
};

struct BufferedVideoFrame {
	std::vector<uint8_t> data;
	uint32_t width;
	uint32_t height;
	uint32_t linesize;
	uint64_t outputTimestamp;
};

class AsyncSourceContext : public std::enable_shared_from_this<AsyncSourceContext> {
public:
	AsyncSourceContext(obs_data_t *settings, obs_source_t *source,
			   std::shared_ptr<Global::PluginConfig> pluginConfig,
			   std::shared_ptr<Global::GlobalContext> globalContext);

	void shutdown() noexcept;
	~AsyncSourceContext() noexcept;

	AsyncSourceContext(const AsyncSourceContext &) = delete;
	AsyncSourceContext &operator=(const AsyncSourceContext &) = delete;
	AsyncSourceContext(AsyncSourceContext &&) = delete;
	AsyncSourceContext &operator=(AsyncSourceContext &&) = delete;

	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;

	static void getDefaults(obs_data_t *data);
	obs_properties_t *getProperties();
	void update(obs_data_t *settings);

	void videoTick(float seconds);

	const std::shared_ptr<const Logger::ILogger> getLogger() const noexcept { return logger_; }

	static void audioCaptureCallbackStatic(void *data, obs_source_t *source, const struct audio_data *audioData,
					       bool muted);

private:
	void audioCaptureCallback(const struct audio_data *audioData);

	void attachVideoSource(const std::string &name);
	void detachVideoSource() noexcept;
	void attachAudioSource(const std::string &name);
	void detachAudioSource() noexcept;

	bool ensureRenderingResources(uint32_t width, uint32_t height);
	void clearRenderingResources() noexcept;

	[[nodiscard]]
	ObsBridgeUtils::unique_gs_texture_t makeTexture(uint32_t width, uint32_t height, gs_color_format colorFormat,
							uint32_t flags) const noexcept;

	[[nodiscard]]
	std::vector<ObsBridgeUtils::unique_gs_texture_t> createReductionPyramid(uint32_t width, uint32_t height) const;

	[[nodiscard]]
	std::vector<ObsBridgeUtils::unique_gs_texture_t> createDualKawasePyramid(uint32_t width, uint32_t height,
										 int blurSize) const;

	[[nodiscard]]
	AsyncSourceRegion getMaskRoiPosition(uint32_t regionWidth, uint32_t regionHeight) const noexcept;

	void processVideoFrame(obs_source_t *videoSource, uint32_t width, uint32_t height);
	void outputBufferedFrames();
	void outputBufferedAudio();

	obs_source_t *const source_;
	const std::shared_ptr<Global::PluginConfig> pluginConfig_;
	const std::shared_ptr<Global::GlobalContext> globalContext_;
	const std::shared_ptr<const Logger::ILogger> logger_;

	const MainFilter::MainEffect mainEffect_;
	TaskQueue::ThrottledTaskQueue selfieSegmenterTaskQueue_;
	MainFilter::PluginProperty pluginProperty_;

	mutable std::mutex settingsMutex_;
	std::string videoSourceName_;
	std::string audioSourceName_;
	uint64_t delayNs_ = 0;

	obs_weak_source_t *videoSourceWeak_ = nullptr;
	obs_weak_source_t *audioSourceWeak_ = nullptr;

	// Rendering resources (require graphics context to create/destroy)
	mutable std::mutex renderMutex_;
	uint32_t renderWidth_ = 0;
	uint32_t renderHeight_ = 0;

	uint32_t subWidth_ = 0;
	uint32_t subHeight_ = 0;
	uint32_t subPaddedWidth_ = 0;
	uint32_t subPaddedHeight_ = 0;

	std::unique_ptr<SelfieSegmenter::ISelfieSegmenter> selfieSegmenter_;
	std::shared_ptr<Memory::MemoryBlockPool> segmenterMemoryPool_;

	ObsBridgeUtils::unique_gs_texture_t bgrxSource_;
	ObsBridgeUtils::unique_gs_texture_t r32fLuma_;
	std::array<ObsBridgeUtils::unique_gs_texture_t, 2> r32fSubLumas_;
	std::size_t currentSubLumaIndex_ = 0;

	ObsBridgeUtils::unique_gs_texture_t r32fSubPaddedSquaredMotion_;
	std::vector<ObsBridgeUtils::unique_gs_texture_t> r32fMeanSquaredMotionReductionPyramid_;
	std::unique_ptr<ObsBridgeUtils::AsyncTextureReader> r32fReducedMeanSquaredMotionReader_;

	AsyncSourceRegion segmenterRoi_{};
	AsyncSourceRegion sourceRoi_{};
	AsyncSourceRegion maskRoi_{};

	ObsBridgeUtils::unique_gs_texture_t bgrxSegmenterInput_;
	std::unique_ptr<ObsBridgeUtils::AsyncTextureReader> bgrxSegmenterInputReader_;
	std::vector<uint8_t> segmenterInputBuffer_;

	ObsBridgeUtils::unique_gs_texture_t r8SegmentationMask_;

	ObsBridgeUtils::unique_gs_texture_t r32fSubGFIntermediate_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFSource_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFMeanGuide_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFMeanSource_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSource_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFMeanGuideSq_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFA_;
	ObsBridgeUtils::unique_gs_texture_t r32fSubGFB_;
	ObsBridgeUtils::unique_gs_texture_t r8GuidedFilterResult_;

	std::array<ObsBridgeUtils::unique_gs_texture_t, 2> r8TimeAveragedMasks_;
	std::size_t currentTimeAveragedMaskIndex_ = 0;

	std::vector<ObsBridgeUtils::unique_gs_texture_t> bgrxDualKawaseBlurReductionPyramid_;

	// Output capture
	ObsBridgeUtils::unique_gs_texture_t bgrxOutput_;
	std::unique_ptr<ObsBridgeUtils::AsyncTextureReader> bgrxOutputReader_;

	// Rendering state
	std::atomic<MainFilter::FilterLevel> filterLevel_{MainFilter::FilterLevel::Default};
	std::atomic<float> motionIntensityThreshold_{0.0f};
	std::atomic<float> guidedFilterEps_{0.0f};
	std::atomic<float> maskGamma_{2.5f};
	std::atomic<float> maskLowerBound_{0.0f};
	std::atomic<float> maskUpperBoundMargin_{0.0f};
	std::atomic<float> timeAveragedFilteringAlpha_{0.25f};
	std::atomic<bool> enableCenterFrame_{false};
	std::atomic<bool> shouldForceProcessFrame_{true};

	// Buffered video frames for delay
	std::mutex videoQueueMutex_;
	std::deque<BufferedVideoFrame> videoQueue_;

	// Buffered audio frames for delay
	std::mutex audioQueueMutex_;
	std::deque<BufferedAudioFrame> audioQueue_;

	// Global audio info (cached)
	uint32_t audioSamplesPerSec_ = 44100;
	enum speaker_layout audioSpeakers_ = SPEAKERS_STEREO;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::AsyncSource
