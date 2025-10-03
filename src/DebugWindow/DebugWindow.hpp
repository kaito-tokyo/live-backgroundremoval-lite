#pragma once

#include <atomic>
#include <memory>

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include "BridgeUtils/AsyncTextureReader.hpp"
#include "BridgeUtils/ILogger.hpp"

namespace KaitoTokyo {
namespace BackgroundRemovalLite {

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

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
