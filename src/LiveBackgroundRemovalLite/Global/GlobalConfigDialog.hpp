/* GlobalConfigDialog.hpp */
#pragma once

#include <QDialog>
#include <obs-module.h>

class QCheckBox;
class QVBoxLayout;

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

class GlobalConfigDialog : public QDialog {
	Q_OBJECT

public:
	explicit GlobalConfigDialog(QWidget *parent = nullptr);
	~GlobalConfigDialog() override;

private:
	void setupUi();
	void loadConfig(); // 設定読み込み

private slots:
	void onUpdateCheckToggled(bool checked); // チェック変更時の保存処理

private:
	QCheckBox *m_checkUpdatesBox = nullptr;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
