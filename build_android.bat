@echo off
cd /d "E:\STORM SWITCH 4\Build\src\android"
call gradlew.bat assembleMainlineRelease assembleLegacyRelease assembleSdk27Release
if errorlevel 1 (
    echo Android build failed
    exit /b 1
)
echo Android build completed successfully!
powershell -ExecutionPolicy Bypass -File "E:\STORM SWITCH 4\Build\copy_apks.ps1"

