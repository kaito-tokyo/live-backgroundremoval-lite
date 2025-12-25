/*
 * Live Background Removal Lite - Global Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

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

private:
	const std::shared_ptr<PluginConfig> pluginConfig_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
