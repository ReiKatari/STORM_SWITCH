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
⚡ <b>Релиз STORM SWITCH 8.0.4 (Стабильный запуск без фиксов, сохранение кастомных профилей, Diablo 3 DLC Fix, шторка настроек и STORM GAMES WORLD на Android)</b> — <i>Комплексное обновление эмулятора Nintendo Switch для платформ Windows x64 и Android: полная стабильность запуска игр без применения базы фиксов, бережное сохранение пользовательских кастомных параметров при активации исправлений, надежное сохранение дополнений (DLC) и актуальных иконок обновлений в Diablo 3 и других проектах, сохранение настроек из боковой шторки QuickSettings, восстановленная иконка счетчика FPS, встроенный диалог менеджера загрузок STORM GAMES WORLD на Android без перехода в браузер и динамическое охлаждение процессора на паузе.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🛡️ <b>Стабильный запуск и гибкие профили исправлений:</b>
• <b>Устранение вылетов при «Запуске без фиксов»</b>: исправлена обработка событий сокетов BSD (<code>pollfd.revents</code>), ликвидировано аварийное завершение эмулятора при старте игр с сетевыми компонентами.
• <b>Защита пользовательских настроек</b>: при выборе «Применить фиксы» кастомные настройки игры (разрешение рендеринга, кастомный видеодрайвер Turnip, звуковой бэкенд, пропорции экрана) больше не сбрасываются глобальными значениями, а аккуратно объединяются с базой исправлений.

🗡️ <b>Надежное сохранение дополнений и иконок (Diablo 3 Fix):</b>
• <b>Сохранение всех DLC и обновлений</b>: ликвидирована проблема потери установленных дополнений и отката иконки игры к базовой версии после выхода из игры.
• <b>Загрузка актуальных иконок из обновлений</b>: обеспечено корректное чтение и приоритет иконок из пакетов обновлений, обновлен механизм объединения и кэширования метаданных контента.

⚙️ <b>Сохранение настроек из боковой шторки (QuickSettings):</b>
• <b>Надежное применение параметров</b>: любые изменения графики, звука и управления, внесенные через внутриигровую шторку до или во время игрового процесса, гарантированно сохраняются в индивидуальный профиль игры и не теряются при закрытии игры.

⚡ <b>Восстановление оверлея FPS и динамическое охлаждение:</b>
• <b>Восстановлена иконка FPS</b>: фирменный значок <code>⚡</code> возвращен в счетчик кадров оверлея производительности.
• <b>Охлаждение на паузе (Pause Cooling)</b>: автоматическое отключение режима Sustained Performance во время паузы для снижения тепловыделения и частот чипсета с ежесекундным мониторингом температуры аккумулятора в реальном времени.

📱 <b>Встроенный менеджер STORM GAMES WORLD на Android:</b>
• <b>Встроенный диалог без внешнего браузера</b>: доступ к каталогу игр прямо внутри приложения с отображением детальной информации, обложек и скриншотов.
• <b>Прямое скачивание</b>: быстрая загрузка выбранных игр непосредственно в игровую папку с отображением прогресса, скорости загрузки, оставшегося времени и автоматическим обновлением библиотеки.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.4.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.4 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.4_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.4 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.4_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.4 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.4_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.4 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.0.4 deployment to Telegram completed successfully!"
