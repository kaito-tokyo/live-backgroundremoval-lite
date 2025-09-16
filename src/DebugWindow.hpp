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

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QTimer;
QT_END_NAMESPACE

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class MainPluginContext;

class DebugWindow : public QDialog {
	Q_OBJECT
public:
	explicit DebugWindow(std::weak_ptr<MainPluginContext> mainPluginContext, QWidget *parent = nullptr);
	~DebugWindow() noexcept override;

private slots:
	void updateImage();

private:
	std::weak_ptr<MainPluginContext> mainPluginContext;
	QLabel *imageLabel = nullptr;
	QTimer *timer = nullptr;
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
