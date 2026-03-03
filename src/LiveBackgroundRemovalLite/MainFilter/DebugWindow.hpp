// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <memory>

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/AsyncTextureReader.hpp>

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

class MainFilterContext;

class DebugWindow : public QDialog {
	Q_OBJECT
public:
	constexpr static int kPreviewWidth = 640;
	constexpr static int kPreviewHeight = 480;

	DebugWindow(std::weak_ptr<MainFilterContext> weakMainFilterContext, QWidget *parent = nullptr);

	void videoRender();

private slots:
	void updatePreview();
	void onTextureSelectionChanged(int index);

private:
	std::weak_ptr<MainFilterContext> weakMainFilterContext_;
	std::shared_ptr<const Logger::ILogger> logger_;

	QVBoxLayout *layout_;
	QComboBox *previewTextureSelector_;
	QLabel *previewImageLabel_;
	QTimer *updateTimer_;

	std::atomic<int> selectedPreviewTextureIndex_ = 0;

	std::mutex readerMutex_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> bgrxReader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> r8Reader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> r32fReader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> bgrxSegmenterInputReader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> r8MaskRoiReader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> r8SubR8Reader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> r32fSubReader_;
	std::shared_ptr<ObsBridgeUtils::AsyncTextureReader> r32fSubPaddedReader_;

	std::vector<std::uint8_t> bufferR8_;
	std::vector<std::uint8_t> bufferSubR8_;
	std::vector<std::uint8_t> bufferSubPaddedR8_;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
