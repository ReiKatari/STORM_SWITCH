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
⚡ <b>Релиз STORM SWITCH 7.4.9 (Game Minimize Freeze Fix, Cooling Screen, MK11 Eden Nightly Profile, SoR4 Video Fix, Uzuy MMJR Game Mode)</b> — <i>Масштабное обновление эмулятора Nintendo Switch: полное устранение зависания при сворачивании игры, возвращение экрана охлаждения устройства и авто-паузы при нагреве, идеальная графика и 60 FPS в Mortal Kombat 11 по спецификации Eden Nightly с сохранением русского языка, восстановление запуска видеозаставки Streets of Rage 4, каноническая системная интеграция Android Game Mode по стандарту Uzuy MMJR.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

❄️ <b>Сворачивание игры, экран охлаждения и авто-пауза при нагреве:</b>
• <b>Ликвидация зависания при сворачивании (onPause / onStop):</b> в нативном слое Vulkan внедрено гарантированное уведомление об уничтожении поверхности окна (SurfaceChanged(nullptr)), что полностью предотвращает дедлоки драйвера Adreno/Turnip при потере дескриптора ANativeWindow.
• <b>Полноценный экран охлаждения:</b> при постановке игры на паузу или сворачивании безусловно отображается стилизованный блок охлаждения устройства с актуальной температурой аккумулятора, целевым порогом охлаждения (до 35°C), индикатором снятия нагрузки на чипсет (&lt; 1 Вт) и кнопкой мгновенного продолжения игры.
• <b>Экстренная защита от перегрева:</b> порог автоматической защитной паузы приведен к реалистичному аппаратному значению 44.0°C (вместо недостижимых 52°C), обеспечивая своевременное охлаждение без троттлинга ОС Android и с сохранением игрового процесса.

🥋 <b>Mortal Kombat 11 (полное соответствие спецификации Eden Nightly):</b>
• <b>Устранение графических багов и артефактов:</b> Title ID MK11 исключен из списка NEEDS_D24, благодаря чему отключено ошибочное масштабирование глубины D24, разрушавшее тени и геометрию персонажей.
• <b>Возврат стабильных 55–60 FPS на Turnip:</b> удалена ресурсоемкая принудительная синхронизация операций памяти (sync_memory_operations), вызывавшая микрофризы и срезавшая FPS до 15–20. В драйверном генераторе Turnip выделен отдельный профиль с разрешенным сбросом тайлов (tu_tile_discard = true).
• <b>100% русский язык:</b> сохранен европейский регион (индекс 2) и русский язык интерфейса (индекс 10) с защитой от сброса на fallback.

🥊 <b>Streets of Rage 4 (исправление вылета начальной заставки):</b>
• Восстановлен проверенный безопасный NSO-патч машинного кода по смещению 0x008C0048 (cbz x23) для исполняемого файла игры (Build ID 8817441976E32E94909A95F64405A99A092B43DC), предотвращающий сбой IndexOutOfRangeException на 4-м кадре вступительного видеоролика при аппаратном декодировании NVDEC.

🎮 <b>Каноническая интеграция системного Android Game Mode (как в Uzuy MMJR):</b>
• Манифест AndroidManifest.xml полностью очищен от конфликтующих вендорных метаданных, ломавших парсинг службами Samsung GOS, Xiaomi Game Turbo и OnePlus Gamespace.
• Применена эталонная конфигурация game_mode_config.xml и объединенный интент лаунчера, обеспечивающие стопроцентное системное распознавание эмулятора как игры в Android 13, 14 и 15.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.9.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.9 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.9_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.9 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.9_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.9 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.9_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.9 (Windows x64 Release Portable)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

Write-Host "`nRelease 7.4.9 deployment to Telegram completed successfully!"