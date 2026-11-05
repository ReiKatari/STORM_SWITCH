$ErrorActionPreference = "Stop"
$baseDir = Split-Path -Parent $PSScriptRoot
$version = '16.2.5'
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

# Sign APKs cleanly with canonical STORM SERTIFICATE (v1 + v2 + v3) without debug signatures
$secretsDir = "F:\STORM ALL PROJECTS\.storm-secrets"
$propsFile  = Join-Path $secretsDir "storm_signing.properties"

$env:JAVA_HOME = "C:\Users\ReiKa\.jdks\jdk-21.0.12.1+1"
$env:PATH = "$env:JAVA_HOME\bin;$env:PATH"
$apksignerItem = Get-ChildItem "C:\Users\ReiKa\AppData\Local\Android\Sdk\build-tools\*\apksigner.bat" | Sort-Object FullName -Descending | Select-Object -First 1
$zipalignItem  = Get-ChildItem "C:\Users\ReiKa\AppData\Local\Android\Sdk\build-tools\*\zipalign.exe" | Sort-Object FullName -Descending | Select-Object -First 1

function Sign-ApkClean([string]$apkPath) {
    if (-not (Test-Path $apkPath)) { return }
    if ((Test-Path $propsFile) -and ($null -ne $apksignerItem)) {
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

        Write-Host "Sanitizing and single-pass signing $(Split-Path $apkPath -Leaf) with STORM SERTIFICATE..."
        
        # 1. Strip any existing signature files (META-INF/*.SF, *.RSA, *.DSA, *.EC, MANIFEST.MF) to eliminate double-signing artifacts
        $tempClean = "$apkPath.clean.apk"
        $tempAligned = "$apkPath.aligned.apk"
        
        Add-Type -AssemblyName System.IO.Compression
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        
        $srcZip = [System.IO.Compression.ZipFile]::OpenRead($apkPath)
        if (Test-Path $tempClean) { Remove-Item $tempClean -Force }
        $dstZip = [System.IO.Compression.ZipFile]::Open($tempClean, [System.IO.Compression.ZipArchiveMode]::Create)
        
        foreach ($entry in $srcZip.Entries) {
            $fn = $entry.FullName
            if ($fn.StartsWith("META-INF/") -and (
                $fn.EndsWith(".SF") -or $fn.EndsWith(".RSA") -or $fn.EndsWith(".DSA") -or $fn.EndsWith(".EC") -or $fn -eq "META-INF/MANIFEST.MF"
            )) {
                continue
            }
            $newEntry = $dstZip.CreateEntry($fn, [System.IO.Compression.CompressionLevel]::Optimal)
            $srcStream = $entry.Open()
            $dstStream = $newEntry.Open()
            $srcStream.CopyTo($dstStream)
            $dstStream.Close()
            $srcStream.Close()
        }
        $dstZip.Dispose()
        $srcZip.Dispose()
        
        # 2. zipalign -p -f 4
        if ($null -ne $zipalignItem) {
            & $zipalignItem.FullName -p -f 4 $tempClean $tempAligned
            if ($LASTEXITCODE -eq 0 -and (Test-Path $tempAligned)) {
                Move-Item $tempAligned $apkPath -Force
                Remove-Item $tempClean -Force -ErrorAction SilentlyContinue
            } else {
                Move-Item $tempClean $apkPath -Force
            }
        } else {
            Move-Item $tempClean $apkPath -Force
        }

        # 3. Clean single-pass apksigner sign with v1, v2, v3 schemes
        & $apksignerItem.FullName sign `
            --v1-signing-enabled true `
            --v2-signing-enabled true `
            --v3-signing-enabled true `
            --v4-signing-enabled false `
            --ks $certFile --ks-key-alias $keyAlias --ks-pass "pass:$certPass" --key-pass "pass:$certPass" `
            $apkPath
        if ($LASTEXITCODE -ne 0) {
            throw "apksigner sign failed for $apkPath"
        }
        & $apksignerItem.FullName verify --verbose $apkPath
        if ($LASTEXITCODE -ne 0) {
            throw "apksigner verify failed for $apkPath"
        }
        $aapt2Item = Get-ChildItem "C:\Users\ReiKa\AppData\Local\Android\Sdk\build-tools\*\aapt2.exe" | Sort-Object FullName -Descending | Select-Object -First 1
        if ($null -ne $aapt2Item) {
            $badging = (& $aapt2Item.FullName dump badging $apkPath | Select-Object -First 1)
            if ($badging -match "versionCode='(\d+)'") {
                $vc = [int64]$matches[1]
                Write-Host "Verified $(Split-Path $apkPath -Leaf) versionCode=$vc"
                if ($vc -lt 34188413) {
                    throw "FATAL UPGRADE GUARD: versionCode ($vc) is not strictly greater than or equal to 34188413! In-place upgrade would fail with INSTALL_FAILED_VERSION_DOWNGRADE."
                }
            }
        }
    }
}

Write-Host "Copying and signing APKs with canonical STORM_SWITCH_$version naming to $filesDir and $assemblingDir..."

if (Test-Path $mainline) {
    $dst1 = "$filesDir\STORM_SWITCH_$version.apk"
    Copy-Item $mainline $dst1 -Force
    Sign-ApkClean $dst1
    Copy-Item $dst1 "$assemblingDir\STORM_SWITCH_$version.apk" -Force

    $dstPlay = "$filesDir\STORM_SWITCH_${version}_PlayStore.apk"
    Copy-Item $dst1 $dstPlay -Force
    Copy-Item $dst1 "$assemblingDir\STORM_SWITCH_${version}_PlayStore.apk" -Force
}

if (Test-Path $legacy) {
    $dst2 = "$filesDir\STORM_SWITCH_${version}_LEGACY.apk"
    Copy-Item $legacy $dst2 -Force
    Sign-ApkClean $dst2
    Copy-Item $dst2 "$assemblingDir\STORM_SWITCH_${version}_LEGACY.apk" -Force
}

if (Test-Path $sdk27) {
    $dst3 = "$filesDir\STORM_SWITCH_${version}_SDK27.apk"
    Copy-Item $sdk27 $dst3 -Force
    Sign-ApkClean $dst3
    Copy-Item $dst3 "$assemblingDir\STORM_SWITCH_${version}_SDK27.apk" -Force
}

Get-ChildItem -Path $filesDir -Filter "*.idsig" | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path $filesDir -Filter "*_Android*.apk" | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir -Filter "*_Android*.apk" | Remove-Item -Force -ErrorAction SilentlyContinue

Get-ChildItem -Path $filesDir | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir | Unblock-File -ErrorAction SilentlyContinue

Write-Host "Canonical $version APKs cleanly signed with STORM SERTIFICATE and copied to $filesDir and $assemblingDir successfully!"
