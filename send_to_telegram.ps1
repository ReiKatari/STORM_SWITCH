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
⚡ <b>Релиз STORM SWITCH 7.5.4 (Streets of Rage 4 Fix, Stability and UI Polish)</b> — <i>Важное обновление эмулятора Nintendo Switch: полноценная поддержка и запуск Streets of Rage 4 на Windows и Android, устранение критических вылетов при остановке эмуляции и смене языка интерфейса на Windows, обновленные быстрые настройки на Android.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Полноценный запуск Streets of Rage 4 (Windows и Android):</b>
• <b>Dynarmic (Windows)</b>: исправлена обработка сбоев выполнения (NoExecuteFault) при вызове нулевых указателей функций — безопасный возврат в регистр LR и сброс регистров X0/X1.
• <b>NCE (Android)</b>: исправлена обработка префетч-абортов (HandleFailedGuestFault) — эмулятор восстанавливает адрес возврата из LR с валидацией диапазона адресов вместо некорректного смещения PC.
• <b>Game Fix Database</b>: для всех 5 региональных версий Streets of Rage 4 добавлены оптимизированные профили (игнорирование прерываний ЦП, гибридный NVDEC, асинхронная презентация, синхронные операции с памятью, реактивный сброс кэша).

🛡️ <b>Устранение критических сбоев (Windows):</b>
• <b>Остановка эмуляции («Стоп»)</b>: устранено падение при закрытии игры — добавлена защита от повторного входа, отключение дублирующих сигналов закрытия окна, перенос обновления списка игр после полной остановки потока и защита от сбоев кэша запросов (QueryCache).
• <b>Смена языка интерфейса</b>: исправлен вылет при переключении языка в «Параметры -> Интерфейс -> Язык приложения» — правильный порядок инициализации переводчика, безопасное обновление вкладок диалога настроек и защита от рекурсивных сигналов комбобокса.

📱 <b>Улучшения интерфейса Android:</b>
• В боковом меню игры пункт «Быстрые настройки» очищен от скобок и лишнего текста.
• Для пунктов быстрых настроек установлена новая отдельная иконка регуляторов (ic_tune), визуально отличающаяся от общих настроек игры.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.4.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.4 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.4_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.4 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.4_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.4 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.4_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.4 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 7.5.4 deployment to Telegram completed successfully!"