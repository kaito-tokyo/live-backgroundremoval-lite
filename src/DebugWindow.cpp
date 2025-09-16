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

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

DebugWindow::DebugWindow(std::weak_ptr<MainPluginContext> _weakMainPluginContext, QWidget *parent)
	: QDialog(parent), weakMainPluginContext(std::move(_weakMainPluginContext)),
	layout(new QVBoxLayout(this)),
	previewImageLabel(new QLabel(this))
{
	layout->addWidget(previewImageLabel);
	previewImageLabel->setText("No image");

	setLayout(layout);
}

DebugWindow::~DebugWindow() noexcept {}

void DebugWindow::videoRender()
{
	if (auto mainPluginContext = weakMainPluginContext.lock()) {
		if (!readerBgrx) {
			auto renderingContext = weakMainPluginContext.lock()->getRenderingContext();
			readerBgrx = std::make_unique<AsyncTextureReader>(renderingContext->width, renderingContext->height,
									   GS_BGRX);
		}

		if (readerBgrx) {
			readerBgrx->sync();
			if (auto mainPluginContext = weakMainPluginContext.lock()) {
				auto renderingContext = mainPluginContext->getRenderingContext();
				readerBgrx->stage(renderingContext->bgrxOriginalImage.get());
			}
		}

		auto bgrxData = readerBgrx->getBuffer();
	}
}

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
