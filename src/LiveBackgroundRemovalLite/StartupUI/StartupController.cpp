// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
		dialog->raise();
		dialog->activateWindow();
	}
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
