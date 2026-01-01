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

#include "PluginConfigDialog.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QTextEdit>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

PluginConfigDialog::PluginConfigDialog(std::shared_ptr<PluginConfig> pluginConfig, QWidget *parent)
	: QDialog(parent),
	  pluginConfig_{std::move(pluginConfig)}
{
	setupUi();
}

PluginConfigDialog::~PluginConfigDialog() = default;

void PluginConfigDialog::setupUi()
{
	setWindowTitle(tr("Live Background Removal Lite - Global Settings"));
	resize(400, 150);

	auto *mainLayout = new QVBoxLayout(this);

	auto *updatesGroup = new QGroupBox(tr("Updates"), this);
	auto *updatesLayout = new QVBoxLayout(updatesGroup);

	auto *chkAutoUpdate = new QCheckBox(tr("Check for updates automatically"), updatesGroup);

	chkAutoUpdate->setChecked(pluginConfig_->isAutoCheckForUpdateEnabled());

	updatesLayout->addWidget(chkAutoUpdate);
	mainLayout->addWidget(updatesGroup);

	mainLayout->addStretch();

	auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	mainLayout->addWidget(buttonBox);

	// Add About and Licenses buttons
	auto *aboutButton = new QPushButton(tr("About Qt"), this);
	auto *licensesButton = new QPushButton(tr("Open Source Licenses"), this);
	buttonBox->addButton(aboutButton, QDialogButtonBox::HelpRole);
	buttonBox->addButton(licensesButton, QDialogButtonBox::HelpRole);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(aboutButton, &QPushButton::clicked, this, [this]() { QMessageBox::aboutQt(this); });

	connect(licensesButton, &QPushButton::clicked, this, [this]() {
		struct LicenseInfo {
			QString name;
			QString resourcePath;
		};
		const QList<LicenseInfo> licenses = {
			{"Main LICENSE file", "://live-backgroundremoval-lite-licenses/LICENSE"},
			{"GNU General Public License v3.0 or later",
			 "://live-backgroundremoval-lite-licenses/LICENSE.GPL-3.0-or-later"},
			{"MIT License", "://live-backgroundremoval-lite-licenses/LICENSE.MIT"},
			{"curl", "://live-backgroundremoval-lite-licenses/curl.txt"},
			{"exprtk", "://live-backgroundremoval-lite-licenses/exprtk.txt"},
			{"fmt", "://live-backgroundremoval-lite-licenses/fmt.txt"},
			{"googletest", "://live-backgroundremoval-lite-licenses/googletest.txt"},
			{"josuttis-jthread", "://live-backgroundremoval-lite-licenses/josuttis-jthread.txt"},
			{"ncnn", "://live-backgroundremoval-lite-licenses/ncnn.txt"},
			{"obs-studio", "://live-backgroundremoval-lite-licenses/obs-studio.txt"},
			{"qt-lgpl-3.0", "://live-backgroundremoval-lite-licenses/qt-lgpl-3.0.txt"},
			{"stb", "://live-backgroundremoval-lite-licenses/stb.txt"},
			{"wolfssl", "://live-backgroundremoval-lite-licenses/wolfssl.txt"},
			{"zlib", "://live-backgroundremoval-lite-licenses/zlib.txt"},
		};

		QString text;
		text += "<b>Live Background Removal Lite</b><br>\n";
		text += "Copyright (C) 2025 Kaito Udagawa &lt;umireon@kaito.tokyo&gt;<br>\n";
		text += "<br>\n";
		text += "This software is licensed under the GNU General Public License v3.0 or later.<br>\n";
		text += "See below for included open source licenses.<br><br>\n";

		for (const auto &license : licenses) {
			QFile file(license.resourcePath);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QTextStream in(&file);
				QString content = in.readAll();
				text += QString("<b>%1</b><br><pre>%2</pre><br>")
						.arg(license.name, content.toHtmlEscaped());
			}
		}

		QDialog dlg(this);
		dlg.setWindowTitle(tr("Open Source Licenses"));
		dlg.resize(700, 600);
		QVBoxLayout *layout = new QVBoxLayout(&dlg);
		QTextEdit *edit = new QTextEdit(&dlg);
		edit->setReadOnly(true);
		edit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
		edit->setHtml(text);
		layout->addWidget(edit);
		QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
		layout->addWidget(box);
		QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
		dlg.exec();
	});

	connect(this, &QDialog::accepted, [this, chkAutoUpdate]() {
		if (chkAutoUpdate->isChecked()) {
			pluginConfig_->setAutoCheckForUpdateEnabled();
		} else {
			pluginConfig_->setAutoCheckForUpdateDisabled();
		}
	});
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
