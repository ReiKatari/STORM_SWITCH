$mainline = 'E:\STORM SWITCH 4\Build\src\android\app\build\outputs\apk\mainline\release\app-mainline-release.apk'
$legacy = 'E:\STORM SWITCH 4\Build\src\android\app\build\outputs\apk\legacy\release\app-legacy-release.apk'
$sdk27 = 'E:\STORM SWITCH 4\Build\src\android\app\build\outputs\apk\sdk27\release\app-sdk27-release.apk'

Write-Host "Copying APKs with STORM_SWITCH_9.0.0 naming to E:\STORM SWITCH 4\Files..."
if (Test-Path $mainline) {
    Copy-Item $mainline 'E:\STORM SWITCH 4\Files\STORM_SWITCH_9.0.0.apk' -Force
}

if (Test-Path $legacy) {
    Copy-Item $legacy 'E:\STORM SWITCH 4\Files\STORM_SWITCH_9.0.0_LEGACY.apk' -Force
}

if (Test-Path $sdk27) {
    Copy-Item $sdk27 'E:\STORM SWITCH 4\Files\STORM_SWITCH_9.0.0_SDK27.apk' -Force
}

Get-ChildItem -Path 'E:\STORM SWITCH 4\Files' -Recurse | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path 'E:\STORM SWITCH 4\Assembling' -Recurse | Unblock-File -ErrorAction SilentlyContinue

Write-Host "All 9.0.0 APKs copied to E:\STORM SWITCH 4\Files and unblocked successfully!"
