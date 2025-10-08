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

#pragma once

#include <atomic>
#include <memory>

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include "../BridgeUtils/AsyncTextureReader.hpp"
#include "../BridgeUtils/ILogger.hpp"

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

class MainPluginContext;

struct DebugRenderData {
	std::unique_ptr<BridgeUtils::AsyncTextureReader> readerBgrx;
	std::unique_ptr<BridgeUtils::AsyncTextureReader> readerR8;
	std::unique_ptr<BridgeUtils::AsyncTextureReader> readerR32f;
	std::unique_ptr<BridgeUtils::AsyncTextureReader> reader256Bgrx;
	std::unique_ptr<BridgeUtils::AsyncTextureReader> readerMaskRoiR8;
	std::unique_ptr<BridgeUtils::AsyncTextureReader> readerSubR8;
	std::unique_ptr<BridgeUtils::AsyncTextureReader> readerR32fSub;
};

class DebugWindow : public QDialog {
	Q_OBJECT
public:
	static constexpr int PREVIEW_WIDTH = 640;
	static constexpr int PREVIEW_HEIGHT = 480;

	DebugWindow(std::weak_ptr<MainPluginContext> weakMainPluginContext, QWidget *parent = nullptr);
	~DebugWindow() noexcept override;

	void videoRender();

signals:
	void readerReady();

private slots:
	void updatePreview();

private:
	std::weak_ptr<MainPluginContext> weakMainPluginContext;

	QVBoxLayout *layout;
	QComboBox *previewTextureSelector;
	QLabel *previewImageLabel;
	QTimer *updateTimer;

	std::atomic<DebugRenderData *> atomicRenderData{nullptr};
	std::unique_ptr<DebugRenderData> currentRenderData;
	std::vector<std::unique_ptr<DebugRenderData>> oldRenderData;

	std::vector<std::uint8_t> bufferR8;
	std::vector<std::uint8_t> bufferSubR8;
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
