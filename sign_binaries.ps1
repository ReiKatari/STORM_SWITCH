Stop-Process -Name 'storm_eden', 'storm_eden-cli', 'storm_eden-room', 'STORM_SWITCH', 'STORM_SWITCH-cli', 'STORM_SWITCH-room' -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
$baseDir = Split-Path -Parent $PSScriptRoot
$assemblingDir = Join-Path $baseDir "Assembling"
$filesDir = Join-Path $baseDir "Files"

$sha1 = $env:STORM_CERT_THUMBPRINT
if (-not $sha1 -and (Test-Path 'F:\STORM ALL PROJECTS\.storm-secrets\storm_signing.properties')) {
    $propsLine = Get-Content 'F:\STORM ALL PROJECTS\.storm-secrets\storm_signing.properties' | Where-Object { $_ -match '^STORM_CERT_THUMBPRINT=' }
    if ($propsLine) { $sha1 = ($propsLine -split '=', 2)[1].Trim() }
}
if (-not $sha1) {
    $cert = Get-ChildItem Cert:\CurrentUser\My -ErrorAction SilentlyContinue | Where-Object { $_.Subject -match 'STORM SERTIFICATE' } | Select-Object -First 1
    if ($cert) { $sha1 = $cert.Thumbprint }
}

$version = '12.1.0'
if (Test-Path "$PSScriptRoot\GIT-RELEASE") {
    $v = (Get-Content "$PSScriptRoot\GIT-RELEASE" -Raw).Trim()
    if ($v) { $version = $v }
}

Write-Host "Bundling MSVC CRT libraries into Assembling..."
Copy-Item "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT\*.dll" "$assemblingDir\" -Force
Copy-Item "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP\*.dll" "$assemblingDir\" -Force

Get-ChildItem -Path $assemblingDir -Recurse | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Copying freshly compiled $version binaries from build_ninja\bin to Assembling..."
Copy-Item "$PSScriptRoot\build_ninja\bin\STORM_SWITCH*.exe" "$assemblingDir\" -Force
Remove-Item "$assemblingDir\*.pdb" -Force -ErrorAction SilentlyContinue

Write-Host "Signing Assembling executables with unified STORM SERTIFICATE..."
& $signtool sign /sha1 $sha1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 "$assemblingDir\STORM_SWITCH.exe"
if (Test-Path "$assemblingDir\STORM_SWITCH-cli.exe") {
    & $signtool sign /sha1 $sha1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 "$assemblingDir\STORM_SWITCH-cli.exe"
}
if (Test-Path "$assemblingDir\STORM_SWITCH-room.exe") {
    & $signtool sign /sha1 $sha1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 "$assemblingDir\STORM_SWITCH-room.exe"
}

$sevenZip = "$assemblingDir\7z.exe"
if (-not (Test-Path $sevenZip)) {
    $sevenZip = 'C:\Program Files\7-Zip\7z.exe'
}

function Prepare-Staging($targetDir, $includeCrt) {
    if (Test-Path $targetDir) { Remove-Item $targetDir -Recurse -Force }
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null

    # 1. Copy Executables and 7z
    Copy-Item "$assemblingDir\STORM_SWITCH*.exe" $targetDir -Force
    if (Test-Path "$assemblingDir\7z.*") {
        Copy-Item "$assemblingDir\7z.*" $targetDir -Force
    }

    # 2. If Windows 10, bundle VC CRT dlls
    if ($includeCrt) {
        Copy-Item "$assemblingDir\*.dll" $targetDir -Force
    }

    # 3. Create empty user directory
    New-Item -ItemType Directory -Path "$targetDir\user" -Force | Out-Null
}

if (Test-Path "M:\CONSOLES\Nintendo Switch\STORM SWITCH") {
    Write-Host "Syncing signed executables to M:\CONSOLES\Nintendo Switch\STORM SWITCH..."
    Copy-Item "$assemblingDir\STORM_SWITCH*.exe" "M:\CONSOLES\Nintendo Switch\STORM SWITCH\" -Force
}

Write-Host "Preparing Windows 11 staging..."
$stageWin11 = Join-Path $baseDir "Temp_Package_Win11"
Prepare-Staging $stageWin11 $false

$zipPathWin11 = Join-Path $filesDir "STORM_SWITCH_${version}_Windows11.zip"
if (Test-Path $zipPathWin11) { Remove-Item $zipPathWin11 -Force }
Push-Location $stageWin11
if (Test-Path $sevenZip) {
    & $sevenZip a -tzip $zipPathWin11 * -mx=9
} else {
    & tar.exe -a -c -f $zipPathWin11 *
}
Pop-Location
Unblock-File $zipPathWin11 -ErrorAction SilentlyContinue
Remove-Item $stageWin11 -Recurse -Force

Write-Host "Preparing Windows 10 staging (with VC CRT)..."
$stageWin10 = Join-Path $baseDir "Temp_Package_Win10"
Prepare-Staging $stageWin10 $true

$zipPathWin10 = Join-Path $filesDir "STORM_SWITCH_${version}_Windows10.zip"
if (Test-Path $zipPathWin10) { Remove-Item $zipPathWin10 -Force }
Push-Location $stageWin10
if (Test-Path $sevenZip) {
    & $sevenZip a -tzip $zipPathWin10 * -mx=9
} else {
    & tar.exe -a -c -f $zipPathWin10 *
}
Pop-Location
Unblock-File $zipPathWin10 -ErrorAction SilentlyContinue
Remove-Item $stageWin10 -Recurse -Force

Write-Host "All Windows $version executables signed and packaged successfully into $filesDir!"
