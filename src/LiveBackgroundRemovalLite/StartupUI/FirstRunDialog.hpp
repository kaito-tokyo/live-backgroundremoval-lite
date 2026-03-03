// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QWidget>

#include <util/config-file.h>

#include <GlobalContext.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

class FirstRunDialog : public QDialog {
	Q_OBJECT
public:
	FirstRunDialog(std::shared_ptr<Global::PluginConfig> pluginConfig,
		       std::shared_ptr<Global::GlobalContext> globalContext, QWidget *parent = nullptr);

private:
	const std::shared_ptr<Global::PluginConfig> pluginConfig_;
	const std::shared_ptr<Global::GlobalContext> globalContext_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
