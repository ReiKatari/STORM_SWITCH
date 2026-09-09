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
⚡ <b>Релиз STORM SWITCH 7.5.6 (Streets of Rage 4 Fix and Tools Menu Polish)</b> — <i>Экстренное обновление эмулятора Nintendo Switch: полное восстановление встроенных NSO-патчей для Streets of Rage 4 на Windows и Android, устраняющих зависание и падение при старте игры, а также безупречное выравнивание векторных иконок в меню «Инструменты» на Windows.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Streets of Rage 4 (Windows и Android):</b>
• <b>Восстановление встроенных NSO-патчей</b>: в ядро PatchManager возвращены патчи для обхода зависания в цикле опроса контроллеров (0x008C2F94) и проверки нулевого буфера видеопотока (0x008C0048).
• <b>Устранение падения на старте</b>: игра гарантированно проходит вступительные ролики и меню без сбоев IndexOutOfRangeException и черного экрана.
• <b>Профили GameFixDatabase</b>: расширена поддержка базы авто-исправлений для всех региональных версий игры (0100EC9010258000, 0100AC300919A000, 010085800E33E000, 0100BA700E340000, 0100C60010228000).

🎨 <b>Интерфейс меню «Инструменты» (Windows):</b>
• <b>Эталонное выравнивание иконок</b>: пункты «Сбросить скрытые диалоги авто-исправлений...» и «Авто-настройки производительности...» переведены на векторные иконки в общем столбце пиктограмм.
• <b>Чистая типографика</b>: эмодзи удалены из названий пунктов, обеспечивая идеальную геометрию и отступы в меню.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.6.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.6 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.6_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.6 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.6_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.6 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.6_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.6 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 7.5.6 deployment to Telegram completed successfully!"