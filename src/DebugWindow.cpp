/*
OBS Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "DebugWindow.hpp"

#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QTimer>

#include "AsyncTextureReader.hpp"
#include "MainPluginContext.h"
#include "RenderingContext.hpp"

namespace {

constexpr char textureBgrxOriginalImage[] = "bgrxOriginalImage";
constexpr char textureR8OriginalGrayscale[] = "r8OriginalGrayscale";
constexpr char textureBgrxSegmenterInput[] = "bgrxSegmenterInput";

} // namespace

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

DebugWindow::DebugWindow(std::weak_ptr<MainPluginContext> _weakMainPluginContext, QWidget *parent)
	: QDialog(parent),
	  weakMainPluginContext(std::move(_weakMainPluginContext)),
	  layout(new QVBoxLayout(this)),
	  previewTextureSelector(new QComboBox(this)),
	  previewImageLabel(new QLabel(this)),
	  updateTimer(new QTimer(this))
{
	previewTextureSelector->addItem(textureBgrxOriginalImage);
	previewTextureSelector->addItem(textureR8OriginalGrayscale);
	previewTextureSelector->addItem(textureBgrxSegmenterInput);

	layout->addWidget(previewTextureSelector);

	previewImageLabel->setText("No image");
	previewImageLabel->setMaximumSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);

	layout->addWidget(previewImageLabel);

	setLayout(layout);

    connect(updateTimer, &QTimer::timeout, this, &DebugWindow::updatePreview);
    updateTimer->start(100); // 約30fpsで更新

	connect(this, &DebugWindow::readerReady, this, &DebugWindow::updatePreview);
}

DebugWindow::~DebugWindow() noexcept {}

void DebugWindow::videoRender()
{
	if (auto mainPluginContext = weakMainPluginContext.lock()) {
		auto renderingContext = mainPluginContext->getRenderingContext();
		if (!renderingContext) {
			previewImageLabel->setText("No rendering context");
			return;
		}

		if (!readerBgrx || readerBgrx->width != renderingContext-> width || readerBgrx->height != renderingContext->height) {
			readerBgrx = std::make_unique<AsyncTextureReader>(renderingContext->width,
									  renderingContext->height, GS_BGRX);
		}

		if (!readerR8 || readerR8->width != renderingContext->width || readerR8->height != renderingContext->height) {
			readerR8 = std::make_unique<AsyncTextureReader>(renderingContext->width,
									  renderingContext->height, GS_R8);
		}

		if (!reader256Bgrx) {
			reader256Bgrx = std::make_unique<AsyncTextureReader>(256, 256, GS_BGRX);
		}

		if (previewTextureSelector->currentText() == textureBgrxOriginalImage) {
			if (readerBgrx && readerBgrx->width == renderingContext->width && readerBgrx->height == renderingContext->height) {
				readerBgrx->sync();
				readerBgrx->stage(renderingContext->bgrxOriginalImage.get());
			}
		} else if (previewTextureSelector->currentText() == textureR8OriginalGrayscale) {
			if (readerR8 && readerR8->width == renderingContext->width && readerR8->height == renderingContext->height) {
				readerR8->sync();
				readerR8->stage(renderingContext->r8OriginalGrayscale.get());
			}
		} else if (previewTextureSelector->currentText() == textureBgrxSegmenterInput) {
			reader256Bgrx->sync();
			reader256Bgrx->stage(renderingContext->bgrxSegmenterInput.get());
		}
	}
}

void DebugWindow::updatePreview()
{
	if (previewTextureSelector->currentText() == textureBgrxOriginalImage) {
		auto bgrxData = readerBgrx->getBuffer().data();
		if (bgrxData) {
			QImage image(bgrxData, readerBgrx->width, readerBgrx->height,
						QImage::Format_RGB32);
			QPixmap pixmap = QPixmap::fromImage(image);
			QPixmap scaledPixmap = pixmap.scaled(PREVIEW_WIDTH, PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

			previewImageLabel->setPixmap(scaledPixmap);
		}
	} else if (previewTextureSelector->currentText() == textureR8OriginalGrayscale) {
		auto r8Data = readerR8->getBuffer().data();
		if (r8Data) {
			QImage image(r8Data, readerR8->width, readerR8->height,
						QImage::Format_Grayscale8);
			QPixmap pixmap = QPixmap::fromImage(image);
			QPixmap scaledPixmap = pixmap.scaled(PREVIEW_WIDTH, PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

			previewImageLabel->setPixmap(scaledPixmap);
		}
	} else if (previewTextureSelector->currentText() == textureBgrxSegmenterInput) {
		auto bgrxData = reader256Bgrx->getBuffer().data();
		if (bgrxData) {
			QImage image(bgrxData, 256, 256, QImage::Format_RGB32);
			QPixmap pixmap = QPixmap::fromImage(image);

			QPixmap scaledPixmap = pixmap.scaled(PREVIEW_WIDTH, PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

			previewImageLabel->setPixmap(scaledPixmap);
		}
	}
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
