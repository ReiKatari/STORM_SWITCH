@echo off
set "JAVA_HOME=C:\Program Files\Java\jdk-21.0.10"
set "PATH=%JAVA_HOME%\bin;%PATH%"
set "ANDROID_HOME=C:\Users\ReiKatari\AppData\Local\Android\Sdk"
set "ANDROID_SDK_ROOT=C:\Users\ReiKatari\AppData\Local\Android\Sdk"

cd /d "E:\STORM SWITCH 4\Build\src\android"
call gradlew.bat assembleMainlineRelease assembleLegacyRelease assembleSdk27Release
if errorlevel 1 (
    echo Android build failed
    exit /b 1
)
echo Android build completed successfully!
powershell -ExecutionPolicy Bypass -File "E:\STORM SWITCH 4\Build\copy_apks.ps1"
