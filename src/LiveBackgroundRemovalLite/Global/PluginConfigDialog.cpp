// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginConfigDialog.hpp"

#include <stdexcept>

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

PluginConfigDialog::PluginConfigDialog(std::shared_ptr<PluginConfig> pluginConfig,
				       std::shared_ptr<GlobalContext> globalContext, QWidget *parent)
	: QDialog(parent),
	  pluginConfig_{std::move(pluginConfig)},
	  globalContext_{globalContext ? std::move(globalContext)
				       : throw std::invalid_argument(
						 "GlobalContextIsNullError(PluginConfigDialog::PluginConfigDialog)")}
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
		const QString prefix = QString::fromStdString(globalContext_->getQtResourcePrefix());
		const QList<LicenseInfo> licenses = {
			{"GNU General Public License v3.0 or later",
			 QString(":%1/LICENSES/GPL-3.0-or-later.txt").arg(prefix)},
			{"Apache License 2.0", QString(":%1/LICENSES/Apache-2.0.txt").arg(prefix)},
			{"curl", QString(":%1/data/licenses/curl.txt").arg(prefix)},
			{"fmt", QString(":%1/data/licenses/fmt.txt").arg(prefix)},
			{"josuttis-jthread", QString(":%1/data/licenses/josuttis-jthread.txt").arg(prefix)},
			{"ncnn", QString(":%1/data/licenses/ncnn.txt").arg(prefix)},
			{"obs-studio", QString(":%1/data/licenses/obs-studio.txt").arg(prefix)},
			{"wolfssl", QString(":%1/data/licenses/wolfssl.txt").arg(prefix)},
			{"zlib", QString(":%1/data/licenses/zlib.txt").arg(prefix)},
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
