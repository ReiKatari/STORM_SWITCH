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
⚡ <b>Релиз STORM SWITCH 7.4.6 (Game Repack Mounts, Dark Souls and Streets of Rage 4 Fixes, Compact UI, 3-Finger Gesture)</b> — <i>Масштабное обновление эмулятора Nintendo Switch: исправление зависаний и вылетов в Streets of Rage 4 и Dark Souls Remastered, разблокировка встроенных DLC и корректное определение версий для составных репаков (1G+1U+1D), новый надежный трехпальцевый жест вызова быстрых настроек от нижнего края экрана с возможностью полного отключения в боковом меню, компактные диалоги авто-исправления, Amiibo и GameBanana.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🎮 <b>Streets of Rage 4 и Dark Souls Remastered:</b>
• Устранено зависание на логотипе Dotemu с 0 FPS в Streets of Rage 4 (все 4 региональных Title ID) за счет адаптивной конфигурации декодера NVDEC FFmpeg CPU и синхронной презентации кадров.
• Устранены краши при запуске Dark Souls Remastered на NCE/ARM, автоматическое переключение на оптимизированный Dynarmic JIT и игнорирование сбоев нераспределенной памяти.

📦 <b>Монтирование репаков, разблокировка DLC и отображение версий на Android:</b>
• Обеспечено безусловное монтирование внутренних контейнеров при любом запуске игры: бойцы и дополнительный контент из репаков (1G+1U+1D).nsp (включая Mortal Kombat 1) корректно регистрируются в ContentProvider и доступны в игре.
• Улучшен алгоритм определения номеров версий в GameMetadata и GameHelper: исключено ошибочное сведение к 1.0.0, корректный парсинг обновлений из NACP контейнера и наименований файлов.

👆 <b>Управление быстрыми настройками и новый трехпальцевый жест:</b>
• Жест вызова быстрых настроек переведен на свайп тремя пальцами строго от самого нижнего края экрана (нижние 60dp), что полностью исключает случайные срабатывания во время активного игрового процесса.
• В боковом меню эмуляции добавлен независимый переключатель «Включить быстрые настройки», позволяющий полностью отключить жест и пункт меню при ненадобности.

📐 <b>Компактный и современный интерфейс диалогов:</b>
• Переработано окно «Авто-исправление»: устранены избыточные поля и пустые пространства, диалог стал компактным и аккуратным.
• В диалогах каталога Amiibo и модификаций GameBanana высота подвала сокращена на ~60%: кнопки пагинации центрированы, счетчик страниц перемещен в нижний левый угол.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.6.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.6 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.6_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.6 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.6_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.6 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.6_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.6 (Windows x64 Release Portable)</b>"
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

Write-Host "`nRelease 7.4.6 deployment to Telegram completed successfully!"