/*
 * Live Background Removal Lite - MainFilter Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#include "DebugWindow.hpp"

#include <iostream>

#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QDebug>

#include <AsyncTextureReader.hpp>

#include "MainFilterContext.hpp"
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
const char *textureR32fSubGFMeanGuideSource = "r32fSubGFMeanGuideSource";
const char *textureR32fSubGFMeanGuideSq = "r32fSubGFMeanGuideSq";
const char *textureR32fSubGFA = "r32fSubGFA";
const char *textureR32fSubGFB = "r32fSubGFB";
const char *textureR8GuidedFilterResult = "r8GuidedFilterResult";
const char *textureR8TimeAveragedMasks0 = "r8TimeAveragedMasks[0]";
const char *textureR8TimeAveragedMasks1 = "r8TimeAveragedMasks[1]";

const std::vector<const char *> textureNames = {
	textureBgrxSource,
	textureR32fLuma,
	textureR32fSubLumas0,
	textureR32fSubLumas1,
	textureR32fSubPaddedSquaredMotion,
	textureBgrxSegmenterInput,
	textureR8SegmentationMask,
	textureR32fSubGFSource,
	textureR32fSubGFMeanGuide,
	textureR32fSubGFMeanSource,
	textureR32fSubGFMeanGuideSource,
	textureR32fSubGFMeanGuideSq,
	textureR32fSubGFA,
	textureR32fSubGFB,
	textureR8GuidedFilterResult,
	textureR8TimeAveragedMasks0,
	textureR8TimeAveragedMasks1,
};

const std::vector<const char *> bgrxTextures = {textureBgrxSource};
const std::vector<const char *> r8Textures = {textureR8GuidedFilterResult, textureR8TimeAveragedMasks0,
					      textureR8TimeAveragedMasks1};
const std::vector<const char *> r32fTextures = {textureR32fLuma};
const std::vector<const char *> bgrxSegmenterInputTextures = {textureBgrxSegmenterInput};
const std::vector<const char *> r8MaskRoiTextures = {textureR8SegmentationMask};
const std::vector<const char *> r32fSubPaddedTextures = {textureR32fSubPaddedSquaredMotion};
const std::vector<const char *> r32fSubTextures = {
	textureR32fSubLumas0,        textureR32fSubLumas1,       textureR32fSubGFSource,
	textureR32fSubGFMeanGuide,   textureR32fSubGFMeanSource, textureR32fSubGFMeanGuideSource,
	textureR32fSubGFMeanGuideSq, textureR32fSubGFA,          textureR32fSubGFB};

} // namespace

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

DebugWindow::DebugWindow(std::weak_ptr<MainPluginContext> weakMainPluginContext, QWidget *parent)
	: QDialog(parent),
	  weakMainPluginContext_(std::move(weakMainPluginContext)),
	  layout_(new QVBoxLayout(this)),
	  previewTextureSelector_(new QComboBox(this)),
	  previewImageLabel_(new QLabel(this)),
	  updateTimer_(new QTimer(this))
{
	for (const auto &textureName : textureNames) {
		previewTextureSelector_->addItem(textureName);
	}

	connect(previewTextureSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&DebugWindow::onTextureSelectionChanged);

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

	auto selectedPreviewTextureIndex = selectedPreviewTextureIndex_.load(std::memory_order_acquire);
	if (selectedPreviewTextureIndex >= static_cast<int>(textureNames.size())) {
		logger.warn("DebugWindow::videoRender: selectedPreviewTextureIndex out of bounds");
		return;
	}
	auto selectedPreviewTextureName = textureNames[selectedPreviewTextureIndex];

	std::shared_ptr<AsyncTextureReader> currentReader;
	gs_texture_t *currentTexture;
	{
		std::lock_guard<std::mutex> lock(readerMutex_);
		if (selectedPreviewTextureName == textureBgrxSource) {
			currentReader = bgrxReader_;
			currentTexture = renderingContext->bgrxSource_.get();
		} else if (selectedPreviewTextureName == textureR32fLuma) {
			currentReader = r32fReader_;
			currentTexture = renderingContext->r32fLuma_.get();
		} else if (selectedPreviewTextureName == textureR32fSubLumas0) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubLumas_[0].get();
		} else if (selectedPreviewTextureName == textureR32fSubLumas1) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubLumas_[1].get();
		} else if (selectedPreviewTextureName == textureR32fSubPaddedSquaredMotion) {
			currentReader = r32fSubPaddedReader_;
			currentTexture = renderingContext->r32fSubPaddedSquaredMotion_.get();
		} else if (selectedPreviewTextureName == textureBgrxSegmenterInput) {
			currentReader = bgrxSegmenterInputReader_;
			currentTexture = renderingContext->bgrxSegmenterInput_.get();
		} else if (selectedPreviewTextureName == textureR8SegmentationMask) {
			currentReader = r8MaskRoiReader_;
			currentTexture = renderingContext->r8SegmentationMask_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFSource) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFSource_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFMeanGuide) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFMeanGuide_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFMeanSource) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFMeanSource_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFMeanGuideSource) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFMeanGuideSource_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFMeanGuideSq) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFMeanGuideSq_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFA) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFA_.get();
		} else if (selectedPreviewTextureName == textureR32fSubGFB) {
			currentReader = r32fSubReader_;
			currentTexture = renderingContext->r32fSubGFB_.get();
		} else if (selectedPreviewTextureName == textureR8GuidedFilterResult) {
			currentReader = r8Reader_;
			currentTexture = renderingContext->r8GuidedFilterResult_.get();
		} else if (selectedPreviewTextureName == textureR8TimeAveragedMasks0) {
			currentReader = r8Reader_;
			currentTexture = renderingContext->r8TimeAveragedMasks_[0].get();
		} else if (selectedPreviewTextureName == textureR8TimeAveragedMasks1) {
			currentReader = r8Reader_;
			currentTexture = renderingContext->r8TimeAveragedMasks_[1].get();
		} else {
			logger.warn("DebugWindow::videoRender: Unknown texture selected: {}",
				    selectedPreviewTextureName);
			return;
		}
	}

	if (currentReader && currentTexture) {
		currentReader->stage(currentTexture);
	}
}

inline bool checkIfReaderNeedsRecreation(const std::shared_ptr<AsyncTextureReader> &reader, std::uint32_t width,
					 std::uint32_t height)
{
	return !reader || reader->getWidth() != width || reader->getHeight() != height;
}

void DebugWindow::updatePreview()
{
	auto mainPluginContext = weakMainPluginContext_.lock();
	if (!mainPluginContext) {
		blog(LOG_WARNING, "[" PLUGIN_NAME "] DebugWindow::updatePreview: MainPluginContext is null");
		return;
	}

	auto &logger = mainPluginContext->getLogger();

	auto renderingContext = mainPluginContext->getRenderingContext();
	if (!renderingContext) {
		logger.warn("DebugWindow::updatePreview: RenderingContext is null");
		return;
	}

	std::shared_ptr<AsyncTextureReader> bgrxReader;
	std::shared_ptr<AsyncTextureReader> r8Reader;
	std::shared_ptr<AsyncTextureReader> r32fReader;
	std::shared_ptr<AsyncTextureReader> bgrxSegmenterInputReader;
	std::shared_ptr<AsyncTextureReader> r8MaskRoiReader;
	std::shared_ptr<AsyncTextureReader> r32fSubReader;
	std::shared_ptr<AsyncTextureReader> r32fSubPaddedReader;

	{
		GraphicsContextGuard graphicsContextGuard;
		std::lock_guard<std::mutex> lock(readerMutex_);

		if (checkIfReaderNeedsRecreation(bgrxReader_, renderingContext->getWidth(),
						 renderingContext->getHeight())) {
			auto bgrxReader = std::make_shared<AsyncTextureReader>(renderingContext->getWidth(),
									       renderingContext->getHeight(), GS_BGRX);
			bgrxReader_ = bgrxReader;
		}

		if (checkIfReaderNeedsRecreation(r8Reader_, renderingContext->getWidth(),
						 renderingContext->getHeight())) {
			auto r8Reader = std::make_shared<AsyncTextureReader>(renderingContext->getWidth(),
									     renderingContext->getHeight(), GS_R8);
			r8Reader_ = r8Reader;
		}

		if (checkIfReaderNeedsRecreation(r32fReader_, renderingContext->getWidth(),
						 renderingContext->getHeight())) {
			auto r32fReader = std::make_shared<AsyncTextureReader>(renderingContext->getWidth(),
									       renderingContext->getHeight(), GS_R32F);
			r32fReader_ = r32fReader;
		}

		if (checkIfReaderNeedsRecreation(
			    bgrxSegmenterInputReader_,
			    static_cast<std::uint32_t>(renderingContext->selfieSegmenter_->getWidth()),
			    static_cast<std::uint32_t>(renderingContext->selfieSegmenter_->getHeight()))) {
			auto bgrxSegmenterInputReader = std::make_shared<AsyncTextureReader>(
				static_cast<std::uint32_t>(renderingContext->selfieSegmenter_->getWidth()),
				static_cast<std::uint32_t>(renderingContext->selfieSegmenter_->getHeight()), GS_BGRX);
			bgrxSegmenterInputReader_ = bgrxSegmenterInputReader;
		}

		if (checkIfReaderNeedsRecreation(r8MaskRoiReader_, renderingContext->maskRoi_.width,
						 renderingContext->maskRoi_.height)) {
			auto r8MaskRoiReader = std::make_shared<AsyncTextureReader>(
				renderingContext->maskRoi_.width, renderingContext->maskRoi_.height, GS_R8);
			r8MaskRoiReader_ = r8MaskRoiReader;
		}

		if (checkIfReaderNeedsRecreation(r8SubR8Reader_, renderingContext->subRegion_.width,
						 renderingContext->subRegion_.height)) {
			auto r8SubR8Reader = std::make_shared<AsyncTextureReader>(
				renderingContext->subRegion_.width, renderingContext->subRegion_.height, GS_R8);
			r8SubR8Reader_ = r8SubR8Reader;
		}

		if (checkIfReaderNeedsRecreation(r32fSubReader_, renderingContext->subRegion_.width,
						 renderingContext->subRegion_.height)) {
			auto r32fSubReader = std::make_shared<AsyncTextureReader>(
				renderingContext->subRegion_.width, renderingContext->subRegion_.height, GS_R32F);
			r32fSubReader_ = r32fSubReader;
		}

		if (checkIfReaderNeedsRecreation(r32fSubPaddedReader_, renderingContext->subPaddedRegion_.width,
						 renderingContext->subPaddedRegion_.height)) {
			auto r32fSubPaddedReader = std::make_shared<AsyncTextureReader>(
				renderingContext->subPaddedRegion_.width, renderingContext->subPaddedRegion_.height,
				GS_R32F);
			r32fSubPaddedReader_ = r32fSubPaddedReader;
		}

		bgrxReader = bgrxReader_;
		r8Reader = r8Reader_;
		r32fReader = r32fReader_;
		bgrxSegmenterInputReader = bgrxSegmenterInputReader_;
		r8MaskRoiReader = r8MaskRoiReader_;
		r32fSubReader = r32fSubReader_;
		r32fSubPaddedReader = r32fSubPaddedReader_;
	}

	auto selectedPreviewTextureIndex = selectedPreviewTextureIndex_.load(std::memory_order_acquire);
	if (selectedPreviewTextureIndex >= static_cast<int>(textureNames.size())) {
		return;
	}
	auto selectedPreviewTextureName = textureNames[selectedPreviewTextureIndex];

	QImage image;
	try {
		if (std::find(bgrxTextures.begin(), bgrxTextures.end(), selectedPreviewTextureName) !=
		    bgrxTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				bgrxReader->sync();
			}
			image = QImage(bgrxReader->getBuffer().data(), bgrxReader->getWidth(), bgrxReader->getHeight(),
				       bgrxReader->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8Textures.begin(), r8Textures.end(), selectedPreviewTextureName) !=
			   r8Textures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r8Reader->sync();
			}
			image = QImage(r8Reader->getBuffer().data(), r8Reader->getWidth(), r8Reader->getHeight(),
				       r8Reader->getBufferLinesize(), QImage::Format_Grayscale8);
		} else if (std::find(r32fTextures.begin(), r32fTextures.end(), selectedPreviewTextureName) !=
			   r32fTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r32fReader->sync();
			}
			auto r32fDataView = reinterpret_cast<const float *>(r32fReader->getBuffer().data());
			bufferR8_.resize(r32fReader->getWidth() * r32fReader->getHeight());
			for (std::uint32_t i = 0; i < r32fReader->getWidth() * r32fReader->getHeight(); ++i) {
				bufferR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferR8_.data(), r32fReader->getWidth(), r32fReader->getHeight(),
				       QImage::Format_Grayscale8);
		} else if (std::find(bgrxSegmenterInputTextures.begin(), bgrxSegmenterInputTextures.end(),
				     selectedPreviewTextureName) != bgrxSegmenterInputTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				bgrxSegmenterInputReader->sync();
			}
			image = QImage(bgrxSegmenterInputReader->getBuffer().data(),
				       bgrxSegmenterInputReader->getWidth(), bgrxSegmenterInputReader->getHeight(),
				       bgrxSegmenterInputReader->getBufferLinesize(), QImage::Format_RGB32);
		} else if (std::find(r8MaskRoiTextures.begin(), r8MaskRoiTextures.end(), selectedPreviewTextureName) !=
			   r8MaskRoiTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r8MaskRoiReader->sync();
			}
			image = QImage(r8MaskRoiReader->getBuffer().data(), r8MaskRoiReader->getWidth(),
				       r8MaskRoiReader->getHeight(), r8MaskRoiReader->getBufferLinesize(),
				       QImage::Format_Grayscale8);
		} else if (std::find(r32fSubTextures.begin(), r32fSubTextures.end(), selectedPreviewTextureName) !=
			   r32fSubTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r32fSubReader->sync();
			}
			auto r32fDataView = reinterpret_cast<const float *>(r32fSubReader->getBuffer().data());
			bufferSubR8_.resize(r32fSubReader->getWidth() * r32fSubReader->getHeight());
			for (std::uint32_t i = 0; i < r32fSubReader->getWidth() * r32fSubReader->getHeight(); ++i) {
				bufferSubR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferSubR8_.data(), r32fSubReader->getWidth(), r32fSubReader->getHeight(),
				       QImage::Format_Grayscale8);
		} else if (std::find(r32fSubPaddedTextures.begin(), r32fSubPaddedTextures.end(),
				     selectedPreviewTextureName) != r32fSubPaddedTextures.end()) {
			{
				GraphicsContextGuard graphicsContextGuard;
				r32fSubPaddedReader->sync();
			}
			auto r32fDataView = reinterpret_cast<const float *>(r32fSubPaddedReader->getBuffer().data());
			bufferSubPaddedR8_.resize(r32fSubPaddedReader->getWidth() * r32fSubPaddedReader->getHeight());
			for (std::uint32_t i = 0;
			     i < r32fSubPaddedReader->getWidth() * r32fSubPaddedReader->getHeight(); ++i) {
				bufferSubPaddedR8_[i] = static_cast<std::uint8_t>((r32fDataView[i]) * 255);
			}
			image = QImage(bufferSubPaddedR8_.data(), r32fSubPaddedReader->getWidth(),
				       r32fSubPaddedReader->getHeight(), QImage::Format_Grayscale8);
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

void DebugWindow::onTextureSelectionChanged(int index)
{
	selectedPreviewTextureIndex_.store(index, std::memory_order_release);
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
