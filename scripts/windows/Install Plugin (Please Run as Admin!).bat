
@echo off
REM Install Live Background Removal Lite Plugin for OBS Studio (Run as Administrator)

setlocal
set PLUGIN_NAME=live-backgroundremoval-lite
set DEST_DIR=C:\ProgramData\obs-studio\plugins

REM Get the directory of this script
set SCRIPT_DIR=%~dp0

REM Check for admin rights
openfiles >nul 2>&1
if %errorlevel% neq 0 (
	echo This script requires administrator privileges.
	echo Please right-click and select "Run as administrator".
	pause
	exit /b 1
)

REM Check for conflicting files in Program Files OBS Studio
:check_conflicts
set CONFLICT1=C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.dll
set CONFLICT2=C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.pdb
set CONFLICT3=C:\Program Files\obs-studio\data\obs-plugins\live-backgroundremoval-lite

set CONFLICT_FOUND=0
if exist "%CONFLICT1%" set CONFLICT_FOUND=1
if exist "%CONFLICT2%" set CONFLICT_FOUND=1
if exist "%CONFLICT3%" set CONFLICT_FOUND=1

if %CONFLICT_FOUND%==1 (
	echo.
	echo WARNING: The following files or folders must be deleted before installation can continue:
	if exist "%CONFLICT1%" echo   %CONFLICT1%
	if exist "%CONFLICT2%" echo   %CONFLICT2%
	if exist "%CONFLICT3%" echo   %CONFLICT3%\
	echo.
	echo Please close OBS Studio and delete these files/folders manually.
	echo The installation will not continue until they are removed.
	echo.
	pause
	goto check_conflicts
)

REM Create destination directory if it doesn't exist
if not exist "%DEST_DIR%" mkdir "%DEST_DIR%"

REM Copy the plugin folder
echo Copying %PLUGIN_NAME% to %DEST_DIR% ...
xcopy "%SCRIPT_DIR%%PLUGIN_NAME%" "%DEST_DIR%\%PLUGIN_NAME%" /E /I /Y
if %errorlevel% neq 0 (
	echo Failed to copy plugin files.
	pause
	exit /b 1
)

echo Installation complete.
echo Please restart OBS Studio if it is running.
pause
exit /b 0
