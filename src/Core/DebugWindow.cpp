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

#include "../BridgeUtils/AsyncTextureReader.hpp"
#include "../BridgeUtils/ObsLogger.hpp"

#include "MainPluginContext.h"
#include "RenderingContext.hpp"

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
constexpr char textureR8GuidedFilterResult[] = "r8GuidedFilterResult";
constexpr char textureR8TimeAveragedMasks0[] = "r8TimeAveragedMasks[0]";
constexpr char textureR8TimeAveragedMasks1[] = "r8TimeAveragedMasks[1]";

const std::vector<std::string> bgrxTextures = {textureBgrxOriginalImage};
const std::vector<std::string> r8Textures = {textureR8GuidedFilterResult, textureR8TimeAveragedMasks0,
					     textureR8TimeAveragedMasks1};
const std::vector<std::string> r32fTextures = {textureR32fOriginalGrayscale};
const std::vector<std::string> bgrx256Textures = {textureBgrxSegmenterInput};
const std::vector<std::string> r8MaskRoiTextures = {textureR8SegmentationMask};
const std::vector<std::string> r8SubTextures = {textureR8SubGFGuide, textureR8SubGFSource};
const std::vector<std::string> r32fSubTextures = {
	textureR32fSubGFMeanGuide,   textureR32fSubGFMeanSource, textureR16fSubGFMeanGuideSource,
	textureR32fSubGFMeanGuideSq, textureR32fSubGFA,          textureR32fSubGFB};

} // namespace

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

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
	previewTextureSelector->addItem(textureR8GuidedFilterResult);
	previewTextureSelector->addItem(textureR8TimeAveragedMasks0);
	previewTextureSelector->addItem(textureR8TimeAveragedMasks1);

	layout->addWidget(previewTextureSelector);

	previewImageLabel->setText("No image");
	previewImageLabel->setMaximumSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);

	layout->addWidget(previewImageLabel);

	setLayout(layout);

	connect(updateTimer, &QTimer::timeout, this, &DebugWindow::updatePreview);
	updateTimer->start(1000 / 60);

	connect(this, &DebugWindow::readerReady, this, &DebugWindow::updatePreview);
}

void DebugWindow::videoRender()
{
	DebugRenderData *renderData = atomicRenderData.load(std::memory_order_acquire);
	if (!renderData) {
		return;
	}

	if (auto mainPluginContext = weakMainPluginContext.lock()) {
		auto renderingContext = mainPluginContext->getRenderingContext();
		if (!renderingContext) {
			return;
		}

		auto currentTexture = previewTextureSelector->currentText();
		if (currentTexture == textureBgrxOriginalImage) {
			if (renderData->readerBgrx)
				renderData->readerBgrx->stage(renderingContext->bgrxSource);
		} else if (currentTexture == textureR32fOriginalGrayscale) {
			if (renderData->readerR32f)
				renderData->readerR32f->stage(renderingContext->r32fGrayscale);
		} else if (currentTexture == textureBgrxSegmenterInput) {
			if (renderData->reader256Bgrx)
				renderData->reader256Bgrx->stage(renderingContext->bgrxSegmenterInput);
		} else if (currentTexture == textureR8SegmentationMask) {
			if (renderData->readerMaskRoiR8)
				renderData->readerMaskRoiR8->stage(renderingContext->r8SegmentationMask);
		} else if (currentTexture == textureR8SubGFGuide) {
			if (renderData->readerSubR8)
				renderData->readerSubR8->stage(renderingContext->r8SubGFGuide);
		} else if (currentTexture == textureR8SubGFSource) {
			if (renderData->readerSubR8)
				renderData->readerSubR8->stage(renderingContext->r8SubGFSource);
		} else if (currentTexture == textureR32fSubGFMeanGuide) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanGuide);
		} else if (currentTexture == textureR32fSubGFMeanSource) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanSource);
		} else if (currentTexture == textureR16fSubGFMeanGuideSource) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanGuideSource);
		} else if (currentTexture == textureR32fSubGFMeanGuideSq) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanGuideSq);
		} else if (currentTexture == textureR32fSubGFA) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFA);
		} else if (currentTexture == textureR32fSubGFB) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFB);
		} else if (currentTexture == textureR8GuidedFilterResult) {
			if (renderData->readerR8)
				renderData->readerR8->stage(renderingContext->r8GuidedFilterResult);
		} else if (currentTexture == textureR8TimeAveragedMasks0) {
			if (renderData->readerR8)
				renderData->readerR8->stage(renderingContext->r8TimeAveragedMasks[0]);
		} else if (currentTexture == textureR8TimeAveragedMasks1) {
			if (renderData->readerR8)
				renderData->readerR8->stage(renderingContext->r8TimeAveragedMasks[1]);
		}
	}
}

