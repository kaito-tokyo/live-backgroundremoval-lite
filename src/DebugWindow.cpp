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
constexpr char textureR8OriginalGrayscale[] = "r8OriginalGrayscale";
constexpr char textureBgrxSegmenterInput[] = "bgrxSegmenterInput";
constexpr char textureR8SegmentationMask[] = "r8SegmentationMask";
constexpr char textureR8GFGuideSub[] = "r8GFGuideSub";
constexpr char textureR8GFSourceSub[] = "r8GFSourceSub";
constexpr char textureR16fGFMeanGuideSub[] = "r16fGFMeanGuideSub";
constexpr char textureR16fGFMeanSourceSub[] = "r16fGFMeanSourceSub";
constexpr char textureR16fGFGuideSourceSub[] = "r16fGFGuideSourceSub";
constexpr char textureR16fGFGuideSqSub[] = "r16fGFGuideSqSub";
constexpr char textureR16fGFMeanGuideSourceSub[] = "r16fGFMeanGuideSourceSub";
constexpr char textureR16fGFMeanGuideSqSub[] = "r16fGFMeanGuideSqSub";
constexpr char textureR16fGFASub[] = "r16fGFASub";
constexpr char textureR16fGFBSub[] = "r16fGFBSub";
constexpr char textureR8GFResult[] = "r8GFResult";

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
	previewTextureSelector->addItem(textureR8SegmentationMask);
	previewTextureSelector->addItem(textureR8GFGuideSub);
	previewTextureSelector->addItem(textureR8GFSourceSub);
	previewTextureSelector->addItem(textureR16fGFMeanGuideSub);
	previewTextureSelector->addItem(textureR16fGFMeanSourceSub);
	previewTextureSelector->addItem(textureR16fGFGuideSourceSub);
	previewTextureSelector->addItem(textureR16fGFGuideSqSub);
	previewTextureSelector->addItem(textureR16fGFMeanGuideSourceSub);
	previewTextureSelector->addItem(textureR16fGFMeanGuideSqSub);
	previewTextureSelector->addItem(textureR16fGFASub);
	previewTextureSelector->addItem(textureR16fGFBSub);
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

		if (!readerMaskRoiR8 || readerMaskRoiR8->width != renderingContext->maskRoiWidth ||
		    readerMaskRoiR8->height != renderingContext->maskRoiHeight) {
			readerMaskRoiR8 = std::make_unique<AsyncTextureReader>(renderingContext->maskRoiWidth,
									       renderingContext->maskRoiHeight, GS_R8);
		}

		if (!reader256Bgrx) {
			reader256Bgrx = std::make_unique<AsyncTextureReader>(256, 256, GS_BGRX);
		}

		if (!readerSubR8 || readerSubR8->width != renderingContext->gfWidthSub ||
		    readerSubR8->height != renderingContext->gfHeightSub) {
			readerSubR8 = std::make_unique<AsyncTextureReader>(renderingContext->gfWidthSub,
									   renderingContext->gfHeightSub, GS_R8);
		}

		if (!readerR16fSub || readerR16fSub->width != renderingContext->gfWidthSub ||
		    readerR16fSub->height != renderingContext->gfHeightSub) {
			readerR16fSub = std::make_unique<AsyncTextureReader>(renderingContext->gfWidthSub,
									     renderingContext->gfHeightSub, GS_R16F);
		}

		auto currentTexture = previewTextureSelector->currentText();
		if (currentTexture == textureBgrxOriginalImage) {
			if (readerBgrx && readerBgrx->width == renderingContext->width &&
			    readerBgrx->height == renderingContext->height) {
				readerBgrx->sync();
				readerBgrx->stage(renderingContext->bgrxOriginalImage.get());
			}
		} else if (currentTexture == textureR8OriginalGrayscale) {
			if (readerR8 && readerR8->width == renderingContext->width &&
			    readerR8->height == renderingContext->height) {
				readerR8->sync();
				readerR8->stage(renderingContext->r8OriginalGrayscale.get());
			}
		} else if (currentTexture == textureBgrxSegmenterInput) {
			if (reader256Bgrx) {
				reader256Bgrx->sync();
				reader256Bgrx->stage(renderingContext->bgrxSegmenterInput.get());
			}
		} else if (currentTexture == textureR8SegmentationMask) {
			if (readerMaskRoiR8 && readerMaskRoiR8->width == renderingContext->maskRoiWidth &&
			    readerMaskRoiR8->height == renderingContext->maskRoiHeight) {
				readerMaskRoiR8->sync();
				readerMaskRoiR8->stage(renderingContext->r8SegmentationMask.get());
			}
		} else if (currentTexture == textureR8GFGuideSub) {
			if (readerSubR8 && readerSubR8->width == renderingContext->gfWidthSub &&
			    readerSubR8->height == renderingContext->gfHeightSub) {
				readerSubR8->sync();
				readerSubR8->stage(renderingContext->r8GFGuideSub.get());
			}
		} else if (currentTexture == textureR8GFSourceSub) {
			if (readerSubR8 && readerSubR8->width == renderingContext->gfWidthSub &&
			    readerSubR8->height == renderingContext->gfHeightSub) {
				readerSubR8->sync();
				readerSubR8->stage(renderingContext->r8GFSourceSub.get());
			}
		} else if (currentTexture == textureR16fGFMeanGuideSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFMeanGuideSub.get());
			}
		} else if (currentTexture == textureR16fGFMeanSourceSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFMeanSourceSub.get());
			}
		} else if (currentTexture == textureR16fGFGuideSourceSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFGuideSourceSub.get());
			}
		} else if (currentTexture == textureR16fGFGuideSqSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFGuideSqSub.get());
			}
		} else if (currentTexture == textureR16fGFMeanGuideSourceSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFMeanGuideSourceSub.get());
			}
		} else if (currentTexture == textureR16fGFMeanGuideSqSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFMeanGuideSqSub.get());
			}
		} else if (currentTexture == textureR16fGFASub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFASub.get());
			}
		} else if (currentTexture == textureR16fGFBSub) {
			if (readerR16fSub && readerR16fSub->width == renderingContext->gfWidthSub &&
			    readerR16fSub->height == renderingContext->gfHeightSub) {
				readerR16fSub->sync();
				readerR16fSub->stage(renderingContext->r16fGFBSub.get());
			}
		} else if (currentTexture == textureR8GFResult) {
			if (readerR8 && readerR8->width == renderingContext->width &&
			    readerR8->height == renderingContext->height) {
				readerR8->sync();
				readerR8->stage(renderingContext->r8GFResult.get());
			}
		}
	}
}

