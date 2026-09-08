$tokenFile = "e:\STORM EDEN 3\tg_token.txt"
if (Test-Path $tokenFile) {
    $token = (Get-Content $tokenFile -Raw).Trim()
} elseif ($env:TELEGRAM_BOT_TOKEN) {
    $token = $env:TELEGRAM_BOT_TOKEN.Trim()
} else {
    Write-Error "Telegram bot token file not found!"
    exit 1
}

$chatId = "-5389146045"

function Send-TGMessage($text) {
    $url = "https://api.telegram.org/bot$token/sendMessage"
    $body = @{
        chat_id = $chatId
        text = $text
        parse_mode = "HTML"
        disable_web_page_preview = $true
    } | ConvertTo-Json -Compress
    
    $headers = @{ "Content-Type" = "application/json; charset=utf-8" }
    $res = Invoke-RestMethod -Uri $url -Method Post -Body ([System.Text.Encoding]::UTF8.GetBytes($body)) -Headers $headers
    Write-Host "Message Sent: $($res.ok)"
}

function Send-TGDocument($filePath, $caption) {
    Write-Host "Sending $filePath via curl..."
    $url = "https://api.telegram.org/bot$token/sendDocument"
    $fileName = [System.IO.Path]::GetFileName($filePath)
    
    & curl.exe -s -X POST $url `
        -F "chat_id=$chatId" `
        -F "parse_mode=HTML" `
        -F "caption=$caption" `
        -F "document=@$filePath"
        
    Write-Host "`nUploaded $fileName successfully!"
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

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.3 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.3 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.3 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.3_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.3 (Windows x64 Release Portable)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

Write-Host "`nRelease 7.5.3 deployment to Telegram completed successfully!"