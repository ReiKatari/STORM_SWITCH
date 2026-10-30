@echo off
powershell -NoProfile -Command "try { $ppid = (Get-CimInstance Win32_Process -Filter \"ProcessId=$PID\").ParentProcessId; (Get-Process -Id $ppid).PriorityClass = 'BelowNormal'; Get-Process java -ErrorAction SilentlyContinue | ForEach-Object { try { $_.PriorityClass = 'BelowNormal' } catch {} } } catch {}" 2>nul
set "JAVA_HOME=C:\Users\ReiKa\.jdks\jdk-21.0.12.1+1"
set "PATH=%JAVA_HOME%\bin;%PATH%"
set "ANDROID_HOME=C:\Users\ReiKa\AppData\Local\Android\Sdk"
set "ANDROID_SDK_ROOT=C:\Users\ReiKa\AppData\Local\Android\Sdk"
set "CMAKE_BUILD_PARALLEL_LEVEL=4"

cd /d "%~dp0src\android"
call gradlew.bat assembleMainlineRelease --max-workers=2 -Dorg.gradle.priority=low -Dorg.gradle.java.home="%JAVA_HOME%"
if errorlevel 1 (
    echo Android Mainline build failed
    exit /b 1
)
call gradlew.bat assembleLegacyRelease --max-workers=2 -Dorg.gradle.priority=low -Dorg.gradle.java.home="%JAVA_HOME%"
if errorlevel 1 (
    echo Android Legacy build failed
    exit /b 1
)
call gradlew.bat assembleSdk27Release --max-workers=2 -Dorg.gradle.priority=low -Dorg.gradle.java.home="%JAVA_HOME%"
if errorlevel 1 (
    echo Android Sdk27 build failed
    exit /b 1
)
echo Android build completed successfully!
powershell -ExecutionPolicy Bypass -File "%~dp0copy_apks.ps1"
