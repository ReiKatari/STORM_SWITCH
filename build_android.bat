@echo off
set "JAVA_HOME=C:\Users\ReiKa\.jdks\jdk-21.0.12.1+1"
set "PATH=%JAVA_HOME%\bin;%PATH%"
set "ANDROID_HOME=C:\Users\ReiKa\AppData\Local\Android\Sdk"
set "ANDROID_SDK_ROOT=C:\Users\ReiKa\AppData\Local\Android\Sdk"

cd /d "%~dp0src\android"
call gradlew.bat assembleMainlineRelease assembleLegacyRelease assembleSdk27Release -Dorg.gradle.java.home="%JAVA_HOME%"
if errorlevel 1 (
    echo Android build failed
    exit /b 1
)
echo Android build completed successfully!
powershell -ExecutionPolicy Bypass -File "%~dp0copy_apks.ps1"
