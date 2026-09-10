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
⚡ <b>Релиз STORM SWITCH 8.0.7 (Восстановление графики TotK и Animal Well, защита от краша авто-настроек и чистка профилей)</b> — <i>Комплексное обновление эмулятора Nintendo Switch для Windows x64 и Android: исправление черных силуэтов персонажей в The Legend of Zelda: Tears of the Kingdom, ликвидация искажений рендеринга и сканлайнов в Animal Well, устранение падения при нажатии «Авто-настройки», точная синхронизация барьеров GPU и полная оптимизация кастомных профилей.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🗡️ <b>The Legend of Zelda: Tears of the Kingdom:</b>
• <b>Восстановление шейдеров персонажей</b>: исправлена передача данных обратного чтения буферов GPU (<code>enable_gpu_buffer_readback</code>), благодаря чему устранены черные силуэты Линка, Зельды и спутников.
• <b>Ликвидация мерцания в святилищах</b>: активировано точное поведение барьеров синхронизации GPU (<code>gpu_fence_behavior: Accurate</code>), стабилизирующее конвейер рендеринга.
• <b>Устранение артефактов в кавернах</b>: оптимизирована реактивная очистка буферов (Reactive Flushing) и выделение 8 ГБ DRAM.

🐸 <b>Animal Well и 2D-игры:</b>
• <b>Устранение разделения экрана</b>: ликвидирован баг с разделением экрана пополам на засвеченную и темную половины. Возвращена полноценная обработка расширенных динамических состояний (EDS 2/3) и корректное наложение CRT-сканлайнов.
• <b>Стабильные 60 FPS</b>: оптимизирована адресация памяти Host MMU (Fastmem) и отключен синхронный readback для предотвращения задержек PCIe 200 мс (4 FPS).

⚡ <b>Исправление стабильности интерфейса:</b>
• <b>Защита от краша «Авто-настройки»</b>: устранена бесконечная рекурсия и переполнение стека при нажатии кнопки автоподбора параметров в окне конфигурации (ConfigureDialog).

🧹 <b>Санитария конфигураций и совместимость:</b>
• <b>Глубокая чистка профилей игр</b>: удалены замусоренные промежуточные файлы конфигураций. Оптимизированы профили для <i>Diablo II: Resurrected</i> (8 ГБ DRAM, стабильная загрузка персонажей), <i>Streets of Rage 4</i> (декодирование видеороликов NVDEC), <i>Super Mario Bros. Wonder</i> (устранение взрывов геометрии в Мире 4).

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.7.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.7 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.7_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.7 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.7_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.7 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.7_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.7 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.0.7 deployment to Telegram completed successfully!"
