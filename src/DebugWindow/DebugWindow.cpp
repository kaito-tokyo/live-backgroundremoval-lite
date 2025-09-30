/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "DebugWindow.hpp"

#include <iostream>

#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QDebug>

#include "BridgeUtils/AsyncTextureReader.hpp"

#include "../Core/MainPluginContext.h"
#include "../Core/RenderingContext.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace {

constexpr char textureBgrxOriginalImage[] = "bgrxOriginalImage";
constexpr char textureR32fOriginalGrayscale[] = "r32fOriginalGrayscale";
constexpr char textureBgrxSegmenterInput[] = "bgrxSegmenterInput";
constexpr char textureR8SegmentationMask[] = "r8SegmentationMask";
constexpr char textureR8SubGFGuide[] = "r8SubGFGuide";
constexpr char textureR8SubGFSource[] = "r8SubGFSource";
constexpr char textureR32fSubGFMeanGuide[] = "r32fSubGFMeanGuide";
constexpr char textureR32fSubGFMeanSource[] = "r32fSubGFMeanSource";
constexpr char textureR16fSubGFMeanGuideSource[] = "r16fSubGFMeanGuideSource";
constexpr char textureR32fSubGFMeanGuideSq[] = "r32fSubGFMeanGuideSq";
constexpr char textureR32fSubGFA[] = "r32fSubGFA";
constexpr char textureR32fSubGFB[] = "r32fSubGFB";
constexpr char textureR8GFResult[] = "r8GFResult";

const std::vector<std::string> bgrxTextures = {textureBgrxOriginalImage};
const std::vector<std::string> r8Textures = {textureR8GFResult};
const std::vector<std::string> r32fTextures = {textureR32fOriginalGrayscale};
const std::vector<std::string> bgrx256Textures = {textureBgrxSegmenterInput};
const std::vector<std::string> r8MaskRoiTextures = {textureR8SegmentationMask};
const std::vector<std::string> r8SubTextures = {textureR8SubGFGuide, textureR8SubGFSource};
const std::vector<std::string> r32fSubTextures = {
	textureR32fSubGFMeanGuide,   textureR32fSubGFMeanSource, textureR16fSubGFMeanGuideSource,
	textureR32fSubGFMeanGuideSq, textureR32fSubGFA,          textureR32fSubGFB};

} // namespace

namespace KaitoTokyo {
namespace BackgroundRemovalLite {

DebugWindow::DebugWindow(std::weak_ptr<MainPluginContext> _weakMainPluginContext, QWidget *parent)
	: QDialog(parent),
	  weakMainPluginContext(std::move(_weakMainPluginContext)),
	  layout(new QVBoxLayout(this)),
	  previewTextureSelector(new QComboBox(this)),
	  previewImageLabel(new QLabel(this)),
	  updateTimer(new QTimer(this))
{
	previewTextureSelector->addItem(textureBgrxOriginalImage);
	previewTextureSelector->addItem(textureR32fOriginalGrayscale);
	previewTextureSelector->addItem(textureBgrxSegmenterInput);
	previewTextureSelector->addItem(textureR8SegmentationMask);
	previewTextureSelector->addItem(textureR8SubGFGuide);
	previewTextureSelector->addItem(textureR8SubGFSource);
	previewTextureSelector->addItem(textureR32fSubGFMeanGuide);
	previewTextureSelector->addItem(textureR32fSubGFMeanSource);
	previewTextureSelector->addItem(textureR16fSubGFMeanGuideSource);
	previewTextureSelector->addItem(textureR32fSubGFMeanGuideSq);
	previewTextureSelector->addItem(textureR32fSubGFA);
	previewTextureSelector->addItem(textureR32fSubGFB);
	previewTextureSelector->addItem(textureR8GFResult);

	layout->addWidget(previewTextureSelector);

	previewImageLabel->setText("No image");
	previewImageLabel->setMaximumSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);

	layout->addWidget(previewImageLabel);

	setLayout(layout);

	connect(updateTimer, &QTimer::timeout, this, &DebugWindow::updatePreview);
	updateTimer->start(1000 / 60); // 約30fpsで更新

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

		if (!readerBgrx || readerBgrx->width != renderingContext->width ||
		    readerBgrx->height != renderingContext->height) {
			readerBgrx = std::make_unique<AsyncTextureReader>(renderingContext->width,
									  renderingContext->height, GS_BGRX);
		}

		if (!readerR8 || readerR8->width != renderingContext->width ||
		    readerR8->height != renderingContext->height) {
			readerR8 = std::make_unique<AsyncTextureReader>(renderingContext->width,
									renderingContext->height, GS_R8);
		}

		if (!readerR32f || readerR32f->width != renderingContext->width ||
		    readerR32f->height != renderingContext->height) {
			readerR32f = std::make_unique<AsyncTextureReader>(renderingContext->width,
									  renderingContext->height, GS_R32F);
		}

		if (!readerMaskRoiR8 || readerMaskRoiR8->width != renderingContext->maskRoiWidth ||
		    readerMaskRoiR8->height != renderingContext->maskRoiHeight) {
			readerMaskRoiR8 = std::make_unique<AsyncTextureReader>(renderingContext->maskRoiWidth,
									       renderingContext->maskRoiHeight, GS_R8);
		}

		if (!reader256Bgrx) {
			reader256Bgrx = std::make_unique<AsyncTextureReader>(256, 256, GS_BGRX);
		}

