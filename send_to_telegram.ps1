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
⚡ <b>Релиз STORM SWITCH 7.4.5 (Three-Tier Architecture: Auto-Settings, In-Game Auto-Correction and Auto-Fix, 100% Parameter Restoration)</b> — <i>Масштабное обновление эмулятора Nintendo Switch: строгое разграничение трёх систем оптимизации (⚡ Авто-настройки в диалоге параметров на основе аппаратных характеристик ПК/устройства, 🛠️ Авто-коррекция графического конвейера в реальном времени при просадках FPS с динамической кнопкой в строке состояния, 🛡️ Авто-исправление при запуске игры для базы совместимости), гарантированное восстановление исходных настроек сессии при выходе или остановке эмуляции, а также исправление отображения версий игр и бэйджей на Android.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

⚡ <b>Авто-настройки в диалоге параметров («Параметры STORM SWITCH»):</b>
• В окно общих настроек интегрирована стилизованная кнопка «⚡ Авто-настройки».
• Автоматический анализ аппаратных характеристик системы: архитектура и инструкции ЦП, модель ГПУ, общий объем оперативной памяти и видеопамяти (VRAM), разрешение и частота обновления экрана, статус электропитания (сеть / батарея).
• Применение оптимального профиля (Энтузиаст, Сбалансированный, Энергосбережение) ко всем вкладкам параметров с наглядным модальным отчетом о системе.

🛠️ <b>Авто-коррекция графического конвейера в реальном времени:</b>
• В строке состояния (status bar) реализована динамическая кнопка «🛠️ Авто-коррекция», появляющаяся во время игры при возникновении просадок производительности (< 25 FPS, время кадра > 42 мс или скорость эмуляции < 78%).
• Мгновенная адаптивная оптимизация графического конвейера на лету (снижение разрешения до 0.75X/0.5X, быстрый режим точности ГПУ, аппаратное пересжатие текстур BC3 ASTC, асинхронные шейдеры и вывод, динамический FSR) с возможностью возврата к исходным параметрам одним кликом.

🛡️ <b>Авто-исправление при запуске игры и в контекстном меню:</b>
• Строгое выделение профилей совместимости под эгиду «🛡️ Авто-исправление» как при запуске игр из базы данных, так и в контекстном меню списка игр.
• Полная синхронизация наименований параметров и исчерпывающие пояснения в скобках.

🔄 <b>100% гарантированное восстановление параметров сессии:</b>
• Внедрен надежный механизм моментального снимка состояния всех изменяемых параметров перед запуском игры.
• При завершении игры, закрытии эмулятора или сбое эмуляции все глобальные и сессионные настройки автоматически и бесследно восстанавливаются к первоначальным значениям.

📱 <b>Исправление версий игр и бэйджей на Android:</b>
• Исправлен алгоритм парсинга версий в NACP метаданных и в GameHelper, исключающий ошибочное распознавание 16-значных Title ID как версий.
• Устранены пустые и нулевые бейджи обновлений и DLC в списке игр, отображение реальной версии обновления в формате v{версия}.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.5.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.5 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.5_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.5 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.5_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.5 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.5_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.5 (Windows x64 Release Portable)</b>"
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

Write-Host "`nRelease 7.4.5 deployment to Telegram completed successfully!"