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

const char *textureBgrxSource = "bgrxSource";
const char *textureR32fLuma = "r32fLuma";
const char *textureR32fSubLumas0 = "r32fSubLumas[0]";
const char *textureR32fSubLumas1 = "r32fSubLumas[1]";
const char *textureR32fSubPaddedSquaredMotion = "r32fSubPaddedSquaredMotion";
const char *textureBgrxSegmenterInput = "bgrxSegmenterInput";
const char *textureR8SegmentationMask = "r8SegmentationMask";
const char *textureR32fSubGFSource = "r32fSubGFSource";
const char *textureR32fSubGFMeanGuide = "r32fSubGFMeanGuide";
const char *textureR32fSubGFMeanSource = "r32fSubGFMeanSource";
const char *textureR16fSubGFMeanGuideSource = "r16fSubGFMeanGuideSource";
const char *textureR32fSubGFMeanGuideSq = "r32fSubGFMeanGuideSq";
const char *textureR32fSubGFA = "r32fSubGFA";
const char *textureR32fSubGFB = "r32fSubGFB";
const char *textureR8GuidedFilterResult = "r8GuidedFilterResult";
const char *textureR8TimeAveragedMasks0 = "r8TimeAveragedMasks[0]";
const char *textureR8TimeAveragedMasks1 = "r8TimeAveragedMasks[1]";

const std::vector<std::string> bgrxTextures = {textureBgrxSource};
const std::vector<std::string> r8Textures = {textureR8GuidedFilterResult, textureR8TimeAveragedMasks0,
					     textureR8TimeAveragedMasks1};
const std::vector<std::string> r32fTextures = {textureR32fLuma};
const std::vector<std::string> bgrxSegmenterInputTextures = {textureBgrxSegmenterInput};
const std::vector<std::string> r8MaskRoiTextures = {textureR8SegmentationMask};
const std::vector<std::string> r32fSubPaddedTextures = {textureR32fSubPaddedSquaredMotion};
const std::vector<std::string> r32fSubTextures = {
	textureR32fSubLumas0,        textureR32fSubLumas1,       textureR32fSubGFSource,
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
	previewTextureSelector_->addItem(textureBgrxSource);
	previewTextureSelector_->addItem(textureR32fLuma);
	previewTextureSelector_->addItem(textureR32fSubLumas0);
	previewTextureSelector_->addItem(textureR32fSubLumas1);
	previewTextureSelector_->addItem(textureR32fSubPaddedSquaredMotion);
	previewTextureSelector_->addItem(textureBgrxSegmenterInput);
	previewTextureSelector_->addItem(textureR8SegmentationMask);
	previewTextureSelector_->addItem(textureR32fSubGFSource);
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
	updateTimer_->start(1000 / 15);
}

void DebugWindow::videoRender()
{
	auto mainPluginContext = weakMainPluginContext_.lock();
	if (!mainPluginContext) {
		blog(LOG_WARNING, "[" PLUGIN_NAME "] DebugWindow::videoRender: MainPluginContext is null");
		return;
	}

	auto &logger = mainPluginContext->getLogger();

	auto renderingContext = mainPluginContext->getRenderingContext();
	if (!renderingContext) {
		logger.warn("DebugWindow::videoRender: RenderingContext is null");
		return;
	}

	auto selectedTexture = previewTextureSelector_->currentText();

	if (selectedTexture == textureBgrxSource && bgrxReader_) {
		bgrxReader_->stage(renderingContext->bgrxSource_);
	} else if (selectedTexture == textureR32fLuma && r32fReader_) {
		r32fReader_->stage(renderingContext->r32fLuma_);
	} else if (selectedTexture == textureR32fSubLumas0 && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubLumas_[0]);
	} else if (selectedTexture == textureR32fSubLumas1 && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubLumas_[1]);
	} else if (selectedTexture == textureR32fSubPaddedSquaredMotion && r32fSubPaddedReader_) {
		r32fSubPaddedReader_->stage(renderingContext->r32fSubPaddedSquaredMotion_);
	} else if (selectedTexture == textureBgrxSegmenterInput && bgrxSegmenterInputReader_) {
		bgrxSegmenterInputReader_->stage(renderingContext->bgrxSegmenterInput_);
	} else if (selectedTexture == textureR8SegmentationMask && r8MaskRoiReader_) {
		r8MaskRoiReader_->stage(renderingContext->r8SegmentationMask_);
	} else if (selectedTexture == textureR32fSubGFSource && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFSource_);
	} else if (selectedTexture == textureR32fSubGFMeanGuide && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFMeanGuide_);
	} else if (selectedTexture == textureR32fSubGFMeanSource && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFMeanSource_);
	} else if (selectedTexture == textureR16fSubGFMeanGuideSource && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFMeanGuideSource_);
	} else if (selectedTexture == textureR32fSubGFMeanGuideSq && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFMeanGuideSq_);
	} else if (selectedTexture == textureR32fSubGFA && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFA_);
	} else if (selectedTexture == textureR32fSubGFB && r32fSubReader_) {
		r32fSubReader_->stage(renderingContext->r32fSubGFB_);
	} else if (selectedTexture == textureR8GuidedFilterResult && r8Reader_) {
		r8Reader_->stage(renderingContext->r8GuidedFilterResult_);
	} else if (selectedTexture == textureR8TimeAveragedMasks0 && r8Reader_) {
		r8Reader_->stage(renderingContext->r8TimeAveragedMasks_[0]);
	} else if (selectedTexture == textureR8TimeAveragedMasks1 && r8Reader_) {
		r8Reader_->stage(renderingContext->r8TimeAveragedMasks_[1]);
	} else {
		logger.warn("DebugWindow::videoRender: Unknown texture selected: {}", selectedTexture.toStdString());
		return;
	}
}

