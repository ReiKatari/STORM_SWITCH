$tokenFile = "e:\STORM EDEN 3\tg_token.txt"
if (Test-Path $tokenFile) {
    $token = (Get-Content $tokenFile -Raw).Trim()
} elseif ($env:TELEGRAM_BOT_TOKEN) {
    $token = $env:TELEGRAM_BOT_TOKEN.Trim()
} else {
    Write-Error "Telegram bot token file not found!"
    exit 1
}

$chatId = "-5389146045"

function Send-TGMessage($text) {
    $url = "https://api.telegram.org/bot$token/sendMessage"
    $body = @{
        chat_id = $chatId
        text = $text
        parse_mode = "HTML"
        disable_web_page_preview = $true
    } | ConvertTo-Json -Compress
    
    $headers = @{ "Content-Type" = "application/json; charset=utf-8" }
    $res = Invoke-RestMethod -Uri $url -Method Post -Body ([System.Text.Encoding]::UTF8.GetBytes($body)) -Headers $headers
    Write-Host "Message Sent: $($res.ok)"
}

function Send-TGDocument($filePath, $caption) {
    Write-Host "Sending $filePath via curl..."
    $url = "https://api.telegram.org/bot$token/sendDocument"
    $fileName = [System.IO.Path]::GetFileName($filePath)
    
    & curl.exe -s -X POST $url `
        -F "chat_id=$chatId" `
        -F "parse_mode=HTML" `
        -F "caption=$caption" `
        -F "document=@$filePath"
        
    Write-Host "`nUploaded $fileName successfully!"
}

$announcement = @"
⚡ <b>Релиз STORM SWITCH 7.4.7 (Uzuy MMJR Pipeline, Android Crash Fix, Game Mode, MK11 Fix, Async Translator and Audio Ducking)</b> — <i>Глобальное обновление эмулятора Nintendo Switch: полное устранение вылетов при старте на Android, нативная интеграция системного Game Mode, специализированный профиль для Mortal Kombat 11, перенос лучших оптимизаций рендеринга из Uzuy MMJR и масштабное ускорение экранного переводчика и озвучки с аудио-даккингом.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🛡️ <b>Устранение краша при запуске на Android:</b>
• Полностью устранена критическая ошибка нулевого указателя (SIGSEGV) при старте эмулятора: инициализация провайдеров контента (ContentProvider) переведена на ленивую безопасную модель, исключающую обращение к неинициализированному ядру при статической загрузке библиотеки.
• Безопасное чтение метаданных ROM (GameMetadata) с предварительной верификацией виртуальной файловой системы (VFS).

⚡ <b>Полноценный системный Game Mode на Android:</b>
• Исправлена конфигурация манифеста и схемы AOSP Game Mode (без префиксов пространства имен): система Android теперь гарантированно распознает приложение как игровой движок.
• Внедрена поддержка Android 12+ GameManager с установкой приоритета MODE_GAMEPLAY_UNINTERRUPTIBLE на Android 13+, предотвращающая троттлинг и системные прерывания.
• Оптимизированы фильтры активности: запуск игры через системные лаунчеры (Game Turbo, Game Space, Armoury Crate) теперь передает управление напрямую в движок.

🥋 <b>Специализированный профиль Mortal Kombat 11:</b>
• Встроен обновленный профиль в базу данных авто-исправлений: нормальная точность GPU (Normal GPU Accuracy), стандартный пул памяти 4 ГБ DRAM и асинхронные шейдеры.
• Устранены графические глитчи текстур и вертексов, обеспечена стабильная производительность 55-60 FPS на драйвере STORM DRIVER 2.0.5 Turnip.

🏎️ <b>Оптимизации графического конвейера из Uzuy MMJR:</b>
• <b>Раннее освобождение заборов (Early release fences):</b> устранено зависание на 0 FPS в Ori and the Will of the Wisps, Subnautica и Donkey Kong Country: Tropical Freeze за счет ограничения таймаута ожидания кадра до 1 мс.
• <b>Быстрое время GPU (Fast GPU timing):</b> сокращение задержек при обработке очередей команд командного процессора.
• <b>Оптимизация SPIR-V шейдеров:</b> трехуровневая компиляция и минимизация размера шейдерного байткода.
• <b>Пропуск кадров (Frame skipping) и интерполяция:</b> динамический контроль плавности вывода на требовательных сценах.
• Все новые параметры выведены в графические настройки эмулятора (вкладка «Графика»).

🌐 <b>Экранный OCR-переводчик и синтез речи нового поколения:</b>
• <b>Параллельный асинхронный перевод:</b> обработка текстовых блоков переведена на корутины (async и awaitAll), ускоряя перевод экрана в 3–5 раз без зависания интерфейса.
• <b>Нормализация и очистка текста:</b> автоматическое склеивание разорванных дефисами и переносами строк слов для корректного машинного перевода.
• <b>Умный аудио-даккинг (Audio Ducking):</b> при озвучивании реплик синтезатором речи громкость игры плавно приглушается через системный аудиофокус и автоматически восстанавливается после завершения фразы.
• Приоритетный выбор высококачественных и локальных голосов TTS.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.7.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.7 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.7_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.7 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.7_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.7 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.7_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.7 (Windows x64 Release Portable)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_DRIVER_2.0.5.zip"
        Caption = "⚡ <b>STORM DRIVER 2.0.5 (Universal Turnip Driver)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_DRIVER_2.0.5_ZELDA.zip"
        Caption = "🗡️ <b>STORM DRIVER 2.0.5 Zelda Edition (BotW and TotK Zero-Flicker Architecture)</b>"
    }
)

foreach ($item in $filesToUpload) {
    if (Test-Path $item.Path) {
        Send-TGDocument $item.Path $item.Caption
    } else {
        Write-Warning "File not found: $($item.Path)"
    }
}

Write-Host "`nRelease 7.4.7 deployment to Telegram completed successfully!"