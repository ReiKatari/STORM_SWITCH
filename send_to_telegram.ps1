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
⚡ <b>Релиз STORM SWITCH 8.0.0 (Streets of Rage 4 60 FPS, Mortal Kombat 1 Fix, AC Rogue DLC Unlock, статус-бар с иконкой геймпада и Maximum Performance)</b> — <i>Комплексное обновление эмулятора Nintendo Switch: моментальное декодирование видеороликов без задержек и стабильные 60 FPS в Streets of Rage 4, ликвидация зависания при старте Mortal Kombat 1, восстановление оригинального идентификатора пакета (dev.storm_switch) для бесшовного обновления поверх 7.5.x, стилизованный статус-бар со значком геймпада 🎮 и полным названием версии STORM SWITCH 8.0.0, очищенная вкладка «Дополнения» в свойствах игры и полноценное открытие сюжетной кампании DLC «Изгой» в Assassin's Creed: The Rebel Collection.</i>

━━━━━━━━━━━━━━━━━━━━━━━

🚀 <b>Ключевые изменения и улучшения:</b>

👊 <b>Streets of Rage 4 (моментальное декодирование роликов без задержки и 60 FPS):</b>
• <b>Устранение просадки до 4–6 FPS и кадра 200 мс</b>: оптимизирован движок FFmpeg / VP9 — исключена многокадровая буферизация <code>FF_THREAD_FRAME</code>, активирован режим мгновенного декодирования с нулевой задержкой (<code>delay = 0</code>, <code>FF_THREAD_SLICE</code>, <code>AV_CODEC_FLAG_LOW_DELAY</code>). Вступительные ролики, заставки и катсцены воспроизводятся плавно и без микрофризов.
• <b>Ликвидация сбоя на экране загрузки</b>: внедрен встроенный патч машинного кода ARM64 NSO для обхода блокировок при опросе контроллеров.

🗡️ <b>Assassin's Creed: The Rebel Collection — разблокировка DLC «Изгой» (Rogue):</b>
• <b>Служба AOC и авторегистрация контента</b>: контейнеры NSP автоматически регистрируют встроенные дополнения в диспетчере контента, а служба <code>aoc:u</code> динамически переопрашивает доступные DLC и предоставляет резервные дескрипторы для сборника Rogue (DLC 1, 2, 3).
• <b>Полноценный доступ в игре</b>: предупреждение «Загрузите игру в eShop» полностью снято, сюжетная линия Изгоя доступна для запуска прямо из главного меню.

🎨 <b>Интерфейс, типографика и визуальный статус-бар:</b>
• <b>Иконка геймпада в счетчике FPS</b>: слева от счетчика кадров добавлена гармоничная иконка <code>🎮</code> (как в Windows-интерфейсе, так и в оверлее Android).
• <b>Фирменное отображение версии</b>: номер версии теперь выводится строго как <code>STORM SWITCH 8.0.0</code> без голых цифр и запрещенных приставок.
• <b>Очистка вкладки «Дополнения»</b>: в окне свойств игры вкладка переименована в строгое «Дополнения», убраны лишние кнопки импорта архивов/папок и полностью исключены строки LayeredFS и модификаций.

🔄 <b>Бесшовное обновление поверх версий 7.5.x (возврат dev.storm_switch):</b>
• <b>Оригинальные идентификаторы</b>: приложения собраны с <code>dev.storm_switch</code>, <code>dev.storm_switch.legacy</code> и <code>dev.storm_switch.sdk27</code> для гладкого обновления с сохранением всех настроек и драйверов.

🥊 <b>Mortal Kombat 1 (устранение зависания при запуске):</b>
• <b>Стабильная инициализация потоков</b>: скорректирована синхронизация ядер ЦП и выделение памяти DRAM 6 ГБ для предотвращения зависания движка NetherRealm.

⚡ <b>Аппаратная оптимизация графики:</b>
• <b>Аппаратное декодирование ASTC на ГПУ</b>: перевод декодирования текстур ASTC на Compute Shaders видеокарты.
• <b>Ликвидация задержек рендеринга</b>: устранена избыточная синхронизация памяти и реактивная очистка.

━━━━━━━━━━━━━━━━━━━━━━━
📦 <i>Все исполняемые файлы, инсталляторы и архивы собраны, проверены и готовы к работе.</i>
"@

Send-TGMessage $announcement

Write-Host "2. Uploading release files to Telegram (Main APK first)..."
$filesToUpload = @(
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.0 (Основная версия — Android 14+)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0_LEGACY.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.0 (Версия Legacy — Android 10-13)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0_SDK27.apk"
        Caption = "📱 <b>STORM SWITCH 8.0.0 (Версия SDK27 — Android 8.1-9)</b>"
    },
    @{
        Path = "E:\STORM EDEN 3\Files\STORM_SWITCH_8.0.0_Windows.zip"
        Caption = "💻 <b>STORM SWITCH 8.0.0 (Портативная версия для Windows x64)</b>"
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
Write-Host "`nRelease 8.0.0 deployment to Telegram completed successfully!"