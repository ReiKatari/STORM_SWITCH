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
⚡ <b>Релиз STORM SWITCH 7.5.7 (SoR4 Gameplay Fix, Graphics Restore and Samsung Game Booster)</b> — <i>Крупное обновление эмулятора Nintendo Switch: полное исправление Streets of Rage 4 в геймплее, восстановление графики и текстур в Mortal Kombat 1, Mortal Kombat 11 и Zelda BotW, полная системная интеграция с Samsung Game Booster и ускорение загрузки списка игр.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Streets of Rage 4 (Windows и Android):</b>
• <b>Стабильный игровой процесс</b>: исправлены зависания на заставках и в меню. Обеспечена плавная работа вплоть до полного прохождения уровней без сбоев.
• <b>Корректировка профилей</b>: перевод NVDEC на программное декодирование FFmpeg на ЦП, отключение дедлоков асинхронного вывода и избыточной синхронизации памяти.

🎮 <b>Mortal Kombat 1 и Mortal Kombat 11:</b>
• <b>Mortal Kombat 1</b>: устранено исчезновение и деградация текстур костюмов и окружения через включение реактивной очистки кэша и высокую точность ГПУ.
• <b>Mortal Kombat 11</b>: полностью исправлены розовый фон и пропадание полигонов персонажей, вызванные регрессией точности.

🗡 <b>The Legend of Zelda: Breath of the Wild:</b>
• <b>Исправление физики и ландшафта</b>: строго зафиксирован пул 4 ГБ DRAM, исключающий проваливание Линка сквозь землю и рассинхрон Havok Physics.
• <b>Устранение черных текстур земли</b>: отключен агрессивный префетч шейдеров и сброс LRZ между буферами команд на Adreno 830.

❄️ <b>Охлаждение и пауза:</b>
• <b>Корректный расчет температуры</b>: целевая температура при паузе теперь строго минимум на 1°C ниже текущей температуры устройства.

✨ <b>Новые возможности и интерфейс:</b>
• <b>Диалог выбора Amiibo (Android)</b>: красивое контекстное окно в едином стиле всех 8 тем с выбором между онлайн-базой Amiibo и локальным .bin файлом.
• <b>Кнопка «Отмена» в окне авто-исправлений</b>: возможность прервать запуск игры прямо из диалога подтверждения.
• <b>Интеграция с Samsung Game Booster</b>: поддержка Samsung GOS, Game Tools, Game Home и Android 12+ GameManager для максимальной игровой производительности.
• <b>Мгновенная загрузка списка игр (Windows)</b>: кэширование версий файлов и дополнений сократило время сканирования библиотек в разы.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.7.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.7 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.7_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.7 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.7_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.7 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.7_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.7 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 7.5.7 deployment to Telegram completed successfully!"