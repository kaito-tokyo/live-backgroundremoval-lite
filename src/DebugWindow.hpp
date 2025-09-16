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

#pragma once

#include <memory>

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include "AsyncTextureReader.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class MainPluginContext;

class DebugWindow : public QDialog {
	Q_OBJECT
public:
    static constexpr int PREVIEW_WIDTH = 640;
    static constexpr int PREVIEW_HEIGHT = 480;

	explicit DebugWindow(std::weak_ptr<MainPluginContext> weakMainPluginContext, QWidget *parent = nullptr);
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

    std::unique_ptr<AsyncTextureReader> readerBgrx;
    std::unique_ptr<AsyncTextureReader> readerR8;
    std::unique_ptr<AsyncTextureReader> reader256Bgrx;
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
