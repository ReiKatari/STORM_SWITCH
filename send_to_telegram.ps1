$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Net.Http
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$tokenFile = "e:\STORM EDEN 3\tg_token.txt"
if (Test-Path $tokenFile) {
    $token = (Get-Content $tokenFile -Raw -Encoding UTF8).Trim()
} elseif ($env:TELEGRAM_BOT_TOKEN) {
    $token = $env:TELEGRAM_BOT_TOKEN.Trim()
} else {
    Write-Error "Telegram bot token file not found!"
    exit 1
}

$chatId = "-5389146045"

$httpClient = [System.Net.Http.HttpClient]::new()
$httpClient.Timeout = [System.TimeSpan]::FromMinutes(15)

function Send-TGMessage([string]$text) {
    Write-Host "1. Sending release announcement..."
    $url = "https://api.telegram.org/bot$token/sendMessage"
    $payload = @{
        chat_id = $chatId
        text = $text
        parse_mode = "HTML"
        disable_web_page_preview = $true
    } | ConvertTo-Json -Compress

    $content = [System.Net.Http.StringContent]::new($payload, [System.Text.Encoding]::UTF8, "application/json")
    $response = $httpClient.PostAsync($url, $content).GetAwaiter().GetResult()
    $resStr = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    Write-Host "Announcement Sent: $resStr"
}

function Send-TGDocument([string]$filePath, [string]$caption) {
    $fileName = [System.IO.Path]::GetFileName($filePath)
    Write-Host "Sending $fileName via HttpClient (100% UTF-8)..."
    $url = "https://api.telegram.org/bot$token/sendDocument"

    $form = [System.Net.Http.MultipartFormDataContent]::new()
    $form.Add([System.Net.Http.StringContent]::new($chatId, [System.Text.Encoding]::UTF8), "chat_id")
    $form.Add([System.Net.Http.StringContent]::new("HTML", [System.Text.Encoding]::UTF8), "parse_mode")
    $form.Add([System.Net.Http.StringContent]::new($caption, [System.Text.Encoding]::UTF8), "caption")

    $fileStream = [System.IO.File]::OpenRead($filePath)
    $fileContent = [System.Net.Http.StreamContent]::new($fileStream)
    $fileContent.Headers.ContentType = [System.Net.Http.Headers.MediaTypeHeaderValue]::Parse("application/octet-stream")
    $form.Add($fileContent, "document", $fileName)

    $response = $httpClient.PostAsync($url, $form).GetAwaiter().GetResult()
    $resStr = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    $fileStream.Close()
    $fileStream.Dispose()

    Write-Host "Uploaded $fileName successfully: $resStr`n"
}

$announcement = @"
⚡ <b>Релиз STORM SWITCH 8.0.5 (Поддержка архивов модов .7z и .rar, менеджер установленных Amiibo, редизайн STORM GAMES WORLD и стабильность SoR4)</b> — <i>Масштабное обновление эмулятора Nintendo Switch для платформ Windows x64 и Android: полноценная распаковка модов .7z и .rar, встроенный менеджер локальных дампов Amiibo, глубокий визуальный редизайн каталога STORM GAMES WORLD с официальными обложками и информативными плашками, расширенный журнал логов и исправление стабильности видеодекодера NVDEC.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

📦 <b>Универсальная распаковка модов .7z и .rar (Windows и Android):</b>
• <b>Windows</b>: встроенная распаковка архивов <code>.7z</code> и <code>.rar</code> модов GameBanana через портативный движок 7-Zip. Устранена ошибка повторного вложения каталогов <code>romfs/romfs</code>, предотвращено копирование нераспакованных архивов в папку модов.
• <b>Android</b>: прямая поддержка распаковки архивов <code>.7z</code>, <code>.rar</code> и <code>.zip</code> на базе библиотек Commons Compress, XZ и Junrar с проверкой целостности файлов и обновленным интерфейсом с огранкой переключателей и поля поиска.

👾 <b>Менеджер установленных Amiibo (Android):</b>
• <b>Вкладка «Установленные»</b>: внедрена удобная панель переключения между онлайн-каталогом и локальными Amiibo.
• <b>Управление дампами .bin</b>: мгновенный просмотр сохраненных дампов NTAG215 с точным размером, быстрая загрузка выбранной фигурки в активную игру и удаление ненужных файлов прямо из интерфейса.

📱 <b>Редизайн и улучшения STORM GAMES WORLD на Android:</b>
• <b>Корректный отступ под системную шторку</b>: заголовок и панель диалога опущены под системный статус-бар и вырез экрана (Display Cutout) для исключения наложения часов и значков системы.
• <b>Поддержка альбомного режима</b>: кнопка быстрого перехода в STORM GAMES WORLD добавлена на главный экран при горизонтальной ориентации.
• <b>Восстановление официальных обложек</b>: реализован автоматический поиск и загрузка обложек из CDN Nintendo eShop с резервным отображением локальных иконок установленных игр.
• <b>Информативные плашки и 3D-кнопки</b>: убран префикс «v» из версий, добавлены стильные неоновые плашки внутренней версии (например, <code>655360</code>), размера файла и языка игры, а также объемная кнопка «Скачать» / «Скачано».

📋 <b>Расширенный журнал работы (Логи):</b>
• Окно логов на Android увеличено в ширину и высоту до точных пропорций окон онлайн-базы Amiibo и чит-кодов для удобного чтения длинных сообщений и трассировок.

🎮 <b>Стабильность видеодекодера NVDEC (Streets of Rage 4):</b>
• Корректная обработка тайм-аута отправки кадров <code>SetSubmitTimeout</code> без падений эмулятора.
• Защита от обращения к нулевым дескрипторам буферов в <code>UnmapBuffer</code> и устранение спама предупреждений NVMAP.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.5.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.5 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.5_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.5 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.5_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.5 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.5_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.5 (Портативная версия для Windows x64)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path -LiteralPath $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

$httpClient.Dispose()
Write-Host "`nRelease 8.0.5 deployment to Telegram completed successfully!"
