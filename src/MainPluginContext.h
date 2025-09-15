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

#include <obs.h>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "obs-backgroundremoval-lite"
#endif
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0-alpha"
#endif

#ifdef __cplusplus

#include <future>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <vector>

#include <net.h>

#include "obs-bridge-utils/gs_unique.hpp"
#include <obs-bridge-utils/ObsLogger.hpp>

#include "AsyncTextureReader.hpp"
#include "MainEffect.hpp"
#include "MyCprSession.hpp"
#include "Preset.hpp"
#include "SelfieSegmenter.hpp"
#include "TaskQueue.hpp"
#include "UpdateChecker/UpdateChecker.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class MainPluginContext : public std::enable_shared_from_this<MainPluginContext> {
public:
	obs_source_t *const source;
	const kaito_tokyo::obs_bridge_utils::ObsLogger logger;
	const MainEffect mainEffect;

	MainPluginContext(obs_data_t *settings, obs_source_t *source);
	void startup();
	void shutdown();
	~MainPluginContext() noexcept;

	std::uint32_t getWidth() const noexcept;
	std::uint32_t getHeight() const noexcept;

	static void getDefaults(obs_data_t *data) noexcept;

	obs_properties_t *getProperties();
	void update(obs_data_t *settings);
	void activate();
	void deactivate();
	void show();
	void hide();

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

private:
	SelfieSegmenter selfieSegmenter;
	std::unique_ptr<TaskQueue> selfieSegmenterTaskQueue;
	TaskQueue::CancellationToken selfieSegmenterPendingTaskToken;
	std::mutex selfieSegmenterPendingTaskTokenMutex;
	UpdateChecker<MyCprSession> updateChecker;

	uint32_t width = 0;
	uint32_t height = 0;

	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxSourceInput = nullptr;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxSegmenterInput = nullptr;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8Mask = nullptr;

	std::vector<std::uint8_t> scaledMaskData;

	std::unique_ptr<AsyncTextureReader> readerSegmenterInput = nullptr;

	std::shared_future<std::optional<std::string>> futureLatestVersion;

	std::optional<std::string> getLatestVersion() const;
	void ensureTextures();
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo

extern "C" {
#endif // __cplusplus

const char *main_plugin_context_get_name(void *type_data);
void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source);
void main_plugin_context_destroy(void *data);
uint32_t main_plugin_context_get_width(void *data);
uint32_t main_plugin_context_get_height(void *data);
void main_plugin_context_get_defaults(obs_data_t *data);
obs_properties_t *main_plugin_context_get_properties(void *data);
void main_plugin_context_update(void *data, obs_data_t *settings);
void main_plugin_context_activate(void *data);
void main_plugin_context_deactivate(void *data);
void main_plugin_context_show(void *data);
void main_plugin_context_hide(void *data);
void main_plugin_context_video_tick(void *data, float seconds);
void main_plugin_context_video_render(void *data, gs_effect_t *effect);
struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame);

bool main_plugin_context_module_load(void);
void main_plugin_context_module_unload(void);

#ifdef __cplusplus
}
#endif // __cplusplus
