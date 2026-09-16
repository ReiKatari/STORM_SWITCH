Stop-Process -Name 'storm_eden', 'storm_eden-cli', 'storm_eden-room', 'STORM_SWITCH', 'STORM_SWITCH-cli', 'STORM_SWITCH-room' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
$sha1 = '10C44A100C93E316872A1BEF4D46269EA9C52269'

Write-Host "Copying freshly compiled 8.7.0 binaries from build_ninja\bin to Assembling..."
Copy-Item 'E:\STORM SWITCH 4\Build\build_ninja\bin\STORM_SWITCH*.exe' 'E:\STORM SWITCH 4\Assembling\' -Force

Write-Host "Signing Assembling executables..."
$targetExes = @(
    'E:\STORM SWITCH 4\Assembling\STORM_SWITCH.exe',
    'E:\STORM SWITCH 4\Assembling\STORM_SWITCH-cli.exe',
    'E:\STORM SWITCH 4\Assembling\STORM_SWITCH-room.exe'
)

foreach ($exe in $targetExes) {
    if (Test-Path $exe) {
        & $signtool sign /fd SHA256 /sha1 $sha1 $exe
    }
}

Write-Host "Bundling MSVC CRT libraries into Assembling..."
Copy-Item "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT\*.dll" 'E:\STORM SWITCH 4\Assembling\' -Force
Copy-Item "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP\*.dll" 'E:\STORM SWITCH 4\Assembling\' -Force

Get-ChildItem -Path 'E:\STORM SWITCH 4\Assembling' -Recurse | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Creating Windows 11 release zip (compiled files + empty user folder + custom configs)..."
$stageWin11 = 'E:\STORM SWITCH 4\Build\build_ninja\stage_win11'
if (Test-Path $stageWin11) { Remove-Item $stageWin11 -Recurse -Force }
New-Item -ItemType Directory -Path "$stageWin11\user\config\custom" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\keys" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\nand" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\load" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\screenshots" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\sdmc" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\shader" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin11\user\cache" -Force | Out-Null

Copy-Item 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH*.exe' $stageWin11\ -Force
Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.exe' $stageWin11\ -Force -ErrorAction SilentlyContinue
Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.dll' $stageWin11\ -Force -ErrorAction SilentlyContinue
if (Test-Path 'E:\STORM SWITCH 4\Assembling\user\config\custom') {
    Copy-Item 'E:\STORM SWITCH 4\Assembling\user\config\custom\*.ini' "$stageWin11\user\config\custom\" -Force -ErrorAction SilentlyContinue
}

$zipPathWin11 = 'E:\STORM SWITCH 4\Files\STORM_SWITCH_8.7.0_Windows11.zip'
if (Test-Path $zipPathWin11) { Remove-Item $zipPathWin11 -Force }
& 'C:\Program Files\7-Zip\7z.exe' a -tzip $zipPathWin11 "$stageWin11\*" -mx=9
Unblock-File $zipPathWin11
Remove-Item $stageWin11 -Recurse -Force

Write-Host "Creating Windows 10 release zip (compiled files + empty user folder + custom configs + all .dlls)..."
$stageWin10 = 'E:\STORM SWITCH 4\Build\build_ninja\stage_win10'
if (Test-Path $stageWin10) { Remove-Item $stageWin10 -Recurse -Force }
New-Item -ItemType Directory -Path "$stageWin10\user\config\custom" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\keys" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\nand" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\load" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\screenshots" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\sdmc" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\shader" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageWin10\user\cache" -Force | Out-Null

Copy-Item 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH*.exe' $stageWin10\ -Force
Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.exe' $stageWin10\ -Force -ErrorAction SilentlyContinue
Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.dll' $stageWin10\ -Force -ErrorAction SilentlyContinue
Copy-Item 'E:\STORM SWITCH 4\Assembling\*.dll' $stageWin10\ -Force -ErrorAction SilentlyContinue
if (Test-Path 'E:\STORM SWITCH 4\Assembling\user\config\custom') {
    Copy-Item 'E:\STORM SWITCH 4\Assembling\user\config\custom\*.ini' "$stageWin10\user\config\custom\" -Force -ErrorAction SilentlyContinue
}

$zipPathWin10 = 'E:\STORM SWITCH 4\Files\STORM_SWITCH_8.7.0_Windows10.zip'
if (Test-Path $zipPathWin10) { Remove-Item $zipPathWin10 -Force }
& 'C:\Program Files\7-Zip\7z.exe' a -tzip $zipPathWin10 "$stageWin10\*" -mx=9
Unblock-File $zipPathWin10
Remove-Item $stageWin10 -Recurse -Force

Write-Host "All Windows 8.7.0 executables signed and packaged successfully into E:\STORM SWITCH 4\Files!"
