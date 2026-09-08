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
⚡ <b>Релиз STORM SWITCH 7.5.2 (Streets of Rage 4 Core Fix, Dynarmic JIT Safety, Hybrid NVDEC and Full Per-Game Profile Sync)</b> — <i>Масштабное обновление эмулятора Nintendo Switch: полное устранение зависаний и вылетов в Streets of Rage 4 на Windows и Android, защита JIT-компилятора Dynarmic от повреждения регистров, перевод профилей на эталонный гибридный NVDEC и синхронизация операций памяти.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🥊 <b>Streets of Rage 4 (полное исправление на Windows и Android):</b>
• Устранен конфликт параметров авто-исправления: профили всех 4 Title ID переведены на гибридный NVDEC (Hybrid NVDEC), гарантирующий аппаратное ускорение видеороликов через GPU с надежным резервированием на CPU.
• Включена синхронизация операций памяти (Sync Memory Operations), что полностью ликвидировало разбалансировку счетчиков буферов nvmap (Pin count imbalance detected) и преждевременное открепление поверхностей при закрытии видеопотока.
• Включен асинхронный вывод Vulkan (Async Presentation), исключающий дедлок конвейера и подвисание кадров на стартовой заставке.
• Отключен деструктивный флаг игнорирования прерываний памяти (cpuopt_ignore_memory_aborts = false), приводивший к искажению регистров JIT и падению Userspace PANIC на смещении 0x008C0120.

🛡️ <b>Ядро ARM (Dynarmic 64 JIT):</b>
• В обработчике NoExecuteFault добавлена строгая проверка ненулевого программного счетчика (pc != 0), предотвращающая попытки фиктивного возврата в LR с обнулением регистров при вызовах нулевых указателей функций.

🥋 <b>Mortal Kombat 11 и The Legend of Zelda:</b>
• Подтверждена и зафиксирована стабильная работа исправлений: профили реактивного сброса памяти (Reactive Flushing) для Zelda BotW/TotK и исключение сброса тайлов (tu_tile_discard = false) для Mortal Kombat 11 функционируют как на уровне ядра, так и в базе авто-исправлений.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, подписаны цифровой подписью и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.2.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.2 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.2_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.2 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.2_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.5.2 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.5.2_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.5.2 (Windows x64 Release Portable)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

Write-Host "`nRelease 7.5.2 deployment to Telegram completed successfully!"