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
	resize(400, 150); // 適度なサイズに設定

	// メインレイアウト
	auto *mainLayout = new QVBoxLayout(this);

	// グループボックス（設定項目をまとめる枠）
	auto *updatesGroup = new QGroupBox(tr("Updates"), this);
	auto *updatesLayout = new QVBoxLayout(updatesGroup);

	// チェックボックスの作成
	auto *chkAutoUpdate = new QCheckBox(tr("Check for updates automatically"), updatesGroup);

	// 現在の設定値を読み込んでチェックボックスに反映
	// ※ PluginConfigクラスに getEnableAutoUpdate() メソッドがあると仮定しています
	chkAutoUpdate->setChecked(!pluginConfig_->disableAutoCheckForUpdate);

	updatesLayout->addWidget(chkAutoUpdate);
	mainLayout->addWidget(updatesGroup);

	// ダイアログの下部にスペースを追加（見た目の調整）
	mainLayout->addStretch();

	// OK / Cancel ボタン
	auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	mainLayout->addWidget(buttonBox);

	// ボタンのシグナル接続
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	// OKが押されたときの処理：設定を保存する
	connect(this, &QDialog::accepted, [this, chkAutoUpdate]() {
		// ※ PluginConfigクラスに setEnableAutoUpdate() と save() メソッドがあると仮定しています
		pluginConfig_->setDisableAutoCheckForUpdate(!chkAutoUpdate->isChecked());
	});
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
