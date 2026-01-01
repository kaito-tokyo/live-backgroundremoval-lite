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

#include "StartupController.hpp"

#include <QMainWindow>

#include "FirstRunDialog.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

// ==========================================
// Constants (URLs)
// ==========================================
namespace {
const QString URL_OFFICIAL = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/";
const QString URL_USAGE = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/usage/";
const QString URL_FORUM = "https://obsproject.com/forum/resources/live-background-removal-lite.2226/";
} // namespace

StartupController::StartupController(std::shared_ptr<Global::PluginConfig> pluginConfig,
				     std::shared_ptr<Global::GlobalContext> globalContext)
	: pluginConfig_(std::move(pluginConfig)),
	  globalContext_(std::move(globalContext))
{
}

void StartupController::showFirstRunDialog()
{
	if (QMainWindow *parent = static_cast<QMainWindow *>(obs_frontend_get_main_window())) {
		FirstRunDialog *dialog = new FirstRunDialog(pluginConfig_, globalContext_, parent);
		dialog->show();
	}
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
