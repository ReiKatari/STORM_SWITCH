Stop-Process -Name 'storm_eden', 'storm_eden-cli', 'storm_eden-room', 'STORM_SWITCH', 'STORM_SWITCH-cli', 'STORM_SWITCH-room' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
$sha1 = '10C44A100C93E316872A1BEF4D46269EA9C52269'



Write-Host "Bundling MSVC CRT libraries into Assembling..."
Copy-Item "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT\*.dll" 'E:\STORM SWITCH 4\Assembling\' -Force
Copy-Item "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP\*.dll" 'E:\STORM SWITCH 4\Assembling\' -Force

Get-ChildItem -Path 'E:\STORM SWITCH 4\Assembling' -Recurse | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Copying freshly compiled 9.5.0 binaries from build_ninja\bin to Assembling..."
Copy-Item 'E:\STORM SWITCH 4\Build\build_ninja\bin\STORM_SWITCH*.exe' 'E:\STORM SWITCH 4\Assembling\' -Force
Remove-Item 'E:\STORM SWITCH 4\Assembling\*.pdb' -Force -ErrorAction SilentlyContinue

Write-Host "Signing Assembling executables..."
& $signtool sign /sha1 $sha1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH.exe'
if (Test-Path 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH-cli.exe') {
    & $signtool sign /sha1 $sha1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH-cli.exe'
}
if (Test-Path 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH-room.exe') {
    & $signtool sign /sha1 $sha1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH-room.exe'
}

function Prepare-Staging($targetDir, $includeCrt) {
    if (Test-Path $targetDir) { Remove-Item $targetDir -Recurse -Force }
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null

    # 1. Copy Executables and 7z
    Copy-Item 'E:\STORM SWITCH 4\Assembling\STORM_SWITCH*.exe' $targetDir -Force
    Copy-Item 'E:\STORM SWITCH 4\Assembling\7z.*' $targetDir -Force

    # 2. If Windows 10, bundle VC CRT dlls
    if ($includeCrt) {
        Copy-Item 'E:\STORM SWITCH 4\Assembling\*.dll' $targetDir -Force
    }

    # 3. Create pristine user directory tree
    $subdirs = @('cache', 'config', 'config\custom', 'keys', 'load', 'nand', 'screenshots', 'sdmc', 'shader')
    foreach ($sd in $subdirs) {
        New-Item -ItemType Directory -Path "$targetDir\user\$sd" -Force | Out-Null
    }

    # 4. Copy custom configurations if present
    if (Test-Path 'E:\STORM SWITCH 4\Assembling\user\config\custom') {
        Copy-Item 'E:\STORM SWITCH 4\Assembling\user\config\custom\*.ini' "$targetDir\user\config\custom\" -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "Preparing Windows 11 staging..."
$stageWin11 = 'E:\STORM SWITCH 4\Temp_Package_Win11'
Prepare-Staging $stageWin11 $false

$zipPathWin11 = 'E:\STORM SWITCH 4\Files\STORM_SWITCH_9.5.0_Windows11.zip'
if (Test-Path $zipPathWin11) { Remove-Item $zipPathWin11 -Force }
& 'C:\Program Files\7-Zip\7z.exe' a -tzip $zipPathWin11 "$stageWin11\*" -mx=9
Unblock-File $zipPathWin11
Remove-Item $stageWin11 -Recurse -Force

Write-Host "Preparing Windows 10 staging (with VC CRT)..."
$stageWin10 = 'E:\STORM SWITCH 4\Temp_Package_Win10'
Prepare-Staging $stageWin10 $true

$zipPathWin10 = 'E:\STORM SWITCH 4\Files\STORM_SWITCH_9.5.0_Windows10.zip'
if (Test-Path $zipPathWin10) { Remove-Item $zipPathWin10 -Force }
& 'C:\Program Files\7-Zip\7z.exe' a -tzip $zipPathWin10 "$stageWin10\*" -mx=9
Unblock-File $zipPathWin10
Remove-Item $stageWin10 -Recurse -Force

Write-Host "All Windows 9.5.0 executables signed and packaged successfully into E:\STORM SWITCH 4\Files!"
