$baseDir = Split-Path -Parent $PSScriptRoot
$version = '15.1.0'
if (Test-Path "$PSScriptRoot\GIT-RELEASE") {
    $v = (Get-Content "$PSScriptRoot\GIT-RELEASE" -Raw).Trim()
    if ($v) { $version = $v }
}

$filesDir = Join-Path $baseDir "Files"
$assemblingDir = Join-Path $baseDir "Assembling"
if (-not (Test-Path $filesDir)) { New-Item -ItemType Directory -Path $filesDir -Force }

$mainline = "$PSScriptRoot\src\android\app\build\outputs\apk\mainline\release\app-mainline-release.apk"
$legacy = "$PSScriptRoot\src\android\app\build\outputs\apk\legacy\release\app-legacy-release.apk"
$sdk27 = "$PSScriptRoot\src\android\app\build\outputs\apk\sdk27\release\app-sdk27-release.apk"

Write-Host "Copying APKs with canonical STORM_SWITCH_$version naming to $filesDir and $assemblingDir..."
if (-not (Test-Path $assemblingDir)) { New-Item -ItemType Directory -Path $assemblingDir -Force }

if (Test-Path $mainline) {
    Copy-Item $mainline "$filesDir\STORM_SWITCH_$version.apk" -Force
    Copy-Item $mainline "$assemblingDir\STORM_SWITCH_$version.apk" -Force
}

if (Test-Path $legacy) {
    Copy-Item $legacy "$filesDir\STORM_SWITCH_${version}_LEGACY.apk" -Force
    Copy-Item $legacy "$assemblingDir\STORM_SWITCH_${version}_LEGACY.apk" -Force
}

if (Test-Path $sdk27) {
    Copy-Item $sdk27 "$filesDir\STORM_SWITCH_${version}_SDK27.apk" -Force
    Copy-Item $sdk27 "$assemblingDir\STORM_SWITCH_${version}_SDK27.apk" -Force
}

# Clean any duplicate or legacy named APKs
Get-ChildItem -Path $filesDir -Filter "*_Android*.apk" | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir -Filter "*_Android*.apk" | Remove-Item -Force -ErrorAction SilentlyContinue

Get-ChildItem -Path $filesDir | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Canonical $version APKs copied to $filesDir and $assemblingDir successfully (no duplicates)!"
