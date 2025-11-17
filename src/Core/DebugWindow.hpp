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

#include <AsyncTextureReader.hpp>

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

class MainPluginContext;

class DebugWindow : public QDialog {
	Q_OBJECT
public:
	constexpr static int kPreviewWidth = 640;
	constexpr static int kPreviewHeight = 480;

	DebugWindow(std::weak_ptr<MainPluginContext> weakMainPluginContext, QWidget *parent = nullptr);

	void videoRender();

private slots:
	void updatePreview();

private:
	std::weak_ptr<MainPluginContext> weakMainPluginContext_;

	QVBoxLayout *layout_;
	QComboBox *previewTextureSelector_;
	QLabel *previewImageLabel_;
	QTimer *updateTimer_;

	std::mutex readerMutex_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> bgrxReader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> r8Reader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> r32fReader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> bgrxSegmenterInputReader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> r8MaskRoiReader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> r8SubR8Reader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> r32fSubReader_;
	std::shared_ptr<BridgeUtils::AsyncTextureReader> r32fSubPaddedReader_;

	std::vector<std::uint8_t> bufferR8_;
	std::vector<std::uint8_t> bufferSubR8_;
	std::vector<std::uint8_t> bufferSubPaddedR8_;
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
