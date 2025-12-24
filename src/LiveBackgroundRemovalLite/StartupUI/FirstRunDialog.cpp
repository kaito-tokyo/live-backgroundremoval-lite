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

#include "FirstRunDialog.hpp"

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

#include <util/config-file.h>

#include <GlobalContext.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI {

namespace {

const QString URL_OFFICIAL = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/";
const QString URL_USAGE = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/usage/";
const QString URL_FORUM = "https://obsproject.com/forum/resources/live-background-removal-lite.2226/";

} // anonymous namespace

FirstRunDialog::FirstRunDialog(std::shared_ptr<Global::GlobalContext> globalContext, QWidget *parent)
	: QDialog(parent),
	  globalContext_(std::move(globalContext))
{
	// ---------------------------------------------------------
	// 1. Color Setup
	// ---------------------------------------------------------
	QPalette pal = parent->palette();

	QString colSubText = pal.color(QPalette::Disabled, QPalette::Text).name();
	QString colLink = pal.color(QPalette::Active, QPalette::Link).name();
	const QString CTA_COLOR = "#ffb74d";

	// ---------------------------------------------------------
	// 2. Dialog Setup & Theme Application
	// ---------------------------------------------------------
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint |
		       Qt::MSWindowsFixedSizeDialogHint);

	setWindowTitle(tr("Live Background Removal Lite - Installation Complete"));

	setStyleSheet("QDialog {"
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
		      "QPushButton:pressed { background-color: palette(dark); }");

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->setContentsMargins(30, 30, 30, 30);
	mainLayout->setSpacing(10);

	// ---------------------------------------------------------
	// 3. Header
	// ---------------------------------------------------------
	QHBoxLayout *headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(20);

	QLabel *iconLabel = new QLabel(this);
	iconLabel->setPixmap(QPixmap(":/live-backgroundremoval-lite/logo-512.png"));
	iconLabel->setScaledContents(true);
	iconLabel->setFixedSize(80, 80);
	headerLayout->addWidget(iconLabel);

	QVBoxLayout *titleAreaLayout = new QVBoxLayout();
	titleAreaLayout->setSpacing(2);
	titleAreaLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

	QLabel *titleLabel = new QLabel(tr("Live Background Removal Lite"), this);
	titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold;");
	titleAreaLayout->addWidget(titleLabel);

	// Subtitle
	QString currentVer = QString::fromStdString(globalContext_->pluginVersion_);
	QString latestVer = QString::fromStdString(globalContext_->getLatestVersion());

	QString subtitleHtml = QString("<span style='font-size: 10pt; color: %1;'>v%2").arg(colSubText, currentVer);
	if (!latestVer.isEmpty()) {
		subtitleHtml += QString(tr(" (Latest: %1)")).arg(latestVer);
	}
	subtitleHtml += "</span>&nbsp;&nbsp;&nbsp;";

	QString linkText = tr("Official Site / Latest");
	subtitleHtml +=
		QString("<a href='%1' style='font-size: 10pt; color: %2;'>%3</a>").arg(URL_OFFICIAL, colLink, linkText);

	QLabel *subtitleLabel = new QLabel(subtitleHtml, this);
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
	QLabel *contentLabel = new QLabel(this);
	contentLabel->setWordWrap(true);
	contentLabel->setOpenExternalLinks(true);

	QString contentHtml =
		tr("<p style='font-size: 13px; margin-bottom: 5px;'>"
		   "Thank you for installing! 🎉<br>"
		   "Now, turn your room into a studio <b>without a green screen</b>.<br>"
		   "You are ready to create immersive streams."
		   "</p>"
		   "<hr style='background-color: %1; height: 1px; border: none;'>"
		   "<p style='font-size: 18px;'><b>[Quick Start]</b></p>"
		   "<ol style='line-height: 140%; margin-top: 0px; margin-bottom: 10px;'>"
		   "<li>Right-click your video source > <b>\"Filters\"</b></li>"
		   "<li>Click <b>[ + ]</b> under Effect Filters and add <b>\"Live Background Removal Lite\"</b></li>"
		   "</ol>"
		   "<p style='margin-bottom: 5px;'>"
		   "<b>✨ Want better results?</b><br>"
		   "Check the <a href='%2' style='color: %3;'>Official Guide</a> for pro tips."
		   "</p>")
			.arg(colSubText, URL_USAGE, colLink);

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

	QLabel *reviewLabel = new QLabel(this);
	reviewLabel->setWordWrap(true);
	reviewLabel->setOpenExternalLinks(true);

	QString reviewHtml = tr("<p style='font-size: 12px; color: %1; margin: 0;'>"
				"This plugin is developed by an individual.<br>"
				"If you like it, a <b>5-star rating (★★★★★) on the forum</b><br>"
				"would mean the world to the developer! 🚀"
				"</p>"
				"<p style='font-size: 13px;'>"
				"<a href='%2' style='color: %3; font-weight: bold;'>"
				"▶ Click here to support with a review"
				"</a>"
				"</p>")
				     .arg(colSubText, URL_FORUM, CTA_COLOR);

	reviewLabel->setText(reviewHtml);
	footerLayout->addWidget(reviewLabel, 1);

	// Close Button
	QPushButton *closeBtn = new QPushButton(tr("Close"), this);
	closeBtn->setDefault(true);
	closeBtn->setMinimumWidth(100);
	closeBtn->setMinimumHeight(36);

	QObject::connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
	footerLayout->addWidget(closeBtn, 0, Qt::AlignBottom);

	mainLayout->addLayout(footerLayout);
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::StartupUI
