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
⚡ <b>Релиз STORM SWITCH 8.0.8 (Исправление вылетов Zelda, оптимизация Snapdragon 700, фикс Animal Well)</b> — <i>Устранение критических вылетов при запуске всех игр серии The Legend of Zelda, глубокая оптимизация производительности для Snapdragon 700 серии (Adreno 6xx), исправление артефактов рендеринга Animal Well, динамическая версия Android.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🗡️ <b>The Legend of Zelda — исправление вылетов при запуске:</b>
• <b>Tears of the Kingdom</b>: устранён дедлок GPU при запуске — удалены проблемные настройки <code>gpu_fence_behavior</code> и <code>enable_gpu_buffer_readback</code>, которые вызывали зависание конвейера GPU до обработки первых тиков.
• <b>Breath of the Wild</b>: удалена настройка <code>sync_memory_operations</code>, предназначенная для UE4, которая ломала DMA-синхронизацию движка Nintendo.
• <b>Link's Awakening и Echoes of Wisdom</b>: исправлен режим памяти с 8 ГБ на 4 ГБ — игры, рассчитанные на 4 ГБ Switch, вызывали повреждение кучи при эмуляции 8 ГБ.
• <b>Echoes of Wisdom</b>: исправлен Title ID игры, из-за чего авто-исправления не применялись.
• <b>buffer_cache.h</b>: добавлена защита от дедлока при массивных аллокациях буферов на этапе запуска игры (guard при <code>gpu_tick == 0</code>).

📱 <b>Snapdragon 700 серии — глубокая оптимизация:</b>
• <b>Adreno 6xx (616-642L)</b>: принудительно отключены EDS и VIDS в драйвере Vulkan — аппаратные CP-регистры Adreno 6xx вызывали краши при любом уровне Extended Dynamic State.
• <b>Все 4 пресета</b> (Стандарт, Быстрый, Нормальный, Точный): оптимизированы разрешение (0.5X-0.75X), VRAM, количество шейдерных потоков (2 вместо 4 для 2 Kryo Gold ядер), AA, анизотропия и режим памяти для Adreno 6xx.
• <b>Снижение нагрева</b>: ограничение 2 потоков компиляции шейдеров устраняет перегрев и микрофризы на устройствах с 2 производительными ядрами.

🎮 <b>Animal Well — исправление рендеринга:</b>
• <b>EDS3 по умолчанию</b>: устранены синие/фиолетовые полосы на NVIDIA Maxwell — глобальный параметр <code>dyna_state</code> переключён с EDS1 на EDS3.
• <b>Принудительный EDS3</b>: добавлен во все Title ID Animal Well в базе авто-исправлений.

📱 <b>Android — динамическая версия:</b>
• <b>Исправлен хардкод версии</b>: overlay-строка эмуляции теперь отображает актуальную версию вместо устаревшей <code>8.0.3</code>.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.8.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.8 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.8_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.8 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.8_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.8 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.8_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.8 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.0.8 deployment to Telegram completed successfully!"
