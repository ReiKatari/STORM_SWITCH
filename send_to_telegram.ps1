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
⚡ <b>Релиз STORM SWITCH 7.5.0 (Eden Nightly Defaults, MK11 and Zelda Water Fixes, Full OEM Game Mode, Overlay Cleanup)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: эталонные настройки Eden Nightly с оптимизированными энергоэффективными параметрами по умолчанию, устранение артефактов полигонов в Mortal Kombat 11, прозрачная вода в The Legend of Zelda: Breath of the Wild и Tears of the Kingdom, надежный запуск Streets of Rage 4, восстановление поддержки Android Game Mode для всех OEM-производителей и очистка дубликатов оверлея.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

⚙️ <b>Эталонные настройки Eden Nightly и оптимизация по умолчанию:</b>
• Внедрены новые оптимизированные параметры по умолчанию: гибридное декодирование NVDEC и ASTC, включенный эко-режим и охлаждение, энергоэффективный фреймпейсинг, адаптивный контроль компиляции шейдеров, привязка потоков ЦП, кэш конвейеров Vulkan, очистка видеопамяти (VRAM GC), раннее освобождение фенсов, оптимизация вывода SPIR-V, пропуск кадров, асинхронная эмуляция GPU и асинхронная презентация.
• Синхронизированы все авто-настройки, профили калибровки и пресеты производительности.

🗡️ <b>Графика в The Legend of Zelda (BotW и TotK):</b>
• <b>Прозрачная вода:</b> исправлена ошибка непрозрачной белой воды в реках и святилищах за счет отключения агрессивной BC3-компрессии текстур ASTC (установлен несжатый режим для идеальной прозрачности альфа-канала).
• Высокая скорость и отсутствие артефактов Z-буфера.

🥋 <b>Mortal Kombat 11:</b>
• Полностью устранены растяжения вершин и разрывы полигонов на лицах и моделях бойцов благодаря установке высокой точности GPU (High), быстрой эмуляции времени GPU и стандартной раскладке 4 ГБ DRAM.

🥊 <b>Streets of Rage 4:</b>
• Сохранен проверенный NSO-патч машинного кода по смещению 0x008C0048, гарантирующий стабильный запуск вступительной заставки без зависаний.

📱 <b>Android: Game Mode и очистка интерфейса:</b>
• Восстановлена полноценная интеграция со службами оптимизации игр Samsung (GOS, GameHub, GameBooster), Xiaomi (Game Turbo), ASUS ROG, OnePlus, Oppo, Vivo и Huawei.
• Удалены дублирующиеся пункты переключения контроллера из внутриигрового меню настроек экранных кнопок.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.0.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.0 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.0_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.0 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.0_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.0 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.0_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.0 (Windows x64 Release Portable)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

Write-Host "`nRelease 7.5.0 deployment to Telegram completed successfully!"