inline bool checkIfReaderNeedsRecreation(const std::unique_ptr<AsyncTextureReader> &reader, std::uint32_t width, std::uint32_t height)
{
	return !reader || reader->getWidth() != width || reader->getHeight() != height;
}

void DebugWindow::updatePreview()
{
	auto mainPluginContext = weakMainPluginContext_.lock();
	if (!mainPluginContext) {
		blog(LOG_WARNING, "[" PLUGIN_NAME "] DebugWindow::videoRender: MainPluginContext is null");
		return;
	}

	auto &logger = mainPluginContext->getLogger();

	auto renderingContext = mainPluginContext->getRenderingContext();
	if (!renderingContext) {
		logger.warn("DebugWindow::videoRender: RenderingContext is null");
		return;
	}

	logger.info("aaa");

	{
		GraphicsContextGuard graphicsContextGuard;

		if (checkIfReaderNeedsRecreation(bgrxReader_, renderingContext->getWidth(), renderingContext->getHeight())) {
			bgrxReader_ = std::make_unique<AsyncTextureReader>(renderingContext->getWidth(), renderingContext->getHeight(), GS_BGRX);
		}

		if (checkIfReaderNeedsRecreation(r8Reader_, renderingContext->getWidth(), renderingContext->getHeight())) {
			r8Reader_ = std::make_unique<AsyncTextureReader>(renderingContext->getWidth(), renderingContext->getHeight(), GS_R8);
		}

		if (checkIfReaderNeedsRecreation(r32fReader_, renderingContext->getWidth(), renderingContext->getHeight())) {
			r32fReader_ =  std::make_unique<AsyncTextureReader>(renderingContext->getWidth(), renderingContext->getHeight(), GS_R32F);
		}

		if (checkIfReaderNeedsRecreation(bgrxSegmenterInputReader_, renderingContext->selfieSegmenter_->getWidth(), renderingContext->selfieSegmenter_->getHeight())) {
			bgrxSegmenterInputReader_ = std::make_unique<AsyncTextureReader>(renderingContext->selfieSegmenter_->getWidth(), renderingContext->selfieSegmenter_->getHeight(), GS_BGRX);
		}

		if (checkIfReaderNeedsRecreation(r8MaskRoiReader_, renderingContext->maskRoi_.width, renderingContext->maskRoi_.height)) {
			r8MaskRoiReader_ = std::make_unique<AsyncTextureReader>(renderingContext->maskRoi_.width, renderingContext->maskRoi_.height, GS_R8);
		}

		if (checkIfReaderNeedsRecreation(r8SubR8Reader_, renderingContext->subRegion_.width, renderingContext->subRegion_.height)) {
			r8SubR8Reader_ = std::make_unique<AsyncTextureReader>(renderingContext->subRegion_.width, renderingContext->subRegion_.height, GS_R8);
		}

		if (checkIfReaderNeedsRecreation(r32fSubReader_, renderingContext->subRegion_.width, renderingContext->subRegion_.height)) {
			r32fSubReader_ = std::make_unique<AsyncTextureReader>(renderingContext->subRegion_.width, renderingContext->subRegion_.height, GS_R32F);
		}

		if (checkIfReaderNeedsRecreation(r32fSubPaddedReader_, renderingContext->subPaddedRegion_.width, renderingContext->subPaddedRegion_.height)) {
			r32fSubPaddedReader_ = std::make_unique<AsyncTextureReader>(renderingContext->subPaddedRegion_.width, renderingContext->subPaddedRegion_.height, GS_R32F);
		}
	}

	auto currentTexture = previewTextureSelector_->currentText();
	auto currentTextureStd = currentTexture.toStdString();
	QImage image;
	try {
		if (std::find(bgrxTextures.begin(), bgrxTextures.end(), currentTextureStd) != bgrxTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				bgrxReader_->sync();
			}
			image = QImage(bgrxReader_->getBuffer().data(),
				       bgrxReader_->getWidth(),
				       bgrxReader_->getHeight(),
				       bgrxReader_->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8Textures.begin(), r8Textures.end(), currentTextureStd) != r8Textures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r8Reader_->sync();
			}
			image = QImage(r8Reader_->getBuffer().data(),
				       r8Reader_->getWidth(),
				       r8Reader_->getHeight(),
				       r8Reader_->getBufferLinesize(), QImage::Format_Grayscale8);
		} else if (std::find(r32fTextures.begin(), r32fTextures.end(), currentTextureStd) !=
			   r32fTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r32fReader_->sync();
			}
			auto r32fDataView =
				reinterpret_cast<const float *>(r32fReader_->getBuffer().data());
			bufferR8_.resize(r32fReader_->getWidth() *
					 r32fReader_->getHeight());
			for (std::uint32_t i = 0; i < r32fReader_->getWidth() *
							      r32fReader_->getHeight();
			     ++i) {
				bufferR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferR8_.data(), r32fReader_->getWidth(),
				       r32fReader_->getHeight(), QImage::Format_Grayscale8);
		} else if (std::find(bgrxSegmenterInputTextures.begin(), bgrxSegmenterInputTextures.end(), currentTextureStd) !=
			   bgrxSegmenterInputTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				bgrxSegmenterInputReader_->sync();
			}
			image = QImage(bgrxSegmenterInputReader_->getBuffer().data(),
				       bgrxSegmenterInputReader_->getWidth(),
				       bgrxSegmenterInputReader_->getHeight(),
				       bgrxSegmenterInputReader_->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8MaskRoiTextures.begin(), r8MaskRoiTextures.end(), currentTextureStd) !=
			   r8MaskRoiTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r8MaskRoiReader_->sync();
			}
			image = QImage(r8MaskRoiReader_->getBuffer().data(),
				       r8MaskRoiReader_->getWidth(),
				       r8MaskRoiReader_->getHeight(),
				       r8MaskRoiReader_->getBufferLinesize(),
				       QImage::Format_Grayscale8);
		} else if (std::find(r32fSubTextures.begin(), r32fSubTextures.end(), currentTextureStd) !=
			   r32fSubTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r32fSubReader_->sync();
			}
			auto r32fDataView =
				reinterpret_cast<const float *>(r32fSubReader_->getBuffer().data());
			bufferSubR8_.resize(r32fSubReader_->getWidth() *
					    r32fSubReader_->getHeight());
			for (std::uint32_t i = 0; i < r32fSubReader_->getWidth() *
							      r32fSubReader_->getHeight();
			     ++i) {
				bufferSubR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferSubR8_.data(), r32fSubReader_->getWidth(),
				       r32fSubReader_->getHeight(), QImage::Format_Grayscale8);
		} else if (std::find(r32fSubPaddedTextures.begin(), r32fSubPaddedTextures.end(), currentTextureStd) !=
			   r32fSubPaddedTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r32fSubPaddedReader_->sync();
			}
			auto r32fDataView = reinterpret_cast<const float *>(
				r32fSubPaddedReader_->getBuffer().data());
			bufferSubPaddedR8_.resize(r32fSubPaddedReader_->getWidth() *
						  r32fSubPaddedReader_->getHeight());
			for (std::uint32_t i = 0; i < r32fSubPaddedReader_->getWidth() *
							      r32fSubPaddedReader_->getHeight();
			     ++i) {
				bufferSubPaddedR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferSubPaddedR8_.data(), r32fSubPaddedReader_->getWidth(),
				       r32fSubPaddedReader_->getHeight(), QImage::Format_Grayscale8);
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
