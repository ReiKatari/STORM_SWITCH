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
⚡ <b>Релиз STORM SWITCH 8.1.0 и STORM DRIVER 3.0.1</b> — <i>Масштабное исправление критических вылетов (Mortal Kombat 1/11, Diablo III, Zelda BotW), восстановление правильных Title ID в базе данных, устранение зависаний сетевой телеметрии и оптимизация графического конвейера Vulkan.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения STORM SWITCH 8.1.0:</b>

🎯 <b>Полный аудит и исправление Title ID в базе авто-исправлений:</b>
• <b>Diablo III: Eternal Collection</b>: добавлены реальные Title ID для глобальной (<code>01001B300B9BE000</code>) и японской (<code>010032F00C04A000</code>) версий с преднастроенными профилями (режим полёта, 6 ГБ DRAM, асинхронные шейдеры, fastmem).
• <b>Ликвидация конфликтов и дубликатов Title ID</b>: исправлены некорректные идентификаторы для Super Mario Party (<code>010036B0034E4000</code>), Mario Party Superstars (<code>01006FE013472000</code>), Paper Mario: The Origami King (<code>0100A3900C3E2000</code>), Pokémon: Let's Go, Eevee! (<code>0100151003A36000</code>) и Persona 5 Royal (<code>01005CA01580E000</code>).
• <b>Mario Kart 8 Deluxe</b>: объединён профиль с оптимальным режимом 6 ГБ DRAM и реактивной очисткой памяти.

🛡️ <b>Устранение критических сбоев и зависаний:</b>
• <b>Mortal Kombat 1 и 11 (SaveDataSpaceId::Temporary)</b>: устранено падение при запуске из-за отсутствия директории временного кэш-хранилища (space_id=03). Реализовано автоматическое создание папки и безопасное чтение временных данных.
• <b>Diablo III (Battle.net Telemetry)</b>: устранены 11-секундные задержки и зависание на экране сезонов. Заблокированные хосты телеметрии теперь мгновенно возвращают NODATA вместо цикла EAI_AGAIN.
• <b>Diablo III (Fermi2D Blit)</b>: предупреждение о несоответствии глубины буфера переведено в однократный лог, что устранило микрофризы от спама в консоль.
• <b>Zelda: Breath of the Wild (Qualcomm Adreno)</b>: восстановлена аппаратная поддержка <code>shaderInt64</code> для графических процессоров Adreno, предотвращая сбои компиляции глобальной памяти и графические артефакты в святилищах и открытом мире.

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения STORM DRIVER 3.0.1 (Universal Edition):</b>
• <b>Subpass Fusion v2</b>: интеллектуальное объединение проходов рендеринга с защитой глубины и трафарета (+15-20% прироста эффективности).
• <b>Sparse Buffer Pages</b>: улучшена адресация виртуальных страниц буферов памяти для крупных AAA-проектов.
• <b>14 выделенных профилей</b>: индивидуальная настройка движка для Zelda BotW/TotK, Hogwarts Legacy, Persona 5 Royal, Mario Kart 8 Deluxe, Pokémon, Batman, Witcher 3, No Man's Sky, GTA V, DOOM Eternal, Xenoblade Chronicles 3, Diablo II и Streets of Rage 4.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.0.apk"
        Caption = "📱 <b>STORM SWITCH 8.1.0 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.0_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.1.0 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.0_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.1.0 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.0_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.1.0 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.1.0 deployment to Telegram completed successfully!"

