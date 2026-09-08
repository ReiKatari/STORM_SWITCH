$ErrorActionPreference = "Stop"
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
⚡ <b>Релиз STORM SWITCH 7.5.3 (Lossless Frame Generation, Docked Resolution Scale, Stitched Addons Detection and Full 6-Language Sync)</b> — <i>Крупное обновление эмулятора Nintendo Switch: автоматическая активация и двойной счетчик FPS для генерации кадров на Android, динамическое отображение разрешений с учетом Док-режима на Windows и Android, структурированный Менеджер дополнений с поддержкой вшитых DLC в файлах игр, устранение оверхеда трассировки Vulkan и 100% локализация на 6 языков.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🌀 <b>Генерация кадров (Lossless Frame Generation) на Android:</b>
• Автоматическое включение генерации кадров при установке файла Lossless.dll без необходимости ручного поиска параметров.
• Обновлен внутриигровой оверлей FPS: теперь наглядно отображаются как нативный FPS игры, так и результирующий FPS с генерацией кадров в скобках, например: <code>15 FPS (30 FPS)</code>.

🖥️ <b>Динамическое отображение разрешения (Windows и Android):</b>
• При включенном Док-режиме отображается реальное ТВ-разрешение с учетом множителя масштабирования (база 1080p, 2X = 2160p / 4K, 3X = 3240p / 6K, 4X = 4320p / 8K).
• При портативном режиме отображаются нативные разрешения (база 720p, 2X = 1440p / 2K, 3X = 2160p / 4K, 4X = 2880p).
• Мгновенное обновление значений на панели состояния и в меню настроек при переключении док-станции.

📦 <b>Менеджер дополнений и распознавание вшитых DLC (Stitched Games):</b>
• В окне «Менеджер дополнений» реализована строгая группировка и сортировка: Обновления игры -> Официальные DLC по Title ID -> Пользовательские моды, сквозная нумерация 1..N и интерактивная сортировка по клику на заголовки.
• В играх со вшитыми дополнениями (например, <code>(1G+3D)</code>) обеспечено безошибочное распознавание количества DLC в 5-й колонке списка игр и в окне «Свойства игры».

⚡ <b>Оптимизация Vulkan и устранение скрытой нагрузки:</b>
• По умолчанию отключено логирование вызовов Vulkan (gpu_log_vulkan_calls = false), устраняющее избыточный оверхед кольцевого буфера и высвобождающее ресурсы процессора.

🌐 <b>100% локализация на 6 основных языков:</b>
• Полная поддержка русского, английского, немецкого, французского, китайского и японского языков для расширенных настроек графики и подсказок.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.3 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.3 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.3 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.3 (Портативная версия для Windows x64)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

$httpClient.Dispose()
Write-Host "`nRelease 7.5.3 deployment to Telegram completed successfully!"