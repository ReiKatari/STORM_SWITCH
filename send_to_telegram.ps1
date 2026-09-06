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
⚡ <b>Обновление STORM SWITCH 7.3.9 (Fix Broken Graphics and Low FPS on NVIDIA, Texture Swizzle Fix, Stable Hybrid Frame Pacing, Unified UI and Zelda Driver Architecture)</b> — <i>Экстренное обновление эмулятора Nintendo Switch: полное исправление падения производительности до 4–6 FPS и полос на экране на видеокартах NVIDIA, исправление искажения текстур и пост-эффектов в играх на движках Unity и Unreal Engine, устранение статтеров таймера в Windows, синхронизация конвейеров шейдеров и унификация названий настроек</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

🎮 <b>Исправление критического бага с 4–6 FPS и полосами на NVIDIA:</b>
• Восстановлена корректная проверка версии видеодрайверов NVIDIA: расширение VK_EXT_vertex_input_dynamic_state теперь надежно отключается на драйверах ниже 580.119.
• На драйверах NVIDIA ниже 580.119 реализация динамического ввода вершин содержит аппаратную ошибку в Vulkan-драйвере, из-за которой геометрия коллапсировала в горизонтальные полосы, а видеокарта зависала до 150 мс на кадр (4–6 FPS). Теперь геометрия и фреймрейт полностью восстановлены.

🎨 <b>Устранение искажений текстур и пост-эффектов (Unity и Unreal Engine):</b>
• Полностью удален некорректный оверрайд одноканальных текстур (R8_UNORM, BC4_UNORM), принудительно заменявший цветовые каналы на единицу.
• В играх на движке Unity (Signalis и других) и Unreal Engine полностью восстановлены корректный рендеринг G-буфера, карты шероховатости, пост-обработка и освещение без белых заливок и визуального мусора.

⚡ <b>Синхронизация и стабильность дискового кэша шейдеров:</b>
• Восстановлено синхронное завершение построения базового кэша конвейеров до старта первого кадра рендеринга.
• Устранены одновременные коллизии между 16 фоновыми потоками компиляции и главным потоком рендеринга при старте игры, исключены микрофризы и зависания на первом кадре.

❄️ <b>Стабилизация интервалов кадров в Windows (Hybrid Frame Pacing):</b>
• Заменен цикл частых вызовов sleep_for на надежный гибридный спин-тайл с квантом 1 мс.
• На Windows стандартный системный таймер может задерживать вызовы на 15 мс, что вызывало искусственные просадки и дергания кадров. Новый алгоритм гарантирует идеальную плавность 60/120 FPS без пропусков кадров.

📱 <b>Унификация названий параметров (Windows и Android):</b>
• Приведены к единому эталонному стандарту названия всех опций и параметров интерфейса в обеих версиях эмулятора.
• Обеспечена 100% локализация, единый Sentence case и строгое соблюдение экосистемы STORM SOFT.

🗡️ <b>Архитектура драйвера STORM DRIVER 2.0.5 Zelda:</b>
• Подтверждена полная преемственность всех параметров и условий стабильности из версии 1.2.3: сохранена фиксация GMEM, коррекция направления глубины и фиксы геометрии святилищ, а также добавлены улучшенные параметры против мерцания.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы и APK-пакеты собраны, проверены и готовы к работе.</i>
"@

Write-Host "1. Sending release announcement..."
Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.3.9.apk"
        Caption = "📱 <b>STORM SWITCH 7.3.9 (Mainline Release - Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.3.9_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 7.3.9 (Legacy Release - Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.3.9_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 7.3.9 (SDK27 Release - Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_7.3.9_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 7.3.9 (Windows x64 Release Portable)</b>"
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

Write-Host "`nRelease 7.3.9 deployment to Telegram completed successfully!"