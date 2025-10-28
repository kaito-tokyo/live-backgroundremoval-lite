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

#include <AsyncTextureReader.hpp>

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

DebugWindow::DebugWindow(std::weak_ptr<MainPluginContext> weakMainPluginContext, QWidget *parent)
	: QDialog(parent),
	  weakMainPluginContext_(std::move(weakMainPluginContext)),
	  layout_(new QVBoxLayout(this)),
	  previewTextureSelector_(new QComboBox(this)),
	  previewImageLabel_(new QLabel(this)),
	  updateTimer_(new QTimer(this))
{
	previewTextureSelector_->addItem(textureBgrxOriginalImage);
	previewTextureSelector_->addItem(textureR32fOriginalGrayscale);
	previewTextureSelector_->addItem(textureBgrxSegmenterInput);
	previewTextureSelector_->addItem(textureR8SegmentationMask);
	previewTextureSelector_->addItem(textureR8SubGFGuide);
	previewTextureSelector_->addItem(textureR8SubGFSource);
	previewTextureSelector_->addItem(textureR32fSubGFMeanGuide);
	previewTextureSelector_->addItem(textureR32fSubGFMeanSource);
	previewTextureSelector_->addItem(textureR16fSubGFMeanGuideSource);
	previewTextureSelector_->addItem(textureR32fSubGFMeanGuideSq);
	previewTextureSelector_->addItem(textureR32fSubGFA);
	previewTextureSelector_->addItem(textureR32fSubGFB);
	previewTextureSelector_->addItem(textureR8GuidedFilterResult);
	previewTextureSelector_->addItem(textureR8TimeAveragedMasks0);
	previewTextureSelector_->addItem(textureR8TimeAveragedMasks1);

	layout_->addWidget(previewTextureSelector_);

	previewImageLabel_->setText("No image");
	previewImageLabel_->setMaximumSize(kPreviewWidth, kPreviewHeight);

	layout_->addWidget(previewImageLabel_);

	setLayout(layout_);

	connect(updateTimer_, &QTimer::timeout, this, &DebugWindow::updatePreview);
	updateTimer_->start(1000 / 60);

	connect(this, &DebugWindow::readerReady, this, &DebugWindow::updatePreview);
}

void DebugWindow::videoRender()
{
	DebugRenderData *renderData = atomicRenderData_.load(std::memory_order_acquire);
	if (!renderData) {
		return;
	}

	if (auto mainPluginContext = weakMainPluginContext_.lock()) {
		auto renderingContext = mainPluginContext->getRenderingContext();
		if (!renderingContext) {
			return;
		}

		auto currentTexture = previewTextureSelector_->currentText();
		if (currentTexture == textureBgrxOriginalImage) {
			if (renderData->readerBgrx)
				renderData->readerBgrx->stage(renderingContext->bgrxSource_);
		} else if (currentTexture == textureR32fOriginalGrayscale) {
			if (renderData->readerR32f)
				renderData->readerR32f->stage(renderingContext->r32fGrayscale_);
		} else if (currentTexture == textureBgrxSegmenterInput) {
			if (renderData->reader256Bgrx)
				renderData->reader256Bgrx->stage(renderingContext->bgrxSegmenterInput_);
		} else if (currentTexture == textureR8SegmentationMask) {
			if (renderData->readerMaskRoiR8)
				renderData->readerMaskRoiR8->stage(renderingContext->r8SegmentationMask_);
		} else if (currentTexture == textureR8SubGFGuide) {
			if (renderData->readerSubR8)
				renderData->readerSubR8->stage(renderingContext->r8SubGFGuide_);
		} else if (currentTexture == textureR8SubGFSource) {
			if (renderData->readerSubR8)
				renderData->readerSubR8->stage(renderingContext->r8SubGFSource_);
		} else if (currentTexture == textureR32fSubGFMeanGuide) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanGuide_);
		} else if (currentTexture == textureR32fSubGFMeanSource) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanSource_);
		} else if (currentTexture == textureR16fSubGFMeanGuideSource) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanGuideSource_);
		} else if (currentTexture == textureR32fSubGFMeanGuideSq) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFMeanGuideSq_);
		} else if (currentTexture == textureR32fSubGFA) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFA_);
		} else if (currentTexture == textureR32fSubGFB) {
			if (renderData->readerR32fSub)
				renderData->readerR32fSub->stage(renderingContext->r32fSubGFB_);
		} else if (currentTexture == textureR8GuidedFilterResult) {
			if (renderData->readerR8)
				renderData->readerR8->stage(renderingContext->r8GuidedFilterResult_);
		} else if (currentTexture == textureR8TimeAveragedMasks0) {
			if (renderData->readerR8)
				renderData->readerR8->stage(renderingContext->r8TimeAveragedMasks_[0]);
		} else if (currentTexture == textureR8TimeAveragedMasks1) {
			if (renderData->readerR8)
				renderData->readerR8->stage(renderingContext->r8TimeAveragedMasks_[1]);
		}
	}
}

void DebugWindow::updatePreview()
{
	oldRenderData_.clear();

	auto mainPluginContext = weakMainPluginContext_.lock();
	if (!mainPluginContext) {
		return;
	}

	auto renderingContext = mainPluginContext->getRenderingContext();
	if (!renderingContext) {
		previewImageLabel_->setText("No rendering context");
		if (currentRenderData_) {
			oldRenderData_.push_back(std::move(currentRenderData_));
			atomicRenderData_.store(nullptr, std::memory_order_release);
		}
		return;
	}

	bool needsRecreation = !currentRenderData_ ||
			       (currentRenderData_->readerBgrx &&
				(currentRenderData_->readerBgrx->getWidth() != renderingContext->region_.width ||
				 currentRenderData_->readerBgrx->getHeight() != renderingContext->region_.height)) ||
			       (currentRenderData_->readerSubR8 &&
				(currentRenderData_->readerSubR8->getWidth() != renderingContext->subRegion_.width ||
				 currentRenderData_->readerSubR8->getHeight() != renderingContext->subRegion_.height));

	if (needsRecreation) {
		GraphicsContextGuard graphicsContextGuard;
		auto newRenderData = std::make_unique<DebugRenderData>();
		newRenderData->readerBgrx = std::make_unique<AsyncTextureReader>(
			renderingContext->region_.width, renderingContext->region_.height, GS_BGRX);
		newRenderData->readerR8 = std::make_unique<AsyncTextureReader>(renderingContext->region_.width,
									       renderingContext->region_.height, GS_R8);
		newRenderData->readerR32f = std::make_unique<AsyncTextureReader>(
			renderingContext->region_.width, renderingContext->region_.height, GS_R32F);
		newRenderData->readerMaskRoiR8 = std::make_unique<AsyncTextureReader>(
			renderingContext->maskRoi_.width, renderingContext->maskRoi_.height, GS_R8);
		newRenderData->reader256Bgrx = std::make_unique<AsyncTextureReader>(256, 144, GS_BGRX);
		newRenderData->readerSubR8 = std::make_unique<AsyncTextureReader>(
			renderingContext->subRegion_.width, renderingContext->subRegion_.height, GS_R8);
		newRenderData->readerR32fSub = std::make_unique<AsyncTextureReader>(
			renderingContext->subRegion_.width, renderingContext->subRegion_.height, GS_R32F);

		if (currentRenderData_) {
			oldRenderData_.push_back(std::move(currentRenderData_));
		}

		currentRenderData_ = std::move(newRenderData);

		atomicRenderData_.store(currentRenderData_.get(), std::memory_order_release);
	}

	if (!currentRenderData_) {
		return;
	}

	auto currentTexture = previewTextureSelector_->currentText();
	auto currentTextureStd = currentTexture.toStdString();
	QImage image;

	try {
		GraphicsContextGuard graphicsContextGuard;
		if (std::find(bgrxTextures.begin(), bgrxTextures.end(), currentTextureStd) != bgrxTextures.end()) {
			currentRenderData_->readerBgrx->sync();
			image = QImage(currentRenderData_->readerBgrx->getBuffer().data(),
				       currentRenderData_->readerBgrx->getWidth(),
				       currentRenderData_->readerBgrx->getHeight(),
				       currentRenderData_->readerBgrx->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8Textures.begin(), r8Textures.end(), currentTextureStd) != r8Textures.end()) {
			currentRenderData_->readerR8->sync();
			image = QImage(currentRenderData_->readerR8->getBuffer().data(),
				       currentRenderData_->readerR8->getWidth(),
				       currentRenderData_->readerR8->getHeight(),
				       currentRenderData_->readerR8->getBufferLinesize(), QImage::Format_Grayscale8);
		} else if (std::find(r32fTextures.begin(), r32fTextures.end(), currentTextureStd) !=
			   r32fTextures.end()) {
			currentRenderData_->readerR32f->sync();
			auto r32fDataView =
				reinterpret_cast<const float *>(currentRenderData_->readerR32f->getBuffer().data());
			bufferR8_.resize(currentRenderData_->readerR32f->getWidth() *
					 currentRenderData_->readerR32f->getHeight());
			for (std::uint32_t i = 0; i < currentRenderData_->readerR32f->getWidth() *
							      currentRenderData_->readerR32f->getHeight();
			     ++i) {
				bufferR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferR8_.data(), currentRenderData_->readerR32f->getWidth(),
				       currentRenderData_->readerR32f->getHeight(), QImage::Format_Grayscale8);
		} else if (std::find(bgrx256Textures.begin(), bgrx256Textures.end(), currentTextureStd) !=
			   bgrx256Textures.end()) {
			currentRenderData_->reader256Bgrx->sync();
			image = QImage(currentRenderData_->reader256Bgrx->getBuffer().data(),
				       currentRenderData_->reader256Bgrx->getWidth(),
				       currentRenderData_->reader256Bgrx->getHeight(),
				       currentRenderData_->reader256Bgrx->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8MaskRoiTextures.begin(), r8MaskRoiTextures.end(), currentTextureStd) !=
			   r8MaskRoiTextures.end()) {
			currentRenderData_->readerMaskRoiR8->sync();
			image = QImage(currentRenderData_->readerMaskRoiR8->getBuffer().data(),
				       currentRenderData_->readerMaskRoiR8->getWidth(),
				       currentRenderData_->readerMaskRoiR8->getHeight(),
				       currentRenderData_->readerMaskRoiR8->getBufferLinesize(),
				       QImage::Format_Grayscale8);
		} else if (std::find(r8SubTextures.begin(), r8SubTextures.end(), currentTextureStd) !=
			   r8SubTextures.end()) {
			currentRenderData_->readerSubR8->sync();
			image = QImage(currentRenderData_->readerSubR8->getBuffer().data(),
				       currentRenderData_->readerSubR8->getWidth(),
				       currentRenderData_->readerSubR8->getHeight(),
				       currentRenderData_->readerSubR8->getBufferLinesize(), QImage::Format_Grayscale8);
		} else if (std::find(r32fSubTextures.begin(), r32fSubTextures.end(), currentTextureStd) !=
			   r32fSubTextures.end()) {
			currentRenderData_->readerR32fSub->sync();
			auto r32fDataView =
				reinterpret_cast<const float *>(currentRenderData_->readerR32fSub->getBuffer().data());
			bufferSubR8_.resize(currentRenderData_->readerR32fSub->getWidth() *
					    currentRenderData_->readerR32fSub->getHeight());
			for (std::uint32_t i = 0; i < currentRenderData_->readerR32fSub->getWidth() *
							      currentRenderData_->readerR32fSub->getHeight();
			     ++i) {
				bufferSubR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferSubR8_.data(), currentRenderData_->readerR32fSub->getWidth(),
				       currentRenderData_->readerR32fSub->getHeight(), QImage::Format_Grayscale8);
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
		pixmap.scaled(kPreviewWidth, kPreviewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	previewImageLabel_->setPixmap(scaledPixmap);
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