float half_to_float(uint16_t h)
{
	// IEEE 754 half-precision format:
	// 1 sign bit, 5 exponent bits, 10 mantissa bits
	uint32_t s = (h >> 15) & 0x0001; // 符号
	uint32_t e = (h >> 10) & 0x001f; // 指数
	uint32_t m = h & 0x03ff;         // 仮数

	uint32_t result_e, result_m;

	if (e == 0) { // 非正規化数またはゼロ
		if (m == 0) {
			// プラス/マイナスのゼロ
			return s ? -0.0f : 0.0f;
		} else {
			// 非正規化数を正規化数に変換
			while ((m & 0x0400) == 0) {
				m <<= 1;
				e--;
			}
			e++;
			m &= ~0x0400;
		}
	} else if (e == 31) { // 無限大またはNaN
		return s ? -INFINITY : INFINITY;
	}

	// 正規化数の変換
	// 指数部をfloatのバイアス(127)に合わせる
	result_e = e + (127 - 15);
	// 仮数部をfloatの23ビットに合わせる
	result_m = m << 13;

	// ビットを結合して32ビットfloatを作成
	uint32_t result = (s << 31) | (result_e << 23) | result_m;
	return *reinterpret_cast<float *>(&result);
}

void DebugWindow::updatePreview()
{
	auto currentTexture = previewTextureSelector->currentText();

	const std::vector<std::string> bgrxTextures = {textureBgrxOriginalImage};
	const std::vector<std::string> r8Textures = {textureR8OriginalGrayscale, textureR8GFResult};
	const std::vector<std::string> bgrx256Textures = {textureBgrxSegmenterInput};
	const std::vector<std::string> r8MaskRoiTextures = {textureR8SegmentationMask};
	const std::vector<std::string> subR8Textures = {textureR8GFGuideSub, textureR8GFSourceSub};
	const std::vector<std::string> r16fTextures = {textureR16fGFMeanGuideSub,
						       textureR16fGFMeanSourceSub,
						       textureR16fGFGuideSourceSub,
						       textureR16fGFGuideSqSub,
						       textureR16fGFMeanGuideSourceSub,
						       textureR16fGFMeanGuideSqSub,
						       textureR16fGFASub,
						       textureR16fGFBSub};

	QImage image;
	if (std::find(bgrxTextures.begin(), bgrxTextures.end(), currentTexture) != bgrxTextures.end()) {
		image = QImage(readerBgrx->getBuffer().data(), readerBgrx->width, readerBgrx->height,
			       QImage::Format_RGB32);
	} else if (std::find(r8Textures.begin(), r8Textures.end(), currentTexture) != r8Textures.end()) {
		image = QImage(readerR8->getBuffer().data(), readerR8->width, readerR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(bgrx256Textures.begin(), bgrx256Textures.end(), currentTexture) != bgrx256Textures.end()) {
		image = QImage(reader256Bgrx->getBuffer().data(), reader256Bgrx->width, reader256Bgrx->height,
			       QImage::Format_RGB32);
	} else if (std::find(r8MaskRoiTextures.begin(), r8MaskRoiTextures.end(), currentTexture) !=
		   r8MaskRoiTextures.end()) {
		image = QImage(readerMaskRoiR8->getBuffer().data(), readerMaskRoiR8->width, readerMaskRoiR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(subR8Textures.begin(), subR8Textures.end(), currentTexture) != subR8Textures.end()) {
		image = QImage(readerSubR8->getBuffer().data(), readerSubR8->width, readerSubR8->height,
			       QImage::Format_Grayscale8);
	} else if (std::find(r16fTextures.begin(), r16fTextures.end(), currentTexture) != r16fTextures.end()) {
		auto r16fDataView = reinterpret_cast<std::uint16_t *>(readerR16fSub->getBuffer().data());
		bufferSubR8.resize(readerR16fSub->width * readerR16fSub->height);
		for (std::uint32_t i = 0; i < readerR16fSub->width * readerR16fSub->height; ++i) {
			bufferSubR8[i] = static_cast<std::uint8_t>(half_to_float(r16fDataView[i]) * 255);
		}

		image = QImage(bufferSubR8.data(), readerR16fSub->width, readerR16fSub->height,
			       QImage::Format_Grayscale8);
	}

	QPixmap pixmap = QPixmap::fromImage(image);
	QPixmap scaledPixmap =
		pixmap.scaled(PREVIEW_WIDTH, PREVIEW_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	previewImageLabel->setPixmap(scaledPixmap);
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
