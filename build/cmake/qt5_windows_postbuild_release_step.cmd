@echo off

set QTDIR=%1
set OUTPUT_DIR=%2

echo Qt home         : %QTDIR%
echo Output directory: %OUTPUT_DIR%

xcopy /Y /E "%QTDIR%\bin\icu*.dll"       "%OUTPUT_DIR%"
xcopy /Y /E "%QTDIR%\bin\Qt5Core.dll"    "%OUTPUT_DIR%"
xcopy /Y /E "%QTDIR%\bin\Qt5Gui.dll"     "%OUTPUT_DIR%"
xcopy /Y /E "%QTDIR%\bin\Qt5Widgets.dll" "%OUTPUT_DIR%"

if not exist "$(OUTPUT_DIR)\platforms" mkdir "%OUTPUT_DIR%\platforms"
xcopy /Y /E "%QTDIR%\plugins\platforms\qwindows.dll" "%OUTPUT_DIR%\platforms"
