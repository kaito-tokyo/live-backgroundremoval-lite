setlocal

set "PLUGIN_DIR=C:\ProgramData\obs-studio\plugins\live-backgroundremoval-lite"
set "DLL_FILE=C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.dll"
set "PDB_FILE=C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.pdb"
set "DATA_DIR=C:\Program Files\obs-studio\data\obs-plugins\live-backgroundremoval-lite"

if exist "%PLUGIN_DIR%" (
    echo Removing "%PLUGIN_DIR%" ...
    rmdir /s /q "%PLUGIN_DIR%"
    if exist "%PLUGIN_DIR%" (
        echo Failed to remove "%PLUGIN_DIR%".
    ) else (
        echo Successfully removed "%PLUGIN_DIR%".
    )
) else (
    echo "%PLUGIN_DIR%" does not exist.
)

if exist "%DLL_FILE%" (
    echo Removing "%DLL_FILE%" ...
    del /f /q "%DLL_FILE%"
    if exist "%DLL_FILE%" (
        echo Failed to remove "%DLL_FILE%".
    ) else (
        echo Successfully removed "%DLL_FILE%".
    )
) else (
    echo "%DLL_FILE%" does not exist.
)

if exist "%PDB_FILE%" (
    echo Removing "%PDB_FILE%" ...
    del /f /q "%PDB_FILE%"
    if exist "%PDB_FILE%" (
        echo Failed to remove "%PDB_FILE%".
    ) else (
        echo Successfully removed "%PDB_FILE%".
    )
) else (
    echo "%PDB_FILE%" does not exist.
)

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

echo Completed cleanup.

pause

endlocal
