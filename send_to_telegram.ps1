$token = "8210884351:AAEh4VOWHViz2KF_oElAqEfrMPHlI5TWCjM"
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
⚡ <b>Релиз STORM SWITCH 7.4.2 (Streets of Rage 4 Fix, Real DXGI GPU Marketing Names, Bidirectional Footer Synchronization and NVDEC Decoding Options)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: полное устранение зависания после вступительных видеороликов (Streets of Rage 4 и другие игры), точное определение реальных коммерческих названий видеокарт NVIDIA и AMD через DXGI, полная двусторонняя синхронизация всех настроек в подвале и меню программы, а также обновленная терминология видеодекодирования</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🎮 <b>Полное устранение зависаний после видеороликов (Streets of Rage 4):</b>
• Устранена критическая блокировка синхронизации Host1x при завершении воспроизведения видеопотока.
• Внедрен автоматический сброс оставшихся инкрементов при закрытии устройств NVDEC и VIC (FlushSyncpoint), а также добавлен защитный таймаут ожидания синкпоинтов для предотвращения вечного зависания потоков гостевой эмуляции при уничтожении видеодекодера.
• Streets of Rage 4 теперь безупречно переходит от вступительной заставки к интерактивному главному меню и стабильному геймплею на полной скорости (60 FPS).

🖥️ <b>Реальные коммерческие имена видеокарт (DXGI GPU Marketing Names):</b>
• В интерфейсе выбора графического адаптера («Графика -> Устройство вывода») теперь отображаются точные коммерческие названия видеокарт (например, NVIDIA GeForce RTX 3080 Ti) вместо обобщенных системных идентификаторов драйвера (NVIDIA Graphics Device), благодаря интеграции прямого распознавания через DXGI.

🔄 <b>Полная двусторонняя синхронизация настроек подвала:</b>
• Все переключатели, кнопки и контекстные меню статусной строки (подвала) теперь синхронизированы в реальном времени с основными диалогами настроек (глобальными и индивидуальными per-game).
• Изменение разрешения, сглаживания, точности GPU/CPU, API, VSync, видеопамяти и режима док-станции мгновенно применяется на лету, сохраняется в соответствующий конфигурационный файл и корректно обновляет все элементы управления.

🎬 <b>Обновление терминологии аппаратного декодирования NVDEC:</b>
• Параметры декодирования видео в меню и настройках приведены к единому строгому стандарту: Отключено, ЦП, ГПУ, Гибридный.

⚙️ <b>Исправление порядка применения профилей GameFix Database:</b>
• Устранена ошибка порядка вызова SetGlobal(false) при автоматическом применении профилей оптимизации игр, гарантирующая приоритетное применение пользовательских и предустановленных настроек.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы и архивы собраны, проверены и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.2.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.2 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.2_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.2 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.2_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.4.2 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.4.2_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.4.2 (Windows x64 Release Portable)</b>"
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

Write-Host "`nRelease 7.4.2 deployment to Telegram completed successfully!"