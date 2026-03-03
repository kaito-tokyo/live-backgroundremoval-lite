// SPDX-FileCopyrightText: 2025 Kaito Udagawa <umireon@kaito.tokyo>
// SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
//
// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: GPL-3.0-or-later

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
	QPushButton *const matrixRoomButton_;
	QLabel *const matrixRoomDescriptionLabel_;
	QPushButton *const gitHubIssuesButton_;
	QLabel *const gitHubIssuesDescriptionLabel_;

	void setupUi();
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
