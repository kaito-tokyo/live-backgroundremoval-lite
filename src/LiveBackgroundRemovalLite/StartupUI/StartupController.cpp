/*
 * Live Background Removal Lite - StartupUI Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include "StartupController.hpp"

#include <QDialog>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QMainWindow>

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

bool StartupController::checkIfFirstRunCertainly()
{
	config_t *config = obs_frontend_get_user_config();

	config_set_default_bool(config, "live_backgroundremoval_lite", "has_run_before", false);
	bool hasRunBefore = config_get_bool(config, "live_backgroundremoval_lite", "has_run_before");

	if (hasRunBefore)
		return false;

	config_set_bool(config, "live_backgroundremoval_lite", "has_run_before", true);
	return config_save_safe(config, ".tmp", ".bak") == CONFIG_SUCCESS;
}

void StartupController::showFirstRunDialog()
{
	QMainWindow *parent = reinterpret_cast<QMainWindow *>(
		obs_frontend_get_main_window());

	QDialog *dialog = new QDialog(parent);
	dialog->setWindowTitle("HTML Viewer");
	dialog->resize(400, 300);

	QVBoxLayout *layout = new QVBoxLayout(dialog);

	// HTMLを表示するためのウィジェット
	QTextBrowser *browser = new QTextBrowser(dialog);
	// browser->setHtml(
	// 	"<h1 style='background-color:skyblue;'>レポート</h1>"
	// 	"<p>ここに詳しい内容を表示します。<br>"
	// 	"表組みなども簡易的に可能です。</p>"
	// 	"<table border='1' cellspacing='0' cellpadding='5'>"
	// 	"<tr><td>A</td><td>B</td></tr>"
	// 	"<tr><td>100</td><td>200</td></tr>"
	// 	"</table>"
	// );
	browser->setSource(QUrl("https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-version.txt"));

	// リンクを開けるようにする場合
	browser->setOpenExternalLinks(true);

	layout->addWidget(browser);

	// 閉じるボタン
	QPushButton *closeBtn = new QPushButton("閉じる", dialog);
	QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
	layout->addWidget(closeBtn);

	dialog->exec();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
