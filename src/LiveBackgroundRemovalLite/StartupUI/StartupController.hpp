// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <GlobalContext.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

class StartupController {
public:
	explicit StartupController(std::shared_ptr<Global::PluginConfig> pluginConfig,
				   std::shared_ptr<Global::GlobalContext> globalContext);

	void showFirstRunDialog();

private:
	std::shared_ptr<Global::PluginConfig> pluginConfig_;
	std::shared_ptr<Global::GlobalContext> globalContext_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
