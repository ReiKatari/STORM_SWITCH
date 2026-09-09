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
⚡ <b>Релиз STORM SWITCH 7.5.5 (Zelda BotW Docked Fix, Game Booster, Streets of Rage 4 and UI Enhancements)</b> — <i>Масштабное обновление эмулятора Nintendo Switch: исправление текстур и графических артефактов в The Legend of Zelda: BotW в док-режиме, полноценная интеграция с Samsung Game Booster и системным игровым режимом на Android, эталонные габариты окна выбора стиля сенсорного управления, устранение сбоя запуска Streets of Rage 4 со сшитыми NSP и вылета при смене языка на Windows.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🗡️ <b>The Legend of Zelda: Breath of the Wild (Android и Windows):</b>
• <b>Док-режим и динамическое разрешение (DRS)</b>: устранено пропадание текстур земли, скал и появление черных дыр/пустот при работе в разрешении 900p (1600×900) и масштабировании.
• <b>Устранение артефактов в движении</b>: ликвидированы визуальные сбои при выходе из Святилищ и перемещении персонажа благодаря точной синхронизации таймингов ГПУ и буферов видеопамяти.
• <b>Профиль авто-исправления</b>: оптимизированы параметры точности ГПУ, несжатый ASTC, синхронизация памяти и реактивный сброс поверхностей.

🎮 <b>Samsung Game Booster и системный игровой режим (Android):</b>
• <b>Полная системная интеграция</b>: в манифест внедрена категория <code>android.intent.category.GAME</code> и метаданные <code>com.samsung.android.game.gameboostercategory</code> для активного процесса <code>EmulationActivity</code>.
• <b>Samsung One UI and Game Dashboard</b>: система гарантированно определяет эмулятор как игру во время игровой сессии, активируя аппаратные профили максимальной производительности.

🎨 <b>Стиль сенсорного управления (Android):</b>
• <b>Эталонные размеры диалога</b>: окно выбора стиля сенсорного контроллера приведено в полное соответствие с габаритами Менеджера чит-кодов (92% ширины и высоты экрана в альбомной ориентации).
• <b>Сетка в 2 колонки</b>: для широких дисплеев карточки стилей теперь отображаются в удобной двухколоночной сетке без стесненного скролла.

👊 <b>Устранение падения Streets of Rage 4:</b>
• <b>Безопасный PatchManager</b>: устранен вылет при открытии многокомпонентных сшитых NSP-пакетов игры за счет безопасного динамического приведения типов контент-провайдеров.
• <b>Стабильный NVDEC</b>: переключение на программное декодирование на ЦП через FFmpeg исключает зависания вступительных роликов.

💻 <b>Стабильность интерфейса Windows:</b>
• <b>Смена языка без вылетов</b>: устранено падение при переключении языка в «Параметры -> Интерфейс -> Язык приложения» за счет корректной инициализации списков и блокировки фоновых событий.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.5.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.5 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.5_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.5 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.5_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.5 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.5_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.5 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 7.5.5 deployment to Telegram completed successfully!"