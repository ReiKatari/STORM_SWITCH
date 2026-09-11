@echo off
cd /d "e:\STORM EDEN 3\src\src\android"
call gradlew.bat assembleMainlineRelease assembleLegacyRelease assembleSdk27Release
if errorlevel 1 (
    echo Android build failed
    exit /b 1
)
echo Android build completed successfully!
