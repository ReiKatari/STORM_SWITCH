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
⚡ <b>Релиз STORM SWITCH 7.5.9 (Streets of Rage 4 60 FPS, AC Rogue DLC Unlock, Multi-Vendor Game Booster и Maximum Performance)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: полное устранение вылета и зависания Streets of Rage 4 на экране загрузки со звёздочкой, обеспечение плавных 60 FPS, исправление доступа к дополнению «Изгой» (Rogue) в меню Assassin's Creed: The Rebel Collection, универсальная поддержка игровых режимов всех вендоров смартфонов и ликвидация экстремальных задержек кадров (0 FPS / 200 мс).</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Streets of Rage 4 (исправление экрана загрузки и 60 FPS):</b>
• <b>Устранение зависания и вылета на звёздочке</b>: восстановлены и оптимизированы встроенные бинарные инструкции ARM64 в коде NSO, обходящие зацикливание при опросе устройств ввода и предотвращающие аварийное завершение при показе вращающейся звёздочки автосохранения.
• <b>Чистый пропуск роликов и 60 FPS</b>: автоматический бесконфликтный пропуск вступительных видеороликов сразу к титульному экрану и стабильная частота 60 кадров в секунду без дедлоков потока Vulkan.

🗡️ <b>Assassin's Creed: The Rebel Collection — доступ к DLC «Изгой» (Rogue):</b>
• <b>Монтирование внешних дополнений RomFS</b>: реализован надежный механизм резервного поиска (fallback) в диспетчере RomFSFactory и FSP-SRV, благодаря которому установленные DLC из NSP и XCI архивов корректно открываются для гостевого процесса.
• <b>Полноценный доступ в меню</b>: игра безошибочно находит и монтирует файлы дополнения «Изгой» весом 7.15 ГБ, убирая предупреждение о необходимости загрузки из eShop и открывая доступ к сюжетной кампании.
• <b>Сортировка индексов DLC</b>: упорядочивание списков дополнений исключает смещение идентификаторов контента.

🔋 <b>Универсальная поддержка Game Booster и игровых режимов Android:</b>
• <b>Мульти-вендорная интеграция</b>: реализован UniversalGameModeManager с адресной поддержкой игровых центров всех ведущих производителей смартфонов:
  — <b>Samsung</b>: Game Booster, Game Tools, Game Optimizing Service (GOS) и Gaming Hub.
  — <b>Xiaomi / POCO / Redmi</b>: Game Turbo и системная служба Joyose.
  — <b>OnePlus / OPPO / Realme</b>: ColorOS Game Space, HyperBoost и OPlus Games.
  — <b>Vivo / iQOO</b>: Ultra Game Mode и Multi-Turbo.
  — <b>ASUS ROG / Zenfone</b>: Armoury Crate и ROG GameCenter.
  — <b>Huawei / Honor</b>: Game Suite и Game Assistant.
• <b>AOSP Game Mode</b>: полная интеграция с Android 12+ / 13+ / 14+ / 15+ GameManager и GameState API, активация Sustained Performance Mode и минимальной задержки постобработки дисплея.
• <b>Широковещательные фильтры</b>: расширен манифест приложения для полной видимости системных пакетов на современных версиях Android.

⚡ <b>Устранение экстремальных задержек и просадок производительности:</b>
• <b>Аппаратное декодирование ASTC на ГПУ</b>: перевод декодирования сжатых текстур ASTC на вычислительные шейдеры видеокарты (Compute Shaders) по умолчанию вместо ресурсоемкого декодирования на ЦП.
• <b>Устранение задержки 200 мс</b>: отключены избыточные барьеры синхронизации памяти и агрессивная реактивная очистка, приводившие к замиранию конвейера рендеринга и падению до 0 FPS.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.9.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.9 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.9_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.9 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.9_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.9 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.9_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.9 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 7.5.9 deployment to Telegram completed successfully!"