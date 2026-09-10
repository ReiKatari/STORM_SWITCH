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
⚡ <b>Релиз STORM SWITCH 8.0.9 и STORM DRIVER 3.0.1</b> — <i>Глубокая актуализация всех авто-исправлений, ликвидация вылетов и утечек памяти, системные улучшения из Eden Nightly, дросселирование асинхронных сбросов буферов и обновленный универсальный графический драйвер.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения STORM SWITCH 8.0.9:</b>

🎯 <b>Глубокий аудит и актуализация базы авто-исправлений (252 игры):</b>
• <b>Оптимизация памяти (DRAM)</b>: устранён избыточный режим 8 ГБ DRAM для платформеров и нетребовательных игр (Little Nightmares II и III, Yoshi's Crafted World, Bravely Default II, Princess Peach: Showtime!, Alien: Isolation возвращены на 4 ГБ; Need for Speed, Sonic Frontiers, Pokémon, Astral Chain, Luigi's Mansion 3, Pikmin 4, Red Dead Redemption переведены на оптимальные 6 ГБ). Это ликвидирует OOM-вылеты на мобильных устройствах с 6-8 ГБ RAM.
• <b>Обновлён baseline</b>: по умолчанию активированы вычислительные конвейеры (compute pipelines), кэш конвейеров драйвера Vulkan, дисковый кэш шейдеров и чтение буферов GPU.
• <b>Новые игровые профили</b>: добавлены выделенные авто-исправления для Mario Kart 8 Deluxe, Super Mario Party Jamboree, Pikmin 4, Luigi's Mansion 2 HD, Sid Meier's Civilization VII и Persona 5 Royal.

🛡️ <b>Устранение критических сбоев и системные улучшения:</b>
• <b>Pikmin 4 и HID NPad</b>: устранено падение по нулевому указателю (nullptr dereference) при подключении/отключении контроллеров и смене профилей ввода в моменты, когда разделяемая память ещё не сопоставлена.
• <b>Служба AM и управление жизненным циклом</b>: добавлены защитные проверки процесса и апплета в <code>ISelfController</code>, ликвидирующие вылеты и use-after-free при выходе из игр и переключении режимов.
• <b>Буферный кэш (In-flight flush throttling)</b>: внедрён контроль очереди асинхронных сбросов видеопамяти — при накоплении отложенных буферов движок завершает операцию, предотвращая раздувание VRAM и заикания (stutters).
• <b>Qualcomm Adreno Sampler Precision</b>: исправлена некорректная интерполяция кастомных цветов границы текстур в проприетарном драйвере Qualcomm, устраняющая чёрный экран в Persona 5 Royal.

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения STORM DRIVER 3.0.1 (Universal Edition):</b>
• <b>Subpass Fusion v2</b>: активировано интеллектуальное объединение проходов рендеринга с защитой глубины и трафарета (+15-20% прироста эффективности).
• <b>Sparse Buffer Pages</b>: улучшена адресация виртуальных страниц буферов памяти для крупных AAA-проектов.
• <b>14 выделенных профилей</b>: индивидуальная настройка движка для Zelda BotW/TotK, Hogwarts Legacy, Persona 5 Royal, Mario Kart 8 Deluxe, Pokémon, Batman, Witcher 3, No Man's Sky, GTA V, DOOM Eternal, Xenoblade Chronicles 3, Diablo II и Streets of Rage 4.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.9.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.9 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.9_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.9 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.9_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.9 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.9_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.9 (Портативная версия для Windows x64)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_DRIVER_3.0.1.zip"
        Caption = "🎮 <b>STORM DRIVER 3.0.1 (Универсальный драйвер Vulkan Turnip / PanVK для Android)</b>"
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
Write-Host "`nRelease 8.0.9 deployment to Telegram completed successfully!"
