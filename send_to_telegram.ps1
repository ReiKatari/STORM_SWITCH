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
⚡ <b>Релиз STORM SWITCH 7.5.1 (Streets of Rage 4 Fix, MK11 Stage Rendering, Zelda Magic Stability, Cooling Pause and Gesture Enhancements)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: полное устранение зависаний и вылетов в Streets of Rage 4, исправление рендеринга арен и задников в Mortal Kombat 11, устранение мерцания магии и рун в The Legend of Zelda, улучшенный экран паузы и охлаждения без искажения пропорций кадра и надёжный жест вызова быстрых настроек.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🥊 <b>Streets of Rage 4 (Windows и Android):</b>
• Полностью устранены зависания на заставке на Windows и вылеты на Android. Удален поврежденный машинный патч NSO, препятствовавший корректной передаче управления игровому движку после видеоролика.

🥋 <b>Mortal Kombat 11 (Android Turnip):</b>
• Исправлен сбой рендеринга задников и арены («зеленая стена»): отключен деструктивный параметр tu_tile_discard в per-game drirc, добавлены барьеры синхронизации очередей A8xx и сохранение LRZ между командными буферами, применены параметры графики Normal GPU accuracy.

🗡️ <b>The Legend of Zelda: Breath of the Wild и Tears of the Kingdom:</b>
• Устранено мерцание и стробирование при активации рун и магии (Ультрарука, Автосборка, Магнезис, Стазис) благодаря реактивной очистке буферов глубины, с сохранением идеальной прозрачности воды.

❄️ <b>Экран паузы и охлаждение:</b>
• Кадр паузы теперь сохраняет оригинальные пропорции экрана (FIT_XY при растяжении) без искажений и черных полос по бокам.
• При постановке на паузу нагрузка на чипсет и частоты процессора/видеоядра переводятся в глубокий режим покоя (GameManager MODE_NONE) для максимально быстрого остывания устройства.
• В заголовке паузы скорректирована пунктуация: <code>❄️ Игра на паузе - идёт охлаждение устройства</code>.

⚙️ <b>Быстрые настройки и навигация:</b>
• Жест вызова быстрых настроек (свайп 3 пальцами вверх) переработан для безотказного срабатывания в ландшафтном режиме на любых смартфонах.
• В левое боковое меню и шапку добавлены наглядные подсказки о быстром вызове настроек жестом.
• Терминология обновлена: «Авто-коррекция» с дефисом.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.1.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.1 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.1_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.1 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.1_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.1 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.1_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.1 (Windows x64 Release Portable)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

Write-Host "`nRelease 7.5.1 deployment to Telegram completed successfully!"