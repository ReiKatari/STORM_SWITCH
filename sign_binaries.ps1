Stop-Process -Name 'storm_eden', 'storm_eden-cli', 'storm_eden-room', 'STORM_SWITCH', 'STORM_SWITCH-cli', 'STORM_SWITCH-room' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
$sha1 = '10C44A100C93E316872A1BEF4D46269EA9C52269'

Write-Host "Copying freshly compiled 8.2.0 binaries from build_ninja\bin to Assembling..."
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

Write-Host "Creating Windows 8.2.0 release zip..."
$stageDir = 'E:\STORM SWITCH 4\Build\build_ninja\stage_zip'
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Path "$stageDir\user\config", "$stageDir\user\load", "$stageDir\user\nand", "$stageDir\user\sdmc", "$stageDir\user\cache" -Force | Out-Null

Copy-Item 'E:\STORM SWITCH 4\Assembling\user\config\qt-config.ini' "$stageDir\user\config\" -Force -ErrorAction SilentlyContinue
if (Test-Path 'E:\STORM SWITCH 4\Assembling\user\config\custom') {
    Copy-Item 'E:\STORM SWITCH 4\Assembling\user\config\custom' "$stageDir\user\config\" -Recurse -Force
}
Copy-Item 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH*.exe' $stageDir\ -Force
Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.exe' $stageDir\ -Force -ErrorAction SilentlyContinue
Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.dll' $stageDir\ -Force -ErrorAction SilentlyContinue
Copy-Item 'E:\STORM SWITCH 4\Assembling\*.dll' $stageDir\ -Force -ErrorAction SilentlyContinue

$zipPath = 'E:\STORM SWITCH 4\Files\STORM_SWITCH_8.2.0_Windows.zip'
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

& 'C:\Program Files\7-Zip\7z.exe' a -tzip $zipPath "$stageDir\*" -mx=9

Unblock-File $zipPath
Remove-Item $stageDir -Recurse -Force

Write-Host "All executables signed, packaged to 8.2.0 Windows zip in E:\STORM SWITCH 4\Files!"
