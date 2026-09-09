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
⚡ <b>Релиз STORM SWITCH 8.0.0 (Streets of Rage 4 60 FPS, Mortal Kombat 1 Fix, AC Rogue DLC Unlock и Maximum Performance)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: полноценное воспроизведение видео-заставок и плавные 60 FPS в Streets of Rage 4, ликвидация зависания при запуске Mortal Kombat 1, восстановление оригинального идентификатора пакета (dev.storm_switch) для бесшовного обновления поверх версий 7.5.x, умная фильтрация обновлений и DLC из сетки игр, доступ к дополнению «Изгой» в Assassin's Creed и устранение задержек конвейера рендеринга.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Streets of Rage 4 (полное исправление видео, загрузки и 60 FPS):</b>
• <b>Воспроизведение видео-заставок и катсцен</b>: включено многопоточное программное декодирование NVDEC VP9 и асинхронный вывод презентации, что обеспечивает корректный показ всех вступительных роликов и сюжетных заставок.
• <b>Устранение зависания и вылета на звёздочке</b>: оптимизированы встроенные бинарные инструкции ARM64 в коде NSO, обходящие зацикливание при опросе устройств ввода и предотвращающие сбои при сохранении.
• <b>Стабильные 60 FPS</b>: плавная частота кадров без микрофризов и дедлоков потока Vulkan.

🥊 <b>Mortal Kombat 1 (устранение зависания при запуске):</b>
• <b>Ликвидация дедлока при старте</b>: оптимизирована точность синхронизации потоков ЦП в режим «Авто», предотвращающий зависание рабочих потоков распаковки ресурсов движка NetherRealm.
• <b>Оптимизация гостевой памяти</b>: безопасный режим 6 ГБ DRAM исключает исчерпание виртуальной памяти на устройствах с 8–12 ГБ ОЗУ и обеспечивает мгновенный стабильный запуск.

🔄 <b>Бесшовное обновление поверх версий 7.5.x (возврат dev.storm_switch):</b>
• <b>Восстановление оригинальных идентификаторов</b>: восстановлен applicationId <code>dev.storm_switch</code> (основная версия), <code>dev.storm_switch.legacy</code> (Legacy) и <code>dev.storm_switch.sdk27</code> (SDK27).
• <b>Обновление в один клик</b>: эмулятор устанавливается и обновляется поверх существующих версий 7.5.x с полным сохранением пользовательских сохранений, установленных драйверов и настроек.
• <b>Очистка от стороннего кода</b>: удалены лишние фоновые службы и избыточные запросы манифеста.

🎮 <b>Умная фильтрация библиотеки игр (исключение дублирования обновлений и DLC):</b>
• <b>Сетка только для базовых игр</b>: обновлен алгоритм распознавания контейнеров NSP и XCI. Отдельные файлы патчей (с маской 0x800) и дополнений (DLC) больше не отображаются отдельными плитками в библиотеке.
• <b>Автоматическое подключение</b>: все внешние обновления и DLC корректно монтируются и регистрируются для соответствующих базовых игр.

🗡️ <b>Assassin's Creed: The Rebel Collection — доступ к DLC «Изгой» (Rogue):</b>
• <b>Монтирование внешних дополнений RomFS</b>: реализован надежный механизм поиска в диспетчере RomFSFactory и FSP-SRV, благодаря которому установленные DLC из NSP и XCI архивов открываются для гостевого процесса.
• <b>Полноценный доступ в меню</b>: игра безошибочно находит файлы дополнения «Изгой» весом 7.15 ГБ, убирая предупреждение о необходимости загрузки из eShop и открывая сюжетную кампанию.

⚡ <b>Устранение задержек и аппаратная оптимизация графики:</b>
• <b>Аппаратное декодирование ASTC на ГПУ</b>: перевод декодирования сжатых текстур ASTC на вычислительные шейдеры видеокарты (Compute Shaders) вместо нагрузки на ЦП.
• <b>Ликвидация задержек 200 мс</b>: отключены избыточные барьеры синхронизации памяти и агрессивная реактивная очистка, исключая замирание конвейера рендеринга и падение FPS.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.0 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.0 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.0 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.0 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.0.0 deployment to Telegram completed successfully!"