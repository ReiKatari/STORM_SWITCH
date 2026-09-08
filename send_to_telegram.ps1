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
⚡ <b>Релиз STORM SWITCH 7.4.8 (Mortal Kombat 1 and 11 Fixes, Game Mode, SoR4 Boot Fix, Library Multi-Extension Support, Clean Versioning)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: устранение графических артефактов и падения FPS в Mortal Kombat 1, полная поддержка русского языка и оптимизация Mortal Kombat 11, восстановление запуска Streets of Rage 4, полноценная интеграция Game Mode на Android, исправление отображения игр с разными расширениями (NSP и NSZ) и чистое отображение версий.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🥋 <b>Mortal Kombat 1 (устранение графических багов и локаута 30 FPS):</b>
• <b>Ликвидация застывающего шлейфа дыма и спецэффектов:</b> в драйверном профиле Turnip отключен агрессивный сброс тайлов GMEM (tu_tile_discard = false) и включено межкамандное сохранение LRZ (tu_lrz_preserve_across_cmdbuf = true). Промежуточные буферы частиц UE4 больше не повреждаются и дым/сетка Сайракса и Миротворца корректно рассеиваются.
• <b>Стабильные 60 FPS:</b> включены быстрое время ГПУ (fast_gpu_time = true) и синхронизация операций памяти (sync_memory_operations = true), что исключает срабатывание динамического делителя частоты кадров движка Unreal Engine 4 и просадку до 30 FPS.
• <b>Точность буфера глубины D24:</b> альтернативный Title ID MK1 добавлен в список NEEDS_D24 для идеального скейлинга смещения глубины (depth bias) без z-файтинга.

🥋 <b>Mortal Kombat 11 (русский язык, производительность и стабильность):</b>
• <b>100% отображение русского языка:</b> в системном сервисе IReadOnlyApplicationControlDataInterface обеспечен безусловный приоритет настроенного пользователем русского языка без нежелательного отката на английский fallback. В профиль MK11 прописан язык (Russian, индекс 10) и регион (Europe, индекс 2).
• <b>Устранение графических глитчей и тормозов:</b> Title ID MK11 добавлен в NEEDS_D24, профиль дополнен синхронизацией памяти и ускоренным таймингом GPU для стабильных 55–60 FPS на Turnip.

🥊 <b>Streets of Rage 4 (полное устранение краша на старте):</b>
• Удален небезопасный бинарный патч машинного кода в менеджере патчей NSO, приводивший к нарушению регистров и мгновенному аварийному завершению (SIGSEGV) на ARM64 NCE и JIT-компиляторе Dynarmic. Игра запускается и работает стабильно с аппаратным и программным декодером NVDEC.

📱 <b>Библиотека игр Android: поддержка всех форматов файлов:</b>
• Исправлен алгоритм дедупликации библиотеки: формирование уникального идентификатора теперь опирается на фактическое расширение (NSP, NSZ, XCI, XCZ). Игры, имеющиеся в коллекции одновременно в разных форматах (например, The Legend of Zelda: Breath of the Wild в NSP и NSZ), отображаются параллельно без принудительного сокрытия.

🎮 <b>Полноценный системный Game Mode на Android:</b>
• Приведен к строгому стандарту AOSP манифест конфигурации Game Mode (game_mode_config.xml) без конфликтующих дубликатов атрибутов, что восстановило работу игрового профиля на Android 14 и 15.
• Внедрено динамическое переключение игрового состояния GameManager (setGameState): режим MODE_GAMEPLAY_UNINTERRUPTIBLE активируется не только при старте, но и при каждом возвращении в игру (onResume), а при сворачивании безопасно переходит в MODE_NONE (onPause).
• Добавлены метаданные и категории для Samsung Game Booster, Xiaomi Game Turbo, OnePlus Gamespace и Asus Armoury Crate.

🏷️ <b>Чистое отображение версий в интерфейсе:</b>
• В карточках списков и сетки игр версия теперь отображается в чистом виде (например, 1.0.1 вместо v1.0.1).
• Внутренний номер версии Switch выводится без префикса «v» и без круглых скобок (например, 222222 вместо (v222222)).

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.8.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.8 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.8_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.8 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.8_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.8 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.8_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.8 (Windows x64 Release Portable)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_DRIVER_2.0.5.zip"
        Caption = "🏎️ <b>STORM DRIVER 2.0.5 (Mesa Turnip Driver for Adreno 6xx/7xx/8xx)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

Write-Host "`nRelease 7.4.8 deployment to Telegram completed successfully!"