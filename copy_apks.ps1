$ErrorActionPreference = "Stop"
$baseDir = Split-Path -Parent $PSScriptRoot
$version = '16.0.2'
if (Test-Path "$PSScriptRoot\GIT-RELEASE") {
    $v = (Get-Content "$PSScriptRoot\GIT-RELEASE" -Raw).Trim()
    if ($v) { $version = $v }
}

$filesDir = Join-Path $baseDir "Files"
$assemblingDir = Join-Path $baseDir "Assembling"
if (-not (Test-Path $filesDir)) { New-Item -ItemType Directory -Path $filesDir -Force | Out-Null }
if (-not (Test-Path $assemblingDir)) { New-Item -ItemType Directory -Path $assemblingDir -Force | Out-Null }

$mainline = "$PSScriptRoot\src\android\app\build\outputs\apk\mainline\release\app-mainline-release.apk"
$legacy   = "$PSScriptRoot\src\android\app\build\outputs\apk\legacy\release\app-legacy-release.apk"
$sdk27    = "$PSScriptRoot\src\android\app\build\outputs\apk\sdk27\release\app-sdk27-release.apk"

# Sign APKs with APK Signature Scheme v3 Key Rotation (legacy v1 keystore -> STORM SERTIFICATE)
# so in-place Android upgrades work seamlessly over BOTH pre-16.0.0 installs and STORM SERTIFICATE installs!
$secretsDir = "F:\STORM ALL PROJECTS\.storm-secrets"
$propsFile  = Join-Path $secretsDir "storm_signing.properties"
$oldKs      = Join-Path $secretsDir "storm_switch_legacy_v1.keystore"
$lineage    = Join-Path $secretsDir "storm_switch_rotation_lineage.bin"

$env:JAVA_HOME = "C:\Users\ReiKa\.jdks\jdk-21.0.12.1+1"
$apksignerItem = Get-ChildItem "C:\Users\ReiKa\AppData\Local\Android\Sdk\build-tools\*\apksigner.bat" | Sort-Object FullName -Descending | Select-Object -First 1

function Sign-ApkWithRotation([string]$apkPath) {
    if (-not (Test-Path $apkPath)) { return }
    if ((Test-Path $propsFile) -and (Test-Path $oldKs) -and (Test-Path $lineage) -and ($null -ne $apksignerItem)) {
        $props = @{}
        foreach ($line in (Get-Content $propsFile -Encoding UTF8)) {
            if ($line -match '^\s*([^#=\s]+)\s*=\s*(.+)\s*$') {
                $props[$matches[1]] = $matches[2].Trim()
            }
        }
        $certFile = if ($props.ContainsKey("STORM_CERT_FILE")) { $props["STORM_CERT_FILE"] } else { $props["storeFile"] }
        if (-not [System.IO.Path]::IsPathRooted($certFile)) {
            $certFile = Join-Path $secretsDir $certFile
        }
        $certPass = if ($props.ContainsKey("STORM_CERT_PASSWORD")) { $props["STORM_CERT_PASSWORD"] } else { $props["storePassword"] }
        $keyAlias = if ($props.ContainsKey("STORM_KEY_ALIAS")) { $props["STORM_KEY_ALIAS"] } else { $props["keyAlias"] }

        Write-Host "Applying STORM SERTIFICATE + V3 Key Rotation Lineage to $(Split-Path $apkPath -Leaf)..."
        & $apksignerItem.FullName sign `
            --v1-signing-enabled true `
            --v2-signing-enabled true `
            --v3-signing-enabled true `
            --v4-signing-enabled false `
            --rotation-min-sdk-version 28 `
            --lineage $lineage `
            --ks $oldKs --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android `
            --next-signer `
            --ks $certFile --ks-key-alias $keyAlias --ks-pass "pass:$certPass" --key-pass "pass:$certPass" `
            $apkPath
        if ($LASTEXITCODE -ne 0) {
            throw "apksigner sign failed for $apkPath"
        }
        & $apksignerItem.FullName verify $apkPath
        if ($LASTEXITCODE -ne 0) {
            throw "apksigner verify failed for $apkPath"
        }
    }
}

Write-Host "Copying and signing APKs with canonical STORM_SWITCH_$version naming to $filesDir and $assemblingDir..."

if (Test-Path $mainline) {
    $dst1 = "$filesDir\STORM_SWITCH_$version.apk"
    Copy-Item $mainline $dst1 -Force
    Sign-ApkWithRotation $dst1
    Copy-Item $dst1 "$assemblingDir\STORM_SWITCH_$version.apk" -Force
}

if (Test-Path $legacy) {
    $dst2 = "$filesDir\STORM_SWITCH_${version}_LEGACY.apk"
    Copy-Item $legacy $dst2 -Force
    Sign-ApkWithRotation $dst2
    Copy-Item $dst2 "$assemblingDir\STORM_SWITCH_${version}_LEGACY.apk" -Force
}

if (Test-Path $sdk27) {
    $dst3 = "$filesDir\STORM_SWITCH_${version}_SDK27.apk"
    Copy-Item $sdk27 $dst3 -Force
    Sign-ApkWithRotation $dst3
    Copy-Item $dst3 "$assemblingDir\STORM_SWITCH_${version}_SDK27.apk" -Force
}

Get-ChildItem -Path $filesDir -Filter "*.idsig" | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path $filesDir -Filter "*_Android*.apk" | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir -Filter "*_Android*.apk" | Remove-Item -Force -ErrorAction SilentlyContinue

Get-ChildItem -Path $filesDir | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Canonical $version APKs signed with V3 Key Rotation and copied to $filesDir and $assemblingDir successfully!"
