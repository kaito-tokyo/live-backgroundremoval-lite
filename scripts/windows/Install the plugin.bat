REM SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
REM
REM SPDX-License-Identifier: Apache-2.0

setlocal
set "PLUGIN_NAME=Live Background Removal Lite"
set "PLUGIN_DIR=live-backgroundremoval-lite"
set "TARGET_DIR=C:\ProgramData\obs-studio\plugins\%PLUGIN_DIR%"

openfiles >nul 2>&1
if %errorlevel% neq 0 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); [System.Windows.Forms.MessageBox]::Show('Please run as administrator.', 'Error', 0, 16)"
    exit /b
)

if not exist "%TARGET_DIR%" (
    mkdir "%TARGET_DIR%"
)

robocopy "%~dp0%PLUGIN_DIR%" "%TARGET_DIR%" /E /IS

if %errorlevel% GEQ 8 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); [System.Windows.Forms.MessageBox]::Show('Failed to copy files.', 'Error', 0, 16)"
    exit /b
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "[System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); [System.Windows.Forms.MessageBox]::Show('%PLUGIN_NAME% installed successfully!', 'Success', 0, 64)"

exit /b
