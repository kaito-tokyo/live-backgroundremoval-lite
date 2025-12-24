/*
 * Live Background Removal Lite - StartupUI Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * License: GNU GPL v3 or later
 */

#include "StartupController.hpp"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout> // 追加: Explicitly include this
#include <QTextBrowser>
#include <QPushButton>
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QUrl>
#include <QDebug>
#include <QDesktopServices>
#include <QFrame> // 追加: QFrame用

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

// ==========================================
// 定数定義 (URLや設定値)
// ==========================================
namespace {
    const QString URL_OFFICIAL = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/";
    const QString URL_USAGE    = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/usage/";
    const QString URL_FORUM    = "https://obsproject.com/forum/resources/live-background-removal-lite.2226/";
}

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

    // ---------------------------------------------------------
    // 1. カラー取得 (HTML内で使う特殊な色だけ取得)
    // ---------------------------------------------------------
    QPalette pal = parent->palette();

    // HTML内では palette(role) が使えないため、ここだけはHex文字列化が必要です
    // "サブテキスト(薄い文字)" として Disabled Text の色を取得
    QString colSubText = pal.color(QPalette::Disabled, QPalette::Text).name();

    // リンク色はOBSのテーマに従うため取得不要ですが、明示的に装飾したい場合は以下
    QString colLink = pal.color(QPalette::Active, QPalette::Link).name();

    // レビューボタンの強調色（ここはテーマに関わらず目立つ色を指定）
    const QString CTA_BG = "#e65100";
    const QString CTA_FG = "#ffffff";

    // ---------------------------------------------------------
    // 2. ダイアログ設定 & QSSによるテーマ適用
    // ---------------------------------------------------------
    QDialog *dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    dialog->setWindowTitle("Live Background Removal Lite - インストール完了");

    // 【重要】 ここで palette() を使います
    // これにより、背景色と基本文字色はOBSのテーマに自動追従します
    dialog->setStyleSheet(
        "QDialog {"
        "  background-color: palette(window);"
        "  color: palette(windowText);"
        "}"
        // 閉じるボタンのデザインもここで一括指定（OBSライクなデザイン）
        "QPushButton {"
        "  background-color: palette(button);"
        "  color: palette(buttonText);"
        "  border: 1px solid palette(mid);"
        "  border-radius: 4px;"
        "  padding: 6px;"
        "}"
        "QPushButton:hover { background-color: palette(midlight); }"
        "QPushButton:pressed { background-color: palette(dark); }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(15);

    // ---------------------------------------------------------
    // 3. ヘッダー
    // ---------------------------------------------------------
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(20);

    QLabel *iconLabel = new QLabel(dialog);
    iconLabel->setPixmap(QPixmap(":/live-backgroundremoval-lite/logo-512.png"));
    iconLabel->setScaledContents(true);
    iconLabel->setFixedSize(80, 80);
    headerLayout->addWidget(iconLabel);

    QVBoxLayout *titleAreaLayout = new QVBoxLayout();
    titleAreaLayout->setSpacing(2);
    titleAreaLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    QLabel *titleLabel = new QLabel("Live Background Removal Lite", dialog);
    titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold; margin-bottom: 4px;");
    titleAreaLayout->addWidget(titleLabel);

    // サブタイトル（バージョン情報は薄くしたいので、先ほど取得した colSubText を使う）
    QString currentVer = QString::fromStdString(globalContext_->pluginVersion_);
    QString latestVer  = QString::fromStdString(globalContext_->getLatestVersion());

    QString subtitleHtml = QString("<span style='font-size: 10pt; color: %1;'>v%2").arg(colSubText, currentVer);
    if (!latestVer.isEmpty()) subtitleHtml += QString(" (Latest: %1)").arg(latestVer);
    subtitleHtml += "</span>&nbsp;&nbsp;&nbsp;";
    // リンクは標準色(palette(link))に従うので色指定なしでもOKですが、青を強制するなら colLink を使用
    subtitleHtml += QString("<a href='%1' style='font-size: 10pt; color: %2; text-decoration: none;'>公式サイト / 最新版</a>")
                    .arg(URL_OFFICIAL, colLink);

    QLabel *subtitleLabel = new QLabel(subtitleHtml, dialog);
    subtitleLabel->setOpenExternalLinks(true);
    titleAreaLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleAreaLayout);
    mainLayout->addLayout(headerLayout);

    // 区切り線（QSSで色指定可能）
    QFrame *line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("color: palette(mid);"); // 線の色をテーマに合わせる
    mainLayout->addWidget(line1);

    // ---------------------------------------------------------
    // 4. メインコンテンツ
    // ---------------------------------------------------------
    QLabel *contentLabel = new QLabel(dialog);
    contentLabel->setWordWrap(true);
    contentLabel->setOpenExternalLinks(true);
    // 地の文の色は指定しません（親の palette(windowText) を継承するため）

    QString contentHtml = R"(
        <p style='font-size: 13px; margin-bottom: 5px;'>
            インストールありがとうございます！🎉<br>
            これで、<b>グリーンバックなしで</b>あなたの部屋がスタジオに変わります。<br>
            没入感のある配信を作る準備は完了です。
        </p>
        <hr style='background-color: %1; height: 1px; border: none;'>
        <p><b>【さっそく使ってみよう】</b></p>
        <ol style='line-height: 140%; margin-top: 0px; margin-bottom: 10px;'>
            <li>映像ソースを右クリック ＞ <b>「フィルタ」</b></li>
            <li><b>[ + ]</b> から <b>「Live Background Removal Lite」</b>を追加</li>
        </ol>
        <p style='margin-bottom: 5px;'>
            <b>✨ もっとキレイに抜きたい？</b><br>
            プロ級に仕上げる調整のコツは <a href='%2' style='color: %3; text-decoration: none;'>公式サイトのガイド</a> をご覧ください。
        </p>
    )";
    // HTML内の hr(区切り線) の色はHTML属性なので注入が必要
    contentLabel->setText(contentHtml.arg(colSubText, URL_USAGE, colLink));
    mainLayout->addWidget(contentLabel);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: palette(mid);");
    mainLayout->addWidget(line2);

    // ---------------------------------------------------------
    // 5. フッター
    // ---------------------------------------------------------
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(20);

    QLabel *reviewLabel = new QLabel(dialog);
    reviewLabel->setWordWrap(true);
    reviewLabel->setOpenExternalLinks(true);

    // ★レビューボタン部分
    // 背景色は明示的に指定(CTA_BG)し、文字色は白(CTA_FG)で固定します
    QString reviewHtml = R"(
        <p style='font-size: 12px; color: %1; margin: 0;'>
            このプラグインは個人で開発しています。<br>
            気に入っていただけたら、<b>フォーラムで星（★★★★★）</b>を頂けると<br>
            開発者が泣いて喜びます！🚀<br><br>
            <a href='%2' style='background-color: %3; color: %4; padding: 5px 10px; text-decoration: none; border-radius: 4px; font-weight: bold;'>
               ▶ ここをクリックしてレビューで応援する
            </a>
        </p>
    )";
    reviewLabel->setText(reviewHtml.arg(colSubText, URL_FORUM, CTA_BG, CTA_FG));
    footerLayout->addWidget(reviewLabel, 1);

    // 閉じるボタン
    QPushButton *closeBtn = new QPushButton("閉じる", dialog);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setDefault(true);
    closeBtn->setMinimumWidth(100);
    closeBtn->setMinimumHeight(36);
    // スタイルは上部の setStyleSheet で一括定義済みなのでここでは設定不要

    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::close);
    footerLayout->addWidget(closeBtn, 0, Qt::AlignVCenter);

    mainLayout->addLayout(footerLayout);
    dialog->show();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
