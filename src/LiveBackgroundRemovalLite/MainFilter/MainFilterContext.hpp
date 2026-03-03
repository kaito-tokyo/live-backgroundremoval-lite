// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

#include <obs.h>

#include <atomic>
#include <future>
#include <memory>
#include <mutex>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/TaskQueue/ThrottledTaskQueue.hpp>

#include <GlobalContext.hpp>
#include <PluginConfig.hpp>

#include "PluginProperty.hpp"
#include "MainEffect.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

class DebugWindow;
class RenderingContext;

class MainFilterContext : public std::enable_shared_from_this<MainFilterContext> {
public:
	MainFilterContext(obs_data_t *settings, obs_source_t *source,
			  std::shared_ptr<Global::PluginConfig> pluginConfig,
			  std::shared_ptr<Global::GlobalContext> globalContext);

	void shutdown() noexcept;
	~MainFilterContext() noexcept;

	MainFilterContext(const MainFilterContext &) = delete;
	MainFilterContext &operator=(const MainFilterContext &) = delete;
	MainFilterContext(MainFilterContext &&) = delete;
	MainFilterContext &operator=(MainFilterContext &&) = delete;

	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;

	static void getDefaults(obs_data_t *data);

	obs_properties_t *getProperties();
	void update(obs_data_t *settings);
	void activate();
	void deactivate();
	void show();
	void hide();

	void videoTick(float seconds);
	void videoRender();

	const std::shared_ptr<const Logger::ILogger> getLogger() const noexcept { return logger_; }

	std::shared_ptr<RenderingContext> getRenderingContext() const noexcept
	{
		std::lock_guard<std::mutex> lock(renderingContextMutex_);
		return renderingContext_;
	}

private:
	std::shared_ptr<RenderingContext> createRenderingContext(std::uint32_t targetWidth, std::uint32_t targetHeight,
								 int blurSize);
	void applyPluginProperty(const std::shared_ptr<RenderingContext> &_renderingContext);

	obs_source_t *const source_;
	const std::shared_ptr<Global::PluginConfig> pluginConfig_;
	const std::shared_ptr<Global::GlobalContext> globalContext_;

	const std::shared_ptr<const Logger::ILogger> logger_;

	const MainEffect mainEffect_;
	TaskQueue::ThrottledTaskQueue selfieSegmenterTaskQueue_;

	PluginProperty pluginProperty_;

	mutable std::mutex renderingContextMutex_;
	std::shared_ptr<RenderingContext> renderingContext_ = nullptr;

	DebugWindow *debugWindow_ = nullptr;
	mutable std::mutex debugWindowMutex_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
