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

set "PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;E:\STORM EDEN 3\tools\glslang\bin;C:\Users\ReiKatari\glslang\bin"

set "SRC_DIR=E:\STORM SWITCH 4\Build"
set "BUILD_DIR=E:\STORM SWITCH 4\Build\build_ninja"
set "OUTPUT_DIR=E:\STORM SWITCH 4\Assembling"

set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo [1/3] Running CMake configuration...
"%CMAKE%" -G "Ninja" -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DGIT_TAG="9.3.0" ^
    -DGIT_RELEASE="9.3.0" ^
    -DENABLE_QT=ON ^
    -DENABLE_QT_TRANSLATION=ON ^
    -DYUZU_USE_BUNDLED_QT=ON ^
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
    mkdir "%OUTPUT_DIR%\user\keys"
    mkdir "%OUTPUT_DIR%\user\nand"
    mkdir "%OUTPUT_DIR%\user\load"
    mkdir "%OUTPUT_DIR%\user\screenshots"
    mkdir "%OUTPUT_DIR%\user\sdmc"
    mkdir "%OUTPUT_DIR%\user\shader"
    mkdir "%OUTPUT_DIR%\user\cache"
)

if not exist "%OUTPUT_DIR%\user\config\qt-config.ini" (
    if exist "E:\STORM SWITCH 3\Assembling\user\config\qt-config.ini" (
        copy /Y "E:\STORM SWITCH 3\Assembling\user\config\qt-config.ini" "%OUTPUT_DIR%\user\config\"
    )
)
if not exist "%OUTPUT_DIR%\user\config\custom" (
    if exist "E:\STORM SWITCH 3\Assembling\user\config\custom" (
        xcopy /E /Y /I "E:\STORM SWITCH 3\Assembling\user\config\custom" "%OUTPUT_DIR%\user\config\custom\"
    )
)

if exist "L:\CONSOLES\Nintendo Switch\STORM EDEN\user\keys" (
    xcopy /Y /I "L:\CONSOLES\Nintendo Switch\STORM EDEN\user\keys\*" "%OUTPUT_DIR%\user\keys\"
)

echo ===================================================
echo STORM SWITCH build and assembly completed successfully!
echo ===================================================
