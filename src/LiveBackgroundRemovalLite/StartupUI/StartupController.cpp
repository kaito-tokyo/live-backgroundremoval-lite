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
#include <QLabel>
#include <QPixmap>
#include <QUrl>
#include <QDebug>
#include <QDesktopServices>

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

StartupController::StartupController(std::shared_ptr<Global::GlobalContext> globalContext)
	: globalContext_(std::move(globalContext)) {};

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
	QMainWindow *parent = (QMainWindow *)obs_frontend_get_main_window();

	QDialog dialog(parent);
	dialog.setWindowTitle("Live Background Removal Lite - インストール完了");
	dialog.resize(500, 380); // 横幅を少し広めに

	// メインのレイアウト（縦並び）
	QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
	mainLayout->setContentsMargins(25, 25, 25, 25);
	mainLayout->setSpacing(15);

	// ==========================================
	// 1. ヘッダー部分（ロゴ + タイトル）の作成
	// ==========================================
	QHBoxLayout *headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(20); // ロゴと文字の間隔

	// (A) 左側のロゴアイコン
	QLabel *iconLabel = new QLabel(&dialog);
	// 正方形のアイコン画像を指定してください
	QPixmap pixmap(":/live-backgroundremoval-lite/logo-512.png");

	if (!pixmap.isNull()) {
		// アイコンサイズにリサイズ（例: 64x64）
		iconLabel->setPixmap(pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
	// ロゴの縦位置を上揃えではなく中央揃えにする
	iconLabel->setAlignment(Qt::AlignCenter);

	headerLayout->addWidget(iconLabel);

	// ---------------------------------------------------------
	// (B) 右側のタイトル・バージョン情報エリア（縦並び）
	// ---------------------------------------------------------
	QVBoxLayout *titleAreaLayout = new QVBoxLayout();
	titleAreaLayout->setSpacing(5);                                  // タイトルとバージョンの間隔
	titleAreaLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft); // 垂直中央・左寄せ

	// --- 1. タイトル ---
	QLabel *titleLabel = new QLabel("Live Background Removal Lite", &dialog);
	// OBSのテーマを上書きして大きく表示
	titleLabel->setStyleSheet("font-size: 26pt; font-weight: bold;");
	titleAreaLayout->addWidget(titleLabel);

	// --- 2. バージョン情報 ---
	// 文字列の取得と整形
	QString currentVer = QString::fromStdString(globalContext_->pluginVersion_);
	QString latestVer = QString::fromStdString(globalContext_->getLatestVersion());

	QString versionText = QString("Version: %1").arg(currentVer);

	// 最新バージョンが取得できていて、かつ現在と違う場合のみ表示するなど
	if (!latestVer.isEmpty()) {
		versionText += QString("  /  Latest: %1").arg(latestVer);
	}

	QLabel *versionLabel = new QLabel(versionText, &dialog);
	// 少し小さく、グレーにして目立ちすぎないようにする
	versionLabel->setStyleSheet("font-size: 11pt; color: #888888;");
	titleAreaLayout->addWidget(versionLabel);

	// 作成したタイトルエリアをヘッダーレイアウトに追加
	// addWidget ではなく addLayout を使う点に注意してください
	headerLayout->addLayout(titleAreaLayout);

	// ヘッダーレイアウトをメインレイアウトに追加
	mainLayout->addLayout(headerLayout);

	// 区切り線（お好みで）
	QFrame *line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	mainLayout->addWidget(line);

	// ==========================================
	// 2. 説明テキストエリア
	// ==========================================
	QLabel *textLabel = new QLabel(&dialog);
	textLabel->setWordWrap(true);
	textLabel->setTextFormat(Qt::RichText);
	textLabel->setOpenExternalLinks(true);

	textLabel->setText(
		"<p>インストールありがとうございます！正常に読み込まれました。</p>"
		"<p><b>【使い方】</b></p>"
		"<ol style='line-height: 140%;'>"
		"<li>映像ソースを右クリック ＞ <b>「フィルタ」</b></li>"
		"<li>左下の <b>[ + ]</b> から <b>「Live Background Removal Lite」</b>を追加</li>"
		"</ol>"
		"<p style='color: #888; font-size: 11px;'>詳しい情報は <a href='https://kaito-tokyo.github.io/live-backgroundremoval-lite/'>公式サイト</a> をご覧ください。</p>");
	mainLayout->addWidget(textLabel);

	// 下に寄せるためのスペース
	mainLayout->addStretch();

	// ==========================================
	// 3. フッターボタン（フォーラム ＆ 閉じる）
	// ==========================================
	QHBoxLayout *btnLayout = new QHBoxLayout();

	// フォーラムボタン
	QPushButton *forumBtn = new QPushButton("フォーラムで応援する", &dialog);
	forumBtn->setCursor(Qt::PointingHandCursor);
	QObject::connect(forumBtn, &QPushButton::clicked,
			 []() { QDesktopServices::openUrl(QUrl("https://obsproject.com/forum/resources/xxxx/")); });
	btnLayout->addWidget(forumBtn);

	btnLayout->addStretch(); // ボタン間のスペース

	// 閉じるボタン
	QPushButton *closeBtn = new QPushButton("閉じる", &dialog);
	closeBtn->setCursor(Qt::PointingHandCursor);
	closeBtn->setDefault(true);
	QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
	btnLayout->addWidget(closeBtn);

	mainLayout->addLayout(btnLayout);

	dialog.exec();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
