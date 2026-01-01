/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Background Removal Lite - Global Module
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
