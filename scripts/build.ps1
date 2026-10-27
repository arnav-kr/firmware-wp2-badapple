param(
    [Parameter(Mandatory=$true)][string]$SdkRoot,
    [Parameter(Mandatory=$true)][string]$ToolchainRoot,
    [string]$Python = 'python',
    [string]$CMake = 'cmake',
    [string]$Ninja = 'ninja',
    [int]$Jobs = 8,
    [switch]$Package
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$env:ZEPHYR_BASE = Join-Path (Resolve-Path $SdkRoot) 'zephyr'
$env:ZEPHYR_TOOLCHAIN_VARIANT = 'gnuarmemb'
$env:GNUARMEMB_TOOLCHAIN_PATH = (Resolve-Path $ToolchainRoot).Path
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
$pythonPath = (Get-Command $Python -ErrorAction Stop).Source
$ninjaPath = (Get-Command $Ninja -ErrorAction Stop).Source
& $Python "$PSScriptRoot/media.py" embed
if ($LASTEXITCODE -ne 0) { throw 'Asset validation/generation failed' }
$configFiles = 'prj.conf;boards/ats3089c_dev_watch.conf;../config/power.conf;../config/input.conf;../config/media.conf'
& $CMake -S "$projectRoot/app" -B "$projectRoot/build" -G Ninja -DBOARD=ats3089c_dev_watch "-DCONF_FILE=$configFiles" "-DPYTHON_EXECUTABLE=$pythonPath" "-DWEST_PYTHON=$pythonPath" "-DCMAKE_MAKE_PROGRAM=$ninjaPath"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& $CMake --build "$projectRoot/build" --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
if ($Package) {
    & $Python "$PSScriptRoot/package.py" --sdk-root $SdkRoot
    if ($LASTEXITCODE -ne 0) { throw 'Packaging or validation failed' }
}
