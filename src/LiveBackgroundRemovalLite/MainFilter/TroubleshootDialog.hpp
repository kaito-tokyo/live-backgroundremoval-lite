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

#include <QDialog>

class QLabel;
class QPushButton;

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

class TroubleshootDialog : public QDialog {
	Q_OBJECT

public:
	explicit TroubleshootDialog(QWidget *parent = nullptr);
	~TroubleshootDialog() override;

private:
	QPushButton *const matrixRoomButton;
	QLabel *const matrixRoomDescriptionLabel;
	QPushButton *const gitHubIssuesButton;
	QLabel *const gitHubIssuesDescriptionLabel;

	void setupUi();
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
