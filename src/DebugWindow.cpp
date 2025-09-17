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
#include <QDebug>
#include <iostream>

#include "AsyncTextureReader.hpp"
#include "MainPluginContext.h"
#include "RenderingContext.hpp"

namespace {

constexpr char textureBgrxOriginalImage[] = "bgrxOriginalImage";
constexpr char textureR32fOriginalGrayscale[] = "r32fOriginalGrayscale";
constexpr char textureR32fSubOriginalGrayscale[] = "r32fSubOriginalGrayscale";
constexpr char textureR32fSubLastOriginalGrayscale[] = "r32fSubLastOriginalGrayscale";
constexpr char textureBgrxSegmenterInput[] = "bgrxSegmenterInput";
constexpr char textureR8SegmentationMask[] = "r8SegmentationMask";
constexpr char textureR8GFGuideSub[] = "r8GFGuideSub";
constexpr char textureR8GFSourceSub[] = "r8GFSourceSub";
constexpr char textureR32fGFMeanGuideSub[] = "r32fGFMeanGuideSub";
constexpr char textureR32fGFMeanSourceSub[] = "r32fGFMeanSourceSub";
constexpr char textureR32fGFMeanGuideSourceSub[] = "r16fGFMeanGuideSourceSub";
constexpr char textureR32fGFMeanGuideSqSub[] = "r32fGFMeanGuideSqSub";
constexpr char textureR32fGFASub[] = "r32fGFASub";
constexpr char textureR32fGFBSub[] = "r32fGFBSub";
constexpr char textureR8GFResult[] = "r8GFResult";

const std::vector<std::string> bgrxTextures = {textureBgrxOriginalImage};
const std::vector<std::string> r8Textures = {textureR8GFResult};
const std::vector<std::string> r32fTextures = {textureR32fOriginalGrayscale};
const std::vector<std::string> bgrx256Textures = {textureBgrxSegmenterInput};
const std::vector<std::string> r8MaskRoiTextures = {textureR8SegmentationMask};
const std::vector<std::string> subR8Textures = {textureR8GFGuideSub, textureR8GFSourceSub};
const std::vector<std::string> subR32fTextures = {
	textureR32fSubOriginalGrayscale, textureR32fSubLastOriginalGrayscale,
	textureR32fGFMeanGuideSub,   textureR32fGFMeanSourceSub, textureR32fGFMeanGuideSourceSub,
	textureR32fGFMeanGuideSqSub, textureR32fGFASub,          textureR32fGFBSub};

inline double float16_to_double(uint16_t h)
{
	const uint16_t sign_h = (h >> 15) & 0x0001;
	const uint16_t exp_h = (h >> 10) & 0x001f;
	const uint16_t frac_h = h & 0x03ff;

	uint64_t sign_d = static_cast<uint64_t>(sign_h) << 63;
	uint64_t exp_d;
	uint64_t frac_d;

	if (exp_h == 0) {
		if (frac_h == 0) {
			uint64_t result_bits = sign_d;
			double result;
			std::memcpy(&result, &result_bits, sizeof(double));
			return result;
		} else {
			int n = 0;
			uint16_t temp_frac = frac_h;
			while (!((temp_frac <<= 1) & 0x0400)) {
				n++;
			}
			exp_d = static_cast<uint64_t>(1023 - 15 - n + 1);
			frac_d = static_cast<uint64_t>((temp_frac & 0x03ff)) << 42;
		}
	} else if (exp_h == 0x1f) {
		exp_d = 0x7ff;
		frac_d = (frac_h == 0) ? 0 : (static_cast<uint64_t>(frac_h) << 42);
	} else {
		exp_d = static_cast<uint64_t>(exp_h - 15 + 1023);
		frac_d = static_cast<uint64_t>(frac_h) << 42;
	}

	uint64_t result_bits = sign_d | (exp_d << 52) | frac_d;
	double result;
	std::memcpy(&result, &result_bits, sizeof(double));
	return result;
}

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
	previewTextureSelector->addItem(textureR32fOriginalGrayscale);
	previewTextureSelector->addItem(textureR32fSubOriginalGrayscale);
	previewTextureSelector->addItem(textureR32fSubLastOriginalGrayscale);
	previewTextureSelector->addItem(textureBgrxSegmenterInput);
	previewTextureSelector->addItem(textureR8SegmentationMask);
	previewTextureSelector->addItem(textureR8GFGuideSub);
	previewTextureSelector->addItem(textureR8GFSourceSub);
	previewTextureSelector->addItem(textureR32fGFMeanGuideSub);
	previewTextureSelector->addItem(textureR32fGFMeanSourceSub);
	previewTextureSelector->addItem(textureR32fGFMeanGuideSourceSub);
	previewTextureSelector->addItem(textureR32fGFMeanGuideSqSub);
	previewTextureSelector->addItem(textureR32fGFASub);
	previewTextureSelector->addItem(textureR32fGFBSub);
	previewTextureSelector->addItem(textureR8GFResult);

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
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fOriginalGrayscale.get());
		} else if (currentTexture == textureR32fSubOriginalGrayscale) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubOriginalGrayscale.get());
		} else if (currentTexture == textureR32fSubLastOriginalGrayscale) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fSubLastOriginalGrayscale.get());
		} else if (currentTexture == textureBgrxSegmenterInput) {
			reader256Bgrx->sync();
			reader256Bgrx->stage(renderingContext->bgrxSegmenterInput.get());
		} else if (currentTexture == textureR8SegmentationMask) {
			readerMaskRoiR8->sync();
			readerMaskRoiR8->stage(renderingContext->r8SegmentationMask.get());
		} else if (currentTexture == textureR8GFGuideSub) {
			readerSubR8->sync();
			readerSubR8->stage(renderingContext->r8GFGuideSub.get());
		} else if (currentTexture == textureR8GFSourceSub) {
			readerSubR8->sync();
			readerSubR8->stage(renderingContext->r8GFSourceSub.get());
		} else if (currentTexture == textureR32fGFMeanGuideSub) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fGFMeanGuideSub.get());
		} else if (currentTexture == textureR32fGFMeanSourceSub) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fGFMeanSourceSub.get());
		} else if (currentTexture == textureR32fGFMeanGuideSourceSub) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fGFMeanGuideSourceSub.get());
		} else if (currentTexture == textureR32fGFMeanGuideSqSub) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fGFMeanGuideSqSub.get());
		} else if (currentTexture == textureR32fGFASub) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fGFASub.get());
		} else if (currentTexture == textureR32fGFBSub) {
			readerR32fSub->sync();
			readerR32fSub->stage(renderingContext->r32fGFBSub.get());
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
		auto r32fDataView = reinterpret_cast<float *>(readerR32f->getBuffer().data());
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
	} else if (std::find(subR8Textures.begin(), subR8Textures.end(), currentTextureStd) != subR8Textures.end()) {
		image = QImage(readerSubR8->getBuffer().data(), readerSubR8->width, readerSubR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(subR32fTextures.begin(), subR32fTextures.end(), currentTextureStd) !=
		   subR32fTextures.end()) {
		auto r32fDataView = reinterpret_cast<float *>(readerR32fSub->getBuffer().data());
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

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
