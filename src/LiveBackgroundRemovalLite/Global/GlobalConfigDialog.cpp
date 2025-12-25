#include "GlobalConfigDialog.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <obs-data.h>

namespace KaitoTokyo::LiveBackgroundRemovalLite::Global {

GlobalConfigDialog::GlobalConfigDialog(QWidget *parent) : QDialog(parent)
{
	setupUi();
	loadConfig();
}

GlobalConfigDialog::~GlobalConfigDialog() = default;

void GlobalConfigDialog::setupUi()
{
	setWindowTitle(tr("Live Background Removal Lite - Global Settings"));

	// ウィンドウ幅を少し広めに確保（フォームレイアウトが見やすくなるため）
	resize(500, 250);

	// 1. メインレイアウト（全体を縦に並べる）
	auto *mainLayout = new QVBoxLayout(this);

	// --- ヘッダー部分（タイトルとバージョン） ---
	// HTMLタグを使って少しリッチに表示
	auto *titleLabel = new QLabel(tr("<h3>Live Background Removal Lite</h3>"), this);
	titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(titleLabel);

	// --- 設定フォーム部分（ここをQFormLayoutで綺麗にする） ---
	auto *settingsGroup = new QGroupBox(tr("General Settings"), this);
	auto *formLayout = new QFormLayout(settingsGroup);

	// 行間のマージンやスペースを少し調整（お好みで）
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignLeft);

	// 【項目1】 バージョン表示（ただのテキストラベル）
	auto *versionLabel = new QLabel("v1.0.0", this);
	versionLabel->setStyleSheet("color: gray;"); // 少し薄くする
	formLayout->addRow(tr("Current Version:"), versionLabel);

	// 【項目2】 アップデート確認（チェックボックス）
	m_checkUpdatesBox = new QCheckBox(tr("Check automatically on startup"), this);
	m_checkUpdatesBox->setToolTip(tr("Enable to receive notifications about new features and bug fixes."));

	// QFormLayoutのaddRowを使うと「ラベル : ウィジェット」の形で綺麗に配置されます
	formLayout->addRow(tr("Updates:"), m_checkUpdatesBox);

	// グループボックスをメインレイアウトに追加
	mainLayout->addWidget(settingsGroup);

	// --- フッター部分 ---
	mainLayout->addStretch(); // 下にスペースを空ける

	auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
	mainLayout->addWidget(buttonBox);

	// シグナル接続
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
	connect(m_checkUpdatesBox, &QCheckBox::toggled, this, &GlobalConfigDialog::onUpdateCheckToggled);
}

void GlobalConfigDialog::loadConfig()
{
	obs_data_t *config = obs_module_get_config();
	if (!config)
		return;

	obs_data_set_default_bool(config, "CheckUpdates", true);
	const bool isEnabled = obs_data_get_bool(config, "CheckUpdates");

	QSignalBlocker blocker(m_checkUpdatesBox);
	m_checkUpdatesBox->setChecked(isEnabled);
}

void GlobalConfigDialog::onUpdateCheckToggled(bool checked)
{
	obs_data_t *config = obs_module_get_config();
	if (!config)
		return;

	obs_data_set_bool(config, "CheckUpdates", checked);
	obs_module_save_config();
}

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::Global