void DebugWindow::updatePreview()
{
	oldRenderData.clear();

	auto mainPluginContext = weakMainPluginContext.lock();
	if (!mainPluginContext) {
		return;
	}

	auto renderingContext = mainPluginContext->getRenderingContext();
	if (!renderingContext) {
		previewImageLabel->setText("No rendering context");
		if (currentRenderData) {
			oldRenderData.push_back(std::move(currentRenderData));
			atomicRenderData.store(nullptr, std::memory_order_release);
		}
		return;
	}

	bool needsRecreation = !currentRenderData ||
			       (currentRenderData->readerBgrx &&
				(currentRenderData->readerBgrx->getWidth() != renderingContext->region.width ||
				 currentRenderData->readerBgrx->getHeight() != renderingContext->region.height)) ||
			       (currentRenderData->readerSubR8 &&
				(currentRenderData->readerSubR8->getWidth() != renderingContext->subRegion.width ||
				 currentRenderData->readerSubR8->getHeight() != renderingContext->subRegion.height));

	if (needsRecreation) {
		GraphicsContextGuard graphicsContextGuard;
		auto newRenderData = std::make_unique<DebugRenderData>();
		newRenderData->readerBgrx = std::make_unique<AsyncTextureReader>(
			renderingContext->region.width, renderingContext->region.height, GS_BGRX);
		newRenderData->readerR8 = std::make_unique<AsyncTextureReader>(renderingContext->region.width,
									       renderingContext->region.height, GS_R8);
		newRenderData->readerR32f = std::make_unique<AsyncTextureReader>(
			renderingContext->region.width, renderingContext->region.height, GS_R32F);
		newRenderData->readerMaskRoiR8 = std::make_unique<AsyncTextureReader>(
			renderingContext->maskRoi.width, renderingContext->maskRoi.height, GS_R8);
		newRenderData->reader256Bgrx = std::make_unique<AsyncTextureReader>(256, 144, GS_BGRX);
		newRenderData->readerSubR8 = std::make_unique<AsyncTextureReader>(
			renderingContext->subRegion.width, renderingContext->subRegion.height, GS_R8);
		newRenderData->readerR32fSub = std::make_unique<AsyncTextureReader>(
			renderingContext->subRegion.width, renderingContext->subRegion.height, GS_R32F);

		if (currentRenderData) {
			oldRenderData.push_back(std::move(currentRenderData));
		}

		currentRenderData = std::move(newRenderData);

		atomicRenderData.store(currentRenderData.get(), std::memory_order_release);
	}

	if (!currentRenderData) {
		return;
	}

	auto currentTexture = previewTextureSelector->currentText();
	auto currentTextureStd = currentTexture.toStdString();
	QImage image;

	try {
		GraphicsContextGuard graphicsContextGuard;
		if (std::find(bgrxTextures.begin(), bgrxTextures.end(), currentTextureStd) != bgrxTextures.end()) {
			currentRenderData->readerBgrx->sync();
			image = QImage(currentRenderData->readerBgrx->getBuffer().data(),
				       currentRenderData->readerBgrx->getWidth(),
				       currentRenderData->readerBgrx->getHeight(),
				       currentRenderData->readerBgrx->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8Textures.begin(), r8Textures.end(), currentTextureStd) != r8Textures.end()) {
			currentRenderData->readerR8->sync();
			image = QImage(currentRenderData->readerR8->getBuffer().data(),
				       currentRenderData->readerR8->getWidth(),
				       currentRenderData->readerR8->getHeight(),
				       currentRenderData->readerR8->getBufferLinesize(), QImage::Format_Grayscale8);
		} else if (std::find(r32fTextures.begin(), r32fTextures.end(), currentTextureStd) !=
			   r32fTextures.end()) {
			currentRenderData->readerR32f->sync();
			auto r32fDataView =
				reinterpret_cast<const float *>(currentRenderData->readerR32f->getBuffer().data());
			bufferR8.resize(currentRenderData->readerR32f->getWidth() *
					currentRenderData->readerR32f->getHeight());
			for (std::uint32_t i = 0;
			     i < currentRenderData->readerR32f->getWidth() * currentRenderData->readerR32f->getHeight();
			     ++i) {
				bufferR8[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferR8.data(), currentRenderData->readerR32f->getWidth(),
				       currentRenderData->readerR32f->getHeight(), QImage::Format_Grayscale8);
		} else if (std::find(bgrx256Textures.begin(), bgrx256Textures.end(), currentTextureStd) !=
			   bgrx256Textures.end()) {
			currentRenderData->reader256Bgrx->sync();
			image = QImage(currentRenderData->reader256Bgrx->getBuffer().data(),
				       currentRenderData->reader256Bgrx->getWidth(),
				       currentRenderData->reader256Bgrx->getHeight(),
				       currentRenderData->reader256Bgrx->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8MaskRoiTextures.begin(), r8MaskRoiTextures.end(), currentTextureStd) !=
			   r8MaskRoiTextures.end()) {
			currentRenderData->readerMaskRoiR8->sync();
			image = QImage(currentRenderData->readerMaskRoiR8->getBuffer().data(),
				       currentRenderData->readerMaskRoiR8->getWidth(),
				       currentRenderData->readerMaskRoiR8->getHeight(),
				       currentRenderData->readerMaskRoiR8->getBufferLinesize(),
				       QImage::Format_Grayscale8);
		} else if (std::find(r8SubTextures.begin(), r8SubTextures.end(), currentTextureStd) !=
			   r8SubTextures.end()) {
			currentRenderData->readerSubR8->sync();
			image = QImage(currentRenderData->readerSubR8->getBuffer().data(),
				       currentRenderData->readerSubR8->getWidth(),
				       currentRenderData->readerSubR8->getHeight(),
				       currentRenderData->readerSubR8->getBufferLinesize(), QImage::Format_Grayscale8);
		} else if (std::find(r32fSubTextures.begin(), r32fSubTextures.end(), currentTextureStd) !=
			   r32fSubTextures.end()) {
			currentRenderData->readerR32fSub->sync();
			auto r32fDataView =
				reinterpret_cast<const float *>(currentRenderData->readerR32fSub->getBuffer().data());
			bufferSubR8.resize(currentRenderData->readerR32fSub->getWidth() *
					   currentRenderData->readerR32fSub->getHeight());
			for (std::uint32_t i = 0; i < currentRenderData->readerR32fSub->getWidth() *
							      currentRenderData->readerR32fSub->getHeight();
			     ++i) {
				bufferSubR8[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferSubR8.data(), currentRenderData->readerR32fSub->getWidth(),
				       currentRenderData->readerR32fSub->getHeight(), QImage::Format_Grayscale8);
		}
	} catch (const std::exception &e) {
		mainPluginContext->getLogger().error("Failed to sync and update preview: {}", e.what());
		return;
	}

	if (image.isNull()) {
		return;
	}

	QPixmap pixmap = QPixmap::fromImage(image);
	QPixmap scaledPixmap =
		pixmap.scaled(PREVIEW_WIDTH, PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	previewImageLabel->setPixmap(scaledPixmap);
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
