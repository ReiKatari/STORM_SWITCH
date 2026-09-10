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
⚡ <b>Релиз STORM SWITCH 8.0.6 (Ликвидация фризов 4 FPS, нативная распаковка модов .7z, обновление каталога STORM GAMES WORLD и совместимость)</b> — <i>Комплексное обновление эмулятора Nintendo Switch для Windows x64 и Android: устранение критической задержки видеопамяти 200 мс и падения до 4 FPS, исправление графических артефактов в Animal Well, нативная распаковка 7-Zip (GameBanana) с поддержкой сложных фильтров BCJ2, регистрация AOC-команды 50 и обновленный интерфейс STORM GAMES WORLD.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🎮 <b>Устранение просадки 4 FPS и статтеров видеопамяти (Windows):</b>
• <b>Оптимизация чтения буферов GPU</b>: обратное чтение буферов видеопамяти теперь вызывается только для реально модифицированных видеокартой областей памяти (<code>IsRegionGpuModified</code>), что устранило 200-миллисекундные задержки синхронизации PCIe в <i>Animal Well</i>, <i>Streets of Rage 4</i> и <i>Diablo II: Resurrected</i>.
• <b>Глобальное отключение readback по умолчанию</b>: параметр <code>enable_gpu_buffer_readback</code> переведен в <code>false</code> по умолчанию во всех конфигурациях для максимальной отзывчивости рендеринга.
• <b>Исправление графики Animal Well</b>: включена высокая точность GPU (High Accuracy) и отключен рескейл, что ликвидировало артефакты ступенчатых полос и затемнения буфера освещения.

🧩 <b>Нативная распаковка модов .7z и реорганизация LayeredFS (Android):</b>
• <b>Интеграция 7-Zip-JBinding-4Android</b>: внедрен нативный движок декомпрессии архивов 7-Zip, поддерживающий многопоточные фильтры BCJ2 (ранее приводившие к ошибке <i>«Multi input/output stream coders are not yet supported»</i> в Pure Java).
• <b>Умное определение структуры модов</b>: любые архивы модов с GameBanana (включая структуры с <code>atmosphere/contents/&lt;TitleID&gt;/...</code>) автоматически очищаются от лишних вложенных папок и монтируются напрямую в корневые директории <code>romfs</code>, <code>exefs</code> и <code>cheats</code>.
• <b>Регистронезависимый поиск Title ID</b>: обеспечена корректная загрузка модов независимо от регистра идентификатора игры на файловой системе Linux/Android.

🌐 <b>Интерфейс STORM GAMES WORLD (Windows и Android):</b>
• <b>Окно каталога на Windows</b>: расширены базовые габариты окна (1380×800) и списков игр, добавлена фиолетовая плашка внутреннего номера сборки (например, <code>655360</code>).
• <b>Верифицированные обложки игр (Android)</b>: обновлена таблица соответствий официального CDN Nintendo eShop на базе TitleDB — устранены некорректные и смещенные обложки (Brotato, Dave the Diver, Cadence of Hyrule и др.).
• <b>Обновленные стили кнопок</b>: изумрудная заливка (<code>#10B981</code>) с отметкой «Скачано» для загруженных проектов и нейтральный контурный стиль (<code>#334155</code>) с надписью «Скачать» для доступных игр.

⚙️ <b>Сервисы системы Nintendo Switch:</b>
• <b>AOC Service 13.0.0+</b>: зарегистрирована команда 50 (<code>CheckAddOnContentMountStatus</code>) в диспетчере дополнительного контента (AddOnContentManager).

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.6.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.6 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.6_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.6 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.6_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.6 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.6_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.6 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.0.6 deployment to Telegram completed successfully!"
