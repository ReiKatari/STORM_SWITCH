$version = "13.3.0"
$baseDir = Split-Path -Parent $PSScriptRoot
if (Test-Path "$PSScriptRoot\GIT-RELEASE") {
    $v = (Get-Content "$PSScriptRoot\GIT-RELEASE" -Raw).Trim()
    if ($v) { $version = $v }
}

$assemblingDir = Join-Path $baseDir "Assembling"
$filesDir = Join-Path $baseDir "Files"
$buildNinjaBin = Join-Path $PSScriptRoot "build_ninja\bin"

Write-Host "================================================="
Write-Host "Packaging STORM SWITCH $version Releases"
Write-Host "================================================="

if (-not (Test-Path $assemblingDir)) { New-Item -ItemType Directory -Path $assemblingDir -Force | Out-Null }
if (-not (Test-Path $filesDir)) { New-Item -ItemType Directory -Path $filesDir -Force | Out-Null }

Write-Host "Copying fresh compiled executables to $assemblingDir..."
Copy-Item "$buildNinjaBin\STORM_SWITCH.exe" "$assemblingDir\" -Force
Copy-Item "$buildNinjaBin\STORM_SWITCH-cli.exe" "$assemblingDir\" -Force
Copy-Item "$buildNinjaBin\STORM_SWITCH-room.exe" "$assemblingDir\" -Force

if (-not (Test-Path "$assemblingDir\7z.exe")) {
    if (Test-Path "C:\Program Files\7-Zip\7z.exe") {
        Copy-Item "C:\Program Files\7-Zip\7z.exe" "$assemblingDir\" -Force
        Copy-Item "C:\Program Files\7-Zip\7z.dll" "$assemblingDir\" -Force
    }
}
$sevenZip = "$assemblingDir\7z.exe"

$crtDir = "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT"
$ompDir = "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP"
if (Test-Path $crtDir) { Copy-Item "$crtDir\*.dll" "$assemblingDir\" -Force }
if (Test-Path $ompDir) { Copy-Item "$ompDir\*.dll" "$assemblingDir\" -Force }

$userDirs = @(
    "user\cache",
    "user\config",
    "user\config\custom",
    "user\keys",
    "user\load",
    "user\nand",
    "user\screenshots",
    "user\sdmc",
    "user\shader"
)
foreach ($d in $userDirs) {
    $fullPath = Join-Path $assemblingDir $d
    if (-not (Test-Path $fullPath)) { New-Item -ItemType Directory -Path $fullPath -Force | Out-Null }
}

$stagingRoot = Join-Path $env:TEMP "STORM_SWITCH_Release_Staging"
if (Test-Path $stagingRoot) { Remove-Item $stagingRoot -Recurse -Force }
New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null

$commonFiles = @("STORM_SWITCH.exe", "STORM_SWITCH-cli.exe", "STORM_SWITCH-room.exe", "7z.exe", "7z.dll")
$crtDlls = @(
    "concrt140.dll",
    "msvcp140.dll",
    "msvcp140_1.dll",
    "msvcp140_2.dll",
    "msvcp140_atomic_wait.dll",
    "msvcp140_codecvt_ids.dll",
    "vccorlib140.dll",
    "vcomp140.dll",
    "vcruntime140.dll",
    "vcruntime140_1.dll",
    "vcruntime140_threads.dll"
)

# Staging Windows 10
$stageWin10 = Join-Path $stagingRoot "win10"
New-Item -ItemType Directory -Path $stageWin10 -Force | Out-Null
foreach ($f in $commonFiles) {
    Copy-Item "$assemblingDir\$f" "$stageWin10\" -Force
}
foreach ($f in $crtDlls) {
    if (Test-Path "$assemblingDir\$f") {
        Copy-Item "$assemblingDir\$f" "$stageWin10\" -Force
    }
}
New-Item -ItemType Directory -Path (Join-Path $stageWin10 "user") -Force | Out-Null

# Staging Windows 11
$stageWin11 = Join-Path $stagingRoot "win11"
New-Item -ItemType Directory -Path $stageWin11 -Force | Out-Null
foreach ($f in $commonFiles) {
    Copy-Item "$assemblingDir\$f" "$stageWin11\" -Force
}
foreach ($f in $crtDlls) {
    if (Test-Path "$assemblingDir\$f") {
        Copy-Item "$assemblingDir\$f" "$stageWin11\" -Force
    }
}
New-Item -ItemType Directory -Path (Join-Path $stageWin11 "user") -Force | Out-Null

# Zip Windows 10
$zipWin10 = Join-Path $filesDir "STORM_SWITCH_${version}_Windows10.zip"
if (Test-Path $zipWin10) { Remove-Item $zipWin10 -Force }
Write-Host "Creating $zipWin10..."
Push-Location $stageWin10
& $sevenZip a -tzip -mx=9 "$zipWin10" *
Pop-Location

# Zip Windows 11
$zipWin11 = Join-Path $filesDir "STORM_SWITCH_${version}_Windows11.zip"
if (Test-Path $zipWin11) { Remove-Item $zipWin11 -Force }
Write-Host "Creating $zipWin11..."
Push-Location $stageWin11
& $sevenZip a -tzip -mx=9 "$zipWin11" *
Pop-Location

Remove-Item $stagingRoot -Recurse -Force -ErrorAction SilentlyContinue

Get-ChildItem -Path $filesDir | Unblock-File -ErrorAction SilentlyContinue
Get-ChildItem -Path $assemblingDir | Unblock-File -ErrorAction SilentlyContinue

$mTargetDir = "M:\CONSOLES\Nintendo Switch\STORM SWITCH"
if (Test-Path $mTargetDir) {
    Write-Host "Syncing executables to $mTargetDir..."
    foreach ($exe in @("STORM_SWITCH.exe", "STORM_SWITCH-cli.exe", "STORM_SWITCH-room.exe")) {
        $srcExe = Join-Path $assemblingDir $exe
        $dstExe = Join-Path $mTargetDir $exe
        if (Test-Path $srcExe) {
            try {
                Copy-Item $srcExe $dstExe -Force -ErrorAction Stop
            } catch {
                $oldExe = "$dstExe.old"
                Remove-Item $oldExe -Force -ErrorAction SilentlyContinue
                Move-Item $dstExe $oldExe -Force -ErrorAction SilentlyContinue
                Copy-Item $srcExe $dstExe -Force
                Remove-Item $oldExe -Force -ErrorAction SilentlyContinue
            }
        }
    }
}

Write-Host "Packaging complete for $version!"

