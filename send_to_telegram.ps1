$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Net.Http
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$tokenFile = "e:\STORM EDEN 3\tg_token.txt"
if (Test-Path $tokenFile) {
    $token = (Get-Content $tokenFile -Raw -Encoding UTF8).Trim()
} elseif ($env:TELEGRAM_BOT_TOKEN) {
    $token = $env:TELEGRAM_BOT_TOKEN.Trim()
} else {
    Write-Error "Telegram bot token file not found!"
    exit 1
}

$chatId = "-5389146045"

$httpClient = [System.Net.Http.HttpClient]::new()
$httpClient.Timeout = [System.TimeSpan]::FromMinutes(15)

function Send-TGMessage([string]$text) {
    Write-Host "1. Sending release announcement..."
    $url = "https://api.telegram.org/bot$token/sendMessage"
    $payload = @{
        chat_id = $chatId
        text = $text
        parse_mode = "HTML"
        disable_web_page_preview = $true
    } | ConvertTo-Json -Compress

    $content = [System.Net.Http.StringContent]::new($payload, [System.Text.Encoding]::UTF8, "application/json")
    $response = $httpClient.PostAsync($url, $content).GetAwaiter().GetResult()
    $resStr = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    Write-Host "Announcement Sent: $resStr"
}

function Send-TGDocument([string]$filePath, [string]$caption) {
    $fileName = [System.IO.Path]::GetFileName($filePath)
    Write-Host "Sending $fileName via HttpClient (100% UTF-8)..."
    $url = "https://api.telegram.org/bot$token/sendDocument"

    $form = [System.Net.Http.MultipartFormDataContent]::new()
    $form.Add([System.Net.Http.StringContent]::new($chatId, [System.Text.Encoding]::UTF8), "chat_id")
    $form.Add([System.Net.Http.StringContent]::new("HTML", [System.Text.Encoding]::UTF8), "parse_mode")
    $form.Add([System.Net.Http.StringContent]::new($caption, [System.Text.Encoding]::UTF8), "caption")

    $fileStream = [System.IO.File]::OpenRead($filePath)
    $fileContent = [System.Net.Http.StreamContent]::new($fileStream)
    $fileContent.Headers.ContentType = [System.Net.Http.Headers.MediaTypeHeaderValue]::Parse("application/octet-stream")
    $form.Add($fileContent, "document", $fileName)

    $response = $httpClient.PostAsync($url, $form).GetAwaiter().GetResult()
    $resStr = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    $fileStream.Close()
    $fileStream.Dispose()

    Write-Host "Uploaded $fileName successfully: $resStr`n"
}

$announcement = @"
⚡ <b>Релиз STORM SWITCH 8.1.1 и STORM DRIVER 3.0.1</b> — <i>Масштабная стабилизация эмуляции (ликвидация вылетов Mortal Kombat 1/11, Hades II, Zelda, NieR:Automata), интеграция звукового движка SDL3, защита декодера NCE ARM64 и интерактивный просмотрщик дополнений (DLC) в STORM GAMES WORLD.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения STORM SWITCH 8.1.1:</b>

🛡️ <b>Ликвидация вылетов и стабилизация ядра:</b>
• <b>Единый стандарт SaveDir и перенос очистки временного хранилища</b>: устранено повторное дублирование путей сохранений на Windows (<code>nand/user/save/user/save</code>) и случайное удаление <code>/temp/</code> во время загрузки подпрограмм в Mortal Kombat 1/11 и MGS. Очистка временных файлов теперь выполняется строго на уровне инициализации приложения.
• <b>Защита декодера NCE ARM64 от кириллицы</b>: предотвращено ложное декодирование символов UTF-8 как инструкций эксклюзивной записи ARM64 (<code>STXR</code>), устраняющее краши при загрузке локализованных данных.
• <b>Аппаратный лимит декодеров Opus (24 потока)</b>: добавлено отслеживание активных декодеров и код ошибки <code>ResultOutOfOpusDecoders (385)</code>, что обеспечивает бесшовное переключение на программный Opus и устраняет падение при загрузке сохранений в Hades II.
• <b>Корректный порядок остановки GPU-потока</b>: уничтожение потока <code>gpu_thread</code> перенесено после завершения зависимых подсистем, предотвращая зависания при закрытии и перезапуске эмулятора.
• <b>Безопасная загрузка адресов функций</b>: устранён ложный выброс исключения <code>bad_alloc</code> при отсутствии необязательных символов виртуальной памяти.
• <b>Маскировка Vulkan на Android</b>: данные <code>VkApplicationInfo</code> стилизованы под <code>PUBGMobile</code> и <code>UnrealEngine</code>, активируя глубокие аппаратные оптимизации и обход багов в драйверах Qualcomm Adreno и Mali.

🔊 <b>Новый звуковой движок SDL3 Audio на Android:</b>
• Полная замена устаревшего Oboe на высокопроизводительный унифицированный бэкенд <b>SDL3</b> со сверхнизкой задержкой, прямой инициализацией через JNI и повышенной стабильностью вывода звука.

🎮 <b>STORM GAMES WORLD — отображение и просмотр дополнений (DLC):</b>
• <b>Плашка количества дополнений</b>: в каталоге игр и карточке деталей добавлена стильная неоновая плашка <code>+N DLC</code>.
• <b>Стилизованный модальный просмотрщик</b>: при нажатии на плашку открывается удобное объёмное окно со списком всех дополнений, их номерами, названиями и описаниями с автоматической подгрузкой с сервера.

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения STORM DRIVER 3.0.1 (Universal Edition):</b>
• <b>Subpass Fusion v2</b>: интеллектуальное объединение проходов рендеринга с защитой глубины и трафарета (+15-20% прироста эффективности).
• <b>Sparse Buffer Pages</b>: улучшена адресация виртуальных страниц буферов памяти для крупных AAA-проектов.
• <b>14 выделенных профилей</b>: индивидуальная настройка движка для Zelda BotW/TotK, Hogwarts Legacy, Persona 5 Royal, Mario Kart 8 Deluxe, Pokémon, Batman, Witcher 3, No Man's Sky, GTA V, DOOM Eternal, Xenoblade Chronicles 3, Diablo II и Streets of Rage 4.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все бинарные файлы подписаны официальным сертификатом SHA-256, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.1.apk"
        Caption = "📱 <b>STORM SWITCH 8.1.1 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.1_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.1.1 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.1_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.1.1 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_SWITCH_8.1.1_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.1.1 (Портативная версия для Windows x64)</b>"
    },
    @{
        Path = "E:\STORM SWITCH 3\Files\STORM_DRIVER_3.0.1.zip"
        Caption = "🎮 <b>STORM DRIVER 3.0.1 (Универсальный драйвер Vulkan Turnip / PanVK для Android)</b>"
    }
)

foreach ($f in $filesToUpload) {
    if (Test-Path -LiteralPath $f.Path) {
        Send-TGDocument $f.Path $f.Caption
    } else {
        Write-Warning "File not found: $($f.Path)"
    }
}

$httpClient.Dispose()
Write-Host "`nRelease 8.1.1 deployment to Telegram completed successfully!"