		if (!readerSubR8 || readerSubR8->width != renderingContext->widthSub ||
		    readerSubR8->height != renderingContext->heightSub) {
			readerSubR8 = std::make_unique<AsyncTextureReader>(renderingContext->widthSub,
									   renderingContext->heightSub, GS_R8);
		}

		if (!readerR32fSub || readerR32fSub->width != renderingContext->widthSub ||
		    readerR32fSub->height != renderingContext->heightSub) {
			readerR32fSub = std::make_unique<AsyncTextureReader>(renderingContext->widthSub,
									     renderingContext->heightSub, GS_R32F);
		}

		auto currentTexture = previewTextureSelector->currentText();
		if (currentTexture == textureBgrxOriginalImage) {
			readerBgrx->sync();
			readerBgrx->stage(renderingContext->bgrxOriginalImage.get());
		} else if (currentTexture == textureR32fOriginalGrayscale) {
			readerR32f->sync();
			readerR32f->stage(renderingContext->r32fOriginalGrayscale.get());
		} else if (currentTexture == textureBgrxSegmenterInput) {
			reader256Bgrx->sync();
			reader256Bgrx->stage(renderingContext->bgrxSegmenterInput.get());
		} else if (currentTexture == textureR8SegmentationMask) {
			readerMaskRoiR8->sync();
			readerMaskRoiR8->stage(renderingContext->r8SegmentationMask.get());
		} else if (currentTexture == textureR8SubGFGuide) {
			readerSubR8->sync();
			readerSubR8->stage(renderingContext->r8SubGFGuide.get());
		} else if (currentTexture == textureR8SubGFSource) {
			readerSubR8->sync();
			readerSubR8->stage(renderingContext->r8SubGFSource.get());
		} else if (currentTexture == textureR32fSubGFMeanGuide) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubGFMeanGuide.get());
		} else if (currentTexture == textureR32fSubGFMeanSource) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubGFMeanSource.get());
		} else if (currentTexture == textureR16fSubGFMeanGuideSource) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubGFMeanGuideSource.get());
		} else if (currentTexture == textureR32fSubGFMeanGuideSq) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubGFMeanGuideSq.get());
		} else if (currentTexture == textureR32fSubGFA) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubGFA.get());
		} else if (currentTexture == textureR32fSubGFB) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubGFB.get());
		} else if (currentTexture == textureR8GFResult) {
			readerR8->sync();
			readerR8->stage(renderingContext->r8GFResult.get());
		}
	}
}

void DebugWindow::updatePreview()
{
	auto currentTexture = previewTextureSelector->currentText();
	auto currentTextureStd = currentTexture.toStdString();

	QImage image;
	weakMainPluginContext.lock()->getLogger().debug("Updating preview for texture: {}", currentTextureStd);
	if (std::find(bgrxTextures.begin(), bgrxTextures.end(), currentTextureStd) != bgrxTextures.end()) {
		image = QImage(readerBgrx->getBuffer().data(), readerBgrx->width, readerBgrx->height,
			       QImage::Format_RGB32);
	} else if (std::find(r8Textures.begin(), r8Textures.end(), currentTextureStd) != r8Textures.end()) {
		image = QImage(readerR8->getBuffer().data(), readerR8->width, readerR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(r32fTextures.begin(), r32fTextures.end(), currentTextureStd) != r32fTextures.end()) {
		auto r32fDataView = reinterpret_cast<const float *>(readerR32f->getBuffer().data());
		bufferR8.resize(readerR32f->width * readerR32f->height);
		for (std::uint32_t i = 0; i < readerR32f->width * readerR32f->height; ++i) {
			bufferR8[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
		}

		image = QImage(bufferR8.data(), readerR32f->width, readerR32f->height, QImage::Format_Grayscale8);
	} else if (std::find(bgrx256Textures.begin(), bgrx256Textures.end(), currentTextureStd) !=
		   bgrx256Textures.end()) {
		image = QImage(reader256Bgrx->getBuffer().data(), reader256Bgrx->width, reader256Bgrx->height,
			       QImage::Format_RGB32);
	} else if (std::find(r8MaskRoiTextures.begin(), r8MaskRoiTextures.end(), currentTextureStd) !=
		   r8MaskRoiTextures.end()) {
		image = QImage(readerMaskRoiR8->getBuffer().data(), readerMaskRoiR8->width, readerMaskRoiR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(r8SubTextures.begin(), r8SubTextures.end(), currentTextureStd) != r8SubTextures.end()) {
		image = QImage(readerSubR8->getBuffer().data(), readerSubR8->width, readerSubR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(r32fSubTextures.begin(), r32fSubTextures.end(), currentTextureStd) !=
		   r32fSubTextures.end()) {
		auto r32fDataView = reinterpret_cast<const float *>(readerR32fSub->getBuffer().data());
		bufferSubR8.resize(readerR32fSub->width * readerR32fSub->height);
		for (std::uint32_t i = 0; i < readerR32fSub->width * readerR32fSub->height; ++i) {
			bufferSubR8[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
		}

		image = QImage(bufferSubR8.data(), readerR32fSub->width, readerR32fSub->height,
			       QImage::Format_Grayscale8);
	}

	QPixmap pixmap = QPixmap::fromImage(image);
	QPixmap scaledPixmap =
		pixmap.scaled(PREVIEW_WIDTH, PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	previewImageLabel->setPixmap(scaledPixmap);
}

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
