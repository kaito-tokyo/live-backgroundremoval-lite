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

#include "TroubleshootDialog.hpp"

#include <QDesktopServices>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

TroubleshootDialog::TroubleshootDialog(QWidget *parent)
	: QDialog(parent),
	  matrixRoomButton_{new QPushButton(this)},
	  matrixRoomDescriptionLabel_{new QLabel(this)},
	  gitHubIssuesButton_{new QPushButton(this)},
	  gitHubIssuesDescriptionLabel_{new QLabel(this)}
{
	setupUi();

	connect(matrixRoomButton, &QPushButton::clicked, this, [this]() {
		QDesktopServices::openUrl(QUrl("https://matrix.to/#/#live-backgroundremoval-lite:matrix.org"));
		this->accept();
	});

	connect(gitHubIssuesButton, &QPushButton::clicked, this, [this]() {
		QDesktopServices::openUrl(QUrl("https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues"));
		this->accept();
	});
}

TroubleshootDialog::~TroubleshootDialog() = default;

void TroubleshootDialog::setupUi()
{
	QVBoxLayout *layout = new QVBoxLayout();

	QGroupBox *groupBox = new QGroupBox(this);
	groupBox->setTitle(tr("Support and Community"));
	layout->addWidget(groupBox);

	QGridLayout *gridLayout = new QGridLayout(groupBox);
	gridLayout->setColumnStretch(0, 1);
	gridLayout->setColumnStretch(1, 2);
	QMargins gridLayoutMargins = gridLayout->contentsMargins();
	gridLayoutMargins.setTop(0);
	gridLayout->setContentsMargins(gridLayoutMargins);

	matrixRoomButton_->setText(tr("💬 Matrix Chat Room"));
	matrixRoomButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	gridLayout->addWidget(matrixRoomButton_, 0, 0);

	matrixRoomDescriptionLabel_->setText(tr("Join our chat room for interactive support and discussion."));
	gridLayout->addWidget(matrixRoomDescriptionLabel_, 0, 1);

	gitHubIssuesButton_->setText(tr("🐛 GitHub Issues"));
	gitHubIssuesButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	gridLayout->addWidget(gitHubIssuesButton_, 1, 0);

	gitHubIssuesDescriptionLabel_->setText(tr("Report issues or bugs on GitHub."));
	gridLayout->addWidget(gitHubIssuesDescriptionLabel_, 1, 1);

	setLayout(layout);
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
