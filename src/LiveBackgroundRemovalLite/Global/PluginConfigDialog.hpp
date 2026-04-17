// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>

#include "GlobalContext.hpp"
#include "PluginConfig.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

class PluginConfigDialog : public QDialog {
	Q_OBJECT

public:
	explicit PluginConfigDialog(std::shared_ptr<PluginConfig> pluginConfig,
				    std::shared_ptr<GlobalContext> globalContext, QWidget *parent = nullptr);
	~PluginConfigDialog() override;

private:
	void setupUi();
	const std::shared_ptr<PluginConfig> pluginConfig_;
	const std::shared_ptr<GlobalContext> globalContext_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
