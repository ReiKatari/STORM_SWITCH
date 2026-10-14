@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo STORM SWITCH - Automated Build and Packaging Script
echo ===================================================

taskkill /F /IM STORM_SWITCH.exe /IM STORM_SWITCH-cli.exe /IM STORM_SWITCH-room.exe 2>nul

call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" amd64
if errorlevel 1 (
    echo [ERROR] Failed to set up Visual Studio environment.
    exit /b 1
)

set "PATH=%~dp0tools\glslang\bin;%PATH%;C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

set "SRC_DIR=%~dp0"
if "%SRC_DIR:~-1%"=="\" set "SRC_DIR=%SRC_DIR:~0,-1%"
set "BUILD_DIR=%SRC_DIR%\build_ninja"
set "OUTPUT_DIR=%SRC_DIR%\..\Assembling"
set "Qt6_DIR=%SRC_DIR%\.cache\cpm\qt6\6.11.1-windows-amd64\lib\cmake\Qt6"
set "CPM_SOURCE_CACHE=%SRC_DIR%\.cache\cpm"

set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo [1/3] Running CMake configuration...
"%CMAKE%" -G "Ninja" -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DGIT_TAG="11.0.0" ^
    -DGIT_RELEASE="11.0.0" ^
    -DENABLE_QT=ON ^
    -DENABLE_QT_TRANSLATION=ON ^
    -DYUZU_USE_BUNDLED_QT=ON ^
    -DQt6_DIR="%Qt6_DIR%" ^
    -DCPM_SOURCE_CACHE="%CPM_SOURCE_CACHE%" ^
    -DYUZU_CMD=ON ^
    -DYUZU_ROOM=ON ^
    -DYUZU_ROOM_STANDALONE=ON ^
    -DENABLE_WEB_SERVICE=OFF ^
    -DBUILD_TESTING=OFF ^
    "%SRC_DIR%"

if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo [2/3] Compiling STORM SWITCH with Ninja...
"%CMAKE%" --build . --config Release
if errorlevel 1 (
    echo [ERROR] Compilation failed.
    exit /b 1
)

echo [3/3] Packaging into %OUTPUT_DIR%...
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
xcopy /E /Y /I bin\* "%OUTPUT_DIR%\"

copy /Y "C:\Program Files\7-Zip\7z.exe" "%OUTPUT_DIR%\" 2>nul
copy /Y "C:\Program Files\7-Zip\7z.dll" "%OUTPUT_DIR%\" 2>nul
if not exist "%OUTPUT_DIR%\7z.exe" (
    copy /Y "E:\STORM SWITCH 3\Assembling\7z.*" "%OUTPUT_DIR%\" 2>nul
)

echo Bundling Microsoft Visual C++ 14.51 CRT libraries for universal Windows 10/11 launch...
copy /Y "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT\*.dll" "%OUTPUT_DIR%\" 2>nul
copy /Y "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP\*.dll" "%OUTPUT_DIR%\" 2>nul

if not exist "%OUTPUT_DIR%\user" (
    mkdir "%OUTPUT_DIR%\user"
    mkdir "%OUTPUT_DIR%\user\config"
    mkdir "%OUTPUT_DIR%\user\config\custom"
    mkdir "%OUTPUT_DIR%\user\keys"
    mkdir "%OUTPUT_DIR%\user\nand"
    mkdir "%OUTPUT_DIR%\user\load"
    mkdir "%OUTPUT_DIR%\user\screenshots"
    mkdir "%OUTPUT_DIR%\user\sdmc"
    mkdir "%OUTPUT_DIR%\user\shader"
    mkdir "%OUTPUT_DIR%\user\cache"
)

echo ===================================================
echo STORM SWITCH build and assembly completed successfully!
echo ===================================================
