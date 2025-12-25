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

#include "PluginConfigDialog.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <obs-data.h>

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

	chkAutoUpdate->setChecked(!pluginConfig_->disableAutoCheckForUpdate);

	updatesLayout->addWidget(chkAutoUpdate);
	mainLayout->addWidget(updatesGroup);

	mainLayout->addStretch();

	auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	mainLayout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(this, &QDialog::accepted, [this, chkAutoUpdate]() {
		if (chkAutoUpdate->isChecked()) {
			pluginConfig_->setAutoCheckForUpdateEnabled();
		} else {
			pluginConfig_->setAutoCheckForUpdateDisabled();
		}
	});
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
