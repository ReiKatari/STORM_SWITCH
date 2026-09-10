Stop-Process -Name 'storm_eden', 'storm_eden-cli', 'storm_eden-room', 'STORM_SWITCH', 'STORM_SWITCH-cli', 'STORM_SWITCH-room' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
$sha1 = '10C44A100C93E316872A1BEF4D46269EA9C52269'

Write-Host "Signing Assembling executables..."
$targetExes = @(
    'e:\STORM EDEN 3\Assembling\STORM_SWITCH.exe',
    'e:\STORM EDEN 3\Assembling\STORM_SWITCH-cli.exe',
    'e:\STORM EDEN 3\Assembling\STORM_SWITCH-room.exe'
)

# Remove any old storm_eden binaries
Remove-Item 'e:\STORM EDEN 3\Assembling\storm_eden*' -Force -ErrorAction SilentlyContinue
Remove-Item 'e:\STORM EDEN 3\storm_eden*' -Force -ErrorAction SilentlyContinue

foreach ($exe in $targetExes) {
    if (Test-Path $exe) {
        & $signtool sign /fd SHA256 /sha1 $sha1 $exe
        $leaf = Split-Path $exe -Leaf
        Copy-Item $exe (Join-Path 'e:\STORM EDEN 3' $leaf) -Force
        Copy-Item $exe (Join-Path 'e:\STORM SWITCH 3\Assembling' $leaf) -Force
        Copy-Item $exe (Join-Path 'e:\STORM SWITCH 3' $leaf) -Force
    }
}

Get-ChildItem -Path 'e:\STORM EDEN 3\Assembling' -Recurse | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path 'e:\STORM EDEN 3\Files' -Recurse | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path 'e:\STORM EDEN 3' -File | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path 'e:\STORM SWITCH 3\Assembling' -Recurse | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path 'e:\STORM SWITCH 3\Files' -Recurse | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path 'e:\STORM SWITCH 3' -File | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Creating Windows 8.0.4 release zip..."
$stageDir = 'e:\STORM EDEN 3\build\stage_zip'
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Path "$stageDir\user\config", "$stageDir\user\load", "$stageDir\user\nand", "$stageDir\user\sdmc", "$stageDir\user\cache" -Force | Out-Null
Copy-Item 'e:\STORM EDEN 3\Assembling\STORM_SWITCH*.exe' $stageDir\ -Force
Copy-Item 'e:\STORM EDEN 3\Assembling\7z.exe' $stageDir\ -Force -ErrorAction SilentlyContinue
Copy-Item 'e:\STORM EDEN 3\Assembling\7z.dll' $stageDir\ -Force -ErrorAction SilentlyContinue

$zipPath = 'e:\STORM EDEN 3\Files\STORM_SWITCH_8.0.4_Windows.zip'
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

& 'C:\Program Files\7-Zip\7z.exe' a -tzip $zipPath "$stageDir\*" -mx=9

Copy-Item $zipPath 'e:\STORM EDEN 3\STORM_SWITCH_8.0.4_Windows.zip' -Force
Copy-Item $zipPath 'e:\STORM SWITCH 3\Files\STORM_SWITCH_8.0.4_Windows.zip' -Force
Copy-Item $zipPath 'e:\STORM SWITCH 3\STORM_SWITCH_8.0.4_Windows.zip' -Force

Remove-Item $stageDir -Recurse -Force

Write-Host "All executables signed, packaged to 8.0.4 zip and copied to root/Files!"
