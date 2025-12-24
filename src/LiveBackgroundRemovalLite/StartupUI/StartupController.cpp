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
#include <QHBoxLayout>
#include <QPushButton>
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QUrl>
#include <QFrame>
#include <QPalette>

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

// ==========================================
// Constants (URLs)
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

    if (config_get_bool(config, "live_backgroundremoval_lite", "has_run_before"))
        return false;

    config_set_bool(config, "live_backgroundremoval_lite", "has_run_before", true);
    return config_save_safe(config, ".tmp", ".bak") == CONFIG_SUCCESS;
}

void StartupController::showFirstRunDialog()
{
    QMainWindow *parent = (QMainWindow *)obs_frontend_get_main_window();

    // ---------------------------------------------------------
    // 1. Color Setup
    // ---------------------------------------------------------
    QPalette pal = parent->palette();

    QString colSubText = pal.color(QPalette::Disabled, QPalette::Text).name();
    QString colLink    = pal.color(QPalette::Active, QPalette::Link).name();
    const QString CTA_COLOR = "#ffb74d";

    // ---------------------------------------------------------
    // 2. Dialog Setup & Theme Application
    // ---------------------------------------------------------
    QDialog *dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    // タイトルを翻訳対応 (tr)
    dialog->setWindowTitle(QObject::tr("Live Background Removal Lite - Installation Complete"));

    dialog->setStyleSheet(
        "QDialog {"
        "  background-color: palette(window);"
        "  color: palette(windowText);"
        "}"
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
    mainLayout->setSpacing(10);

    // ---------------------------------------------------------
    // 3. Header
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

    // プラグイン名は固有名詞なのでtr不要かもしれませんが、念のため入れておきます
    QLabel *titleLabel = new QLabel(QObject::tr("Live Background Removal Lite"), dialog);
    titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold;");
    titleAreaLayout->addWidget(titleLabel);

    // Subtitle
    QString currentVer = QString::fromStdString(globalContext_->pluginVersion_);
    QString latestVer  = QString::fromStdString(globalContext_->getLatestVersion());

    // "Latest" などの単語を翻訳対応
    QString subtitleHtml = QString("<span style='font-size: 10pt; color: %1;'>v%2").arg(colSubText, currentVer);
    if (!latestVer.isEmpty()) {
        subtitleHtml += QString(QObject::tr(" (Latest: %1)")).arg(latestVer);
    }
    subtitleHtml += "</span>&nbsp;&nbsp;&nbsp;";

    // リンクテキストを翻訳対応
    QString linkText = QObject::tr("Official Site / Latest");
    subtitleHtml += QString("<a href='%1' style='font-size: 10pt; color: %2; text-decoration: none;'>%3</a>")
                    .arg(URL_OFFICIAL, colLink, linkText);

    QLabel *subtitleLabel = new QLabel(subtitleHtml, dialog);
    subtitleLabel->setOpenExternalLinks(true);
    titleAreaLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleAreaLayout);
    mainLayout->addLayout(headerLayout);

    QFrame *line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("color: palette(mid);");
    mainLayout->addWidget(line1);

    // ---------------------------------------------------------
    // 4. Main Content (Multi-language Support)
    // ---------------------------------------------------------
    QLabel *contentLabel = new QLabel(dialog);
    contentLabel->setWordWrap(true);
    contentLabel->setOpenExternalLinks(true);

    // HTML全体をtrで囲みます。
    // Raw String Literal (R"()") はやめて、通常の文字列結合にします。
    // %1, %2 などのプレースホルダーは翻訳者が位置を変えられるようtrの中に含めます。
    QString contentHtml = QObject::tr(
        "<p style='font-size: 13px; margin-bottom: 5px;'>"
        "Thank you for installing! 🎉<br>"
        "Now, turn your room into a studio <b>without a green screen</b>.<br>"
        "You are ready to create immersive streams."
        "</p>"
        "<hr style='background-color: %1; height: 1px; border: none;'>"
        "<p><b>[Quick Start]</b></p>"
        "<ol style='line-height: 140%; margin-top: 0px; margin-bottom: 10px;'>"
        "<li>Right-click your video source > <b>\"Filters\"</b></li>"
        "<li>Click <b>[ + ]</b> and add <b>\"Live Background Removal Lite\"</b></li>"
        "</ol>"
        "<p style='margin-bottom: 5px;'>"
        "<b>✨ Want better results?</b><br>"
        "Check the <a href='%2' style='color: %3; text-decoration: none;'>Official Guide</a> for pro tips."
        "</p>"
    ).arg(colSubText, URL_USAGE, colLink);

    contentLabel->setText(contentHtml);
    mainLayout->addWidget(contentLabel);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: palette(mid);");
    mainLayout->addWidget(line2);

    // ---------------------------------------------------------
    // 5. Footer (Review Request)
    // ---------------------------------------------------------
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(20);

    QLabel *reviewLabel = new QLabel(dialog);
    reviewLabel->setWordWrap(true);
    reviewLabel->setOpenExternalLinks(true);

    QString reviewHtml = QObject::tr(
        "<p style='font-size: 12px; color: %1; margin: 0;'>"
        "This plugin is developed by an individual.<br>"
        "If you like it, a <b>5-star rating (★★★★★) on the forum</b><br>"
        "would mean the world to the developer! 🚀"
	"</p>"
        "<p style='font-size: 13px;'>"
        "<a href='%2' style='color: %3; font-weight: bold;'>"
        "▶ Click here to support with a review"
        "</a>"
        "</p>"
    ).arg(colSubText, URL_FORUM, CTA_COLOR);

    reviewLabel->setText(reviewHtml);
    footerLayout->addWidget(reviewLabel, 1);

    // Close Button
    QPushButton *closeBtn = new QPushButton(QObject::tr("Close"), dialog);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setDefault(true);
    closeBtn->setMinimumWidth(100);
    closeBtn->setMinimumHeight(36);

    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::close);
    footerLayout->addWidget(closeBtn, 0, Qt::AlignBottom);

    mainLayout->addLayout(footerLayout);
    dialog->show();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
