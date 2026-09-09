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
⚡ <b>Релиз STORM SWITCH 7.5.8 (Streets of Rage 4 60 FPS, Launch Crash Fix, Samsung Game Booster and DLC Restore)</b> — <i>Экстренное обновление эмулятора Nintendo Switch: полное исправление вылетов при запуске игр на Android, плавные 60 FPS в Streets of Rage 4 с корректным переходом к игре, универсальная поддержка Samsung Game Booster для всех смартфонов, восстановление доступа к DLC без RomFS и строгая иерархическая сортировка дополнений.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Streets of Rage 4 (60 FPS и пропуск заставок):</b>
• <b>Устранение зависаний видео и 0 FPS</b>: удален поврежденный NSO-патч инструкций ARM64 и перенастроена эмуляция NVDEC (отключена), что позволяет мгновенно и чисто миновать вступительные видеоролики без дедлоков и падений частоты кадров.
• <b>Плавный геймплей</b>: стабильные 60 FPS в игровом процессе на всех региональных версиях игры.

📱 <b>Исправление запуска игр на Android:</b>
• <b>Устранение аварийного завершения</b>: ликвидирован вылет при старте любой игры в версии 7.5.7, вызванный десериализацией Parcelable в Bundle на современных версиях Android.
• <b>Безопасная очистка ресурсов</b>: добавлены защитные блоки перехвата исключений при завершении сессии эмуляции.

🔋 <b>Универсальный Samsung Game Booster:</b>
• <b>Поддержка всех моделей и версий One UI</b>: обеспечена надежная работа Game Booster, Game Tools и Game Plugins на всех устройствах Samsung без сбоев безопасности SecurityException.
• <b>Безопасные системные вызовы</b>: исключены некорректные вызовы внутренних интерфейсов Knox, задействован стандартный Android 12+ GameManager API с безопасными широковещательными событиями.

📦 <b>Восстановление доступа к DLC и порядок дополнений:</b>
• <b>Доступность DLC без RomFS</b>: снято ограничение нулевого размера RomFS в диспетчере контента, благодаря чему игры (например, Assassin's Creed Rebel Collection) корректно видят все лицензионные дополнения и языковые пакеты внутри самой игры.
• <b>Идеальный порядок дополнений</b>: в интерфейсе Windows и Android сначала строго отображаются обновления (Update), затем дополнения строго по порядковому номеру (#1, #2, #3...), и в конце пользовательские модификации (Mods).

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.8.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.8 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.8_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.8 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.8_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.8 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.8_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.8 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 7.5.8 deployment to Telegram completed successfully!"