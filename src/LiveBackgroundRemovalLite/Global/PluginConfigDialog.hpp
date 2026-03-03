// SPDX-FileCopyrightText: 2025 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>

#include "PluginConfig.hpp"

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

class PluginConfigDialog : public QDialog {
	Q_OBJECT

public:
	explicit PluginConfigDialog(std::shared_ptr<PluginConfig> pluginConfig, QWidget *parent = nullptr);
	~PluginConfigDialog() override;

private:
	void setupUi();
	const std::shared_ptr<PluginConfig> pluginConfig_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
