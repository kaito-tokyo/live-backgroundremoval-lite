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
#include <QPainter>

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
	dialog.resize(500, 420); // コンテンツが増えたので縦を少し余裕を持たせる

	// メインのレイアウト（縦並び）
	QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
	mainLayout->setContentsMargins(30, 30, 30, 30); // 余白を少しリッチに
	mainLayout->setSpacing(20);

	// ==========================================
	// 1. ヘッダー部分
	// ==========================================
	QHBoxLayout *headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(20);

	// (A) 左側のロゴアイコン（角丸処理）
	QLabel *iconLabel = new QLabel(&dialog);
	QPixmap srcPixmap(":/live-backgroundremoval-lite/logo-512.png");

	if (!srcPixmap.isNull()) {
		QPixmap resizedPixmap = srcPixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		QPixmap roundedPixmap(resizedPixmap.size());
		roundedPixmap.fill(Qt::transparent);

		QPainter painter(&roundedPixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		QPainterPath path;
		path.addRoundedRect(roundedPixmap.rect(), 18, 18); // 丸みを少し強く(18)
		painter.setClipPath(path);
		painter.drawPixmap(0, 0, resizedPixmap);

		iconLabel->setPixmap(roundedPixmap);
	}
	iconLabel->setAlignment(Qt::AlignCenter);
	headerLayout->addWidget(iconLabel);

	// (B) 右側のタイトル・バージョン情報エリア
	QVBoxLayout *titleAreaLayout = new QVBoxLayout();
	titleAreaLayout->setSpacing(2);
	titleAreaLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

	// タイトル
	QLabel *titleLabel = new QLabel("Live Background Removal Lite", &dialog);
	// OBSのスタイルを上書きして大きく表示
	titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold; margin-bottom: 4px;");
	titleLabel->setWordWrap(true);
	titleAreaLayout->addWidget(titleLabel);

	// サブタイトル行（バージョン + 公式サイトリンク）
	QHBoxLayout *subtitleLayout = new QHBoxLayout();
	subtitleLayout->setSpacing(15);
	subtitleLayout->setAlignment(Qt::AlignLeft);

	QString currentVer = QString::fromStdString(globalContext_->pluginVersion_);
	QString latestVer = QString::fromStdString(globalContext_->getLatestVersion());

	QString versionText = QString("v%1").arg(currentVer);
	if (!latestVer.isEmpty()) {
		versionText += QString(" (Latest: %1)").arg(latestVer);
	}

	QLabel *versionLabel = new QLabel(versionText, &dialog);
	versionLabel->setStyleSheet("font-size: 10pt; color: #888888;");
	subtitleLayout->addWidget(versionLabel);

	// 公式サイトリンク
	QLabel *linkLabel = new QLabel(
		"<a href='https://kaito-tokyo.github.io/live-backgroundremoval-lite/' style='color: #66aaff; text-decoration: none;'>"
		"公式サイト / 最新版"
		"</a>",
		&dialog);
	linkLabel->setOpenExternalLinks(true);
	linkLabel->setTextFormat(Qt::RichText);
	linkLabel->setStyleSheet("font-size: 10pt;");
	subtitleLayout->addWidget(linkLabel);

	subtitleLayout->addStretch();
	titleAreaLayout->addLayout(subtitleLayout);
	headerLayout->addLayout(titleAreaLayout);

	mainLayout->addLayout(headerLayout);

	// 区切り線
	QFrame *line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	mainLayout->addWidget(line);

	// ==========================================
	// 2. 説明テキストエリア（ワクワク感 ＆ レビュー誘導）
	// ==========================================
	QLabel *textLabel = new QLabel(&dialog);
	textLabel->setWordWrap(true);
	textLabel->setTextFormat(Qt::RichText);
	textLabel->setOpenExternalLinks(true);

	QString usageUrl = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/usage/";
	// 実際のフォーラムURL
	QString forumUrl = "https://obsproject.com/forum/resources/live-background-removal-lite.2226/";

	textLabel->setText("<p style='font-size: 13px; margin-bottom: 5px;'>"
			   "インストールありがとうございます！🎉<br>"
			   "これで、<b>グリーンバックなしで</b>あなたの部屋がスタジオに変わります。<br>"
			   "没入感のある配信を作る準備は完了です。</p>"

			   "<hr style='background-color: #444; height: 1px; border: none;'>"

			   "<p><b>【さっそく使ってみよう】</b></p>"
			   "<ol style='line-height: 140%; margin-top: 0px; margin-bottom: 10px;'>"
			   "<li>映像ソースを右クリック ＞ <b>「フィルタ」</b></li>"
			   "<li><b>[ + ]</b> から <b>「Live Background Removal Lite」</b>を追加</li>"
			   "</ol>"

			   "<p style='margin-bottom: 10px;'>"
			   "<b>✨ もっとキレイに抜きたい？</b><br>"
			   "プロ級に仕上げる調整のコツは <a href='" +
			   usageUrl +
			   "' style='color: #66aaff; text-decoration: none;'>"
			   "公式サイトのガイド</a> をご覧ください。</p>"

			   "<hr style='background-color: #444; height: 1px; border: none;'>"

			   "<p style='font-size: 12px; color: #ccc; margin-top: 5px;'>"
			   "このプラグインは個人で開発しています。<br>"
			   "もし気に入っていただけたら、<b>フォーラムで星（★★★★★）</b>を頂けると<br>"
			   "開発者が泣いて喜びますし、次のアップデートへの爆速燃料になります！🚀<br>"
			   "<div style='margin-top: 8px; font-size: 13px;'>"
			   "<a href='" +
			   forumUrl +
			   "' style='color: #ffb74d; text-decoration: none; font-weight: bold;'>"
			   "▶ ここをクリックしてレビューで応援する"
			   "</a></div></p>");
	mainLayout->addWidget(textLabel);

	// バネを入れてテキストを上に詰める
	mainLayout->addStretch();

	// ==========================================
	// 3. フッター（閉じるボタンのみ）
	// ==========================================
	QHBoxLayout *btnLayout = new QHBoxLayout();

	// 【変更点】フォーラムボタンを削除しました

	// 閉じるボタン（右寄せ）
	btnLayout->addStretch(); // 左側の余白を埋める

	QPushButton *closeBtn = new QPushButton("閉じる", &dialog);
	closeBtn->setCursor(Qt::PointingHandCursor);
	closeBtn->setDefault(true);
	// ボタンの幅を少し広げて押しやすく
	closeBtn->setMinimumWidth(120);
	closeBtn->setMinimumHeight(32);

	QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
	btnLayout->addWidget(closeBtn);

	mainLayout->addLayout(btnLayout);

	dialog.exec();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
