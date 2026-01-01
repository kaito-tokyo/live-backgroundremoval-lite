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

#include <obs-module.h>

#include <GlobalContext.hpp>
#include <PluginConfig.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

bool loadModule(std::shared_ptr<Global::PluginConfig> pluginConfig,
		std::shared_ptr<Global::GlobalContext> globalContext) noexcept;
void unloadModule() noexcept;

const char *getName(void *) noexcept;
void *create(obs_data_t *settings, obs_source_t *source) noexcept;
void destroy(void *data) noexcept;
uint32_t getWidth(void *data) noexcept;
uint32_t getHeight(void *data) noexcept;
void getDefaults(obs_data_t *data) noexcept;
obs_properties_t *getProperties(void *data) noexcept;
void update(void *data, obs_data_t *settings) noexcept;
void activate(void *data) noexcept;
void deactivate(void *data) noexcept;
void show(void *data) noexcept;
void hide(void *data) noexcept;

void videoTick(void *data, float seconds) noexcept;
void videoRender(void *data, gs_effect_t *effect) noexcept;

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
