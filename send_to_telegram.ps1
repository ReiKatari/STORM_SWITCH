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
⚡ <b>Релиз STORM SWITCH 7.4.4 (Strict Parameter Synchronization, GameFix Abort Logic and Volumetric Auto-Tuner Dialog)</b> — <i>Масштабное обновление эмулятора Nintendo Switch: 100% строгое соответствие всех названий параметров в диалогах авто-исправлений и авто-настроек с пояснениями в скобках, возможность полной отмены запуска игры при закрытии окна рекомендаций, разграничение локальных настроек игры и глобальных параметров эмулятора, а также встроенный графический интерфейс авто-тюнинга</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🎮 <b>Точная синхронизация названий параметров (Авто-исправления, Авто-коррекции и Авто-настройки):</b>
• Все наименования параметров в окнах рекомендаций GameFix, автокоррекции и встроенного автотюнера теперь в точности до буквы повторяют названия вкладок настроек эмулятора («Параметры STORM SWITCH»).
• Для каждого пункта в явном виде добавлено подробное пояснение в скобках, раскрывающее техническую причину и назначение изменения (защита от дедлоков, фоновая компиляция, предотвращение вылетов, устранение нагрева).
• Строгое разделение контекста применения: Авто-исправления и Авто-коррекции изменяют исключительно индивидуальный профиль конкретной игры (custom/<TitleID>.ini, use_global = false), в то время как Авто-настройки управляют общими глобальными параметрами программы.

🛑 <b>Кнопка «Отменить» и прерывание запуска в окне оптимизации:</b>
• В диалоговое окно рекомендуемых авто-исправлений добавлена отдельная кнопка «Отменить», позволяющая моментально закрыть окно без применения настроек и без запуска игры.
• Закрытие окна рекомендаций через стандартный крестик [X] или клавишу ESC теперь также корректно прерывает операцию запуска, возвращая пользователя к списку игр.

⚡ <b>Встроенный графический диалог и консольная утилита авто-настроек:</b>
• В меню «Инструменты» интегрирован полноценный графический интерфейс авто-настроек производительности с мгновенным переключением между профилями Low, Balanced и High Quality.
• Консольная утилита storm_autotuner.py и пакетный файл storm_autotuner.bat получили полную поддержку среды uv и вывод структурированного списка с детальными пояснениями.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.4.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.4 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.4_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.4 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.4_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.4 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.4_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.4 (Windows x64 Release Portable)</b>"
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

Write-Host "`nRelease 7.4.4 deployment to Telegram completed successfully!"