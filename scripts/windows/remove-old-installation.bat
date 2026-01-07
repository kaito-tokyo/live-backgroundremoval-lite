@echo off
REM Remove old installation of live-backgroundremoval-lite plugin
setlocal

set PLUGIN_DIR1=C:\ProgramData\obs-studio\plugins\live-backgroundremoval-lite
set DLL_FILE=C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.dll
set PDB_FILE=C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.pdb
set DATA_DIR=C:\Program Files\obs-studio\data\obs-plugins\live-backgroundremoval-lite

REM Remove plugin directory (ProgramData)
if exist "%PLUGIN_DIR1%" (
    echo Removing "%PLUGIN_DIR1%" ...
    rmdir /s /q "%PLUGIN_DIR1%"
    if exist "%PLUGIN_DIR1%" (
        echo Failed to remove "%PLUGIN_DIR1%".
        exit /b 1
    ) else (
        echo Successfully removed "%PLUGIN_DIR1%".
    )
) else (
    echo "%PLUGIN_DIR1%" does not exist.
)

REM Remove DLL file
if exist "%DLL_FILE%" (
    echo Removing "%DLL_FILE%" ...
    del /f /q "%DLL_FILE%"
    if exist "%DLL_FILE%" (
        echo Failed to remove "%DLL_FILE%".
        exit /b 1
    ) else (
        echo Successfully removed "%DLL_FILE%".
    )
) else (
    echo "%DLL_FILE%" does not exist.
)

REM Remove PDB file
if exist "%PDB_FILE%" (
    echo Removing "%PDB_FILE%" ...
    del /f /q "%PDB_FILE%"
    if exist "%PDB_FILE%" (
        echo Failed to remove "%PDB_FILE%".
        exit /b 1
    ) else (
        echo Successfully removed "%PDB_FILE%".
    )
) else (
    echo "%PDB_FILE%" does not exist.
)

REM Remove data directory
if exist "%DATA_DIR%" (
    echo Removing "%DATA_DIR%" ...
    rmdir /s /q "%DATA_DIR%"
    if exist "%DATA_DIR%" (
        echo Failed to remove "%DATA_DIR%".
        exit /b 1
    ) else (
        echo Successfully removed "%DATA_DIR%".
    )
) else (
    echo "%DATA_DIR%" does not exist.
)

echo.
echo ------------------------------------------------------------
echo If you have installed OBS Studio in a different location,
echo please manually remove the corresponding plugin files and
echo directories listed above from that location.
echo ------------------------------------------------------------

pause

endlocal
