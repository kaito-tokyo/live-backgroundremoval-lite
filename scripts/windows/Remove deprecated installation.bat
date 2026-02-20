setlocal

del /f /q "C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.dll"
del /f /q "C:\Program Files\obs-studio\obs-plugins\64bit\live-backgroundremoval-lite.pdb"
rmdir /s /q "C:\Program Files\obs-studio\data\obs-plugins\live-backgroundremoval-lite"

echo Completed cleanup.

pause

endlocal
