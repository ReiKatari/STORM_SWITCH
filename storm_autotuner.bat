@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

python --version >nul 2>&1
if %ERRORLEVEL% equ 0 (
    python "%~dp0tools\storm_autotuner.py" %*
    goto :eof
)

py --version >nul 2>&1
if %ERRORLEVEL% equ 0 (
    py "%~dp0tools\storm_autotuner.py" %*
    goto :eof
)

uv --version >nul 2>&1
if %ERRORLEVEL% equ 0 (
    uv run python "%~dp0tools\storm_autotuner.py" %*
    goto :eof
)

if exist "%USERPROFILE%\.local\bin\uv.exe" (
    "%USERPROFILE%\.local\bin\uv.exe" run python "%~dp0tools\storm_autotuner.py" %*
    goto :eof
)

echo [ОШИБКА] Python 3 не найден в PATH! Установите Python для запуска утилиты Auto-Tuner.
pause