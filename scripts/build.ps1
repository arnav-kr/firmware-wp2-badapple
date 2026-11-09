param(
    [Parameter(Mandatory=$true)][string]$SdkRoot,
    [Parameter(Mandatory=$true)][string]$ToolchainRoot,
    [string]$Python = 'python',
    [string]$CMake = 'cmake',
    [string]$Ninja = 'ninja',
    [int]$Jobs = 8,
    [switch]$Package,
    [switch]$Preflight,
    [string]$EmulatorRoot = (Join-Path $PSScriptRoot '../../qemu-wp2')
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
$boardName = 'ats3089c_dev_watch'
$boardRoot = "$projectRoot/build/board-root"
$boardPath = "boards/arm/$boardName"
$sdkBoard = Join-Path $env:ZEPHYR_BASE $boardPath
$localBoard = Join-Path $boardRoot $boardPath
New-Item -ItemType Directory -Path $localBoard -Force | Out-Null
Get-ChildItem -LiteralPath $sdkBoard -File | Copy-Item -Destination $localBoard -Force
$defconfig = Join-Path $localBoard "${boardName}_defconfig"
Get-Content -LiteralPath (Join-Path $sdkBoard "${boardName}_defconfig") |
    Where-Object { $_ -notmatch '^CONFIG_(SPI_ACTS|SPIMT_ACTS|MPU_MONITOR_USER_DATA|MPU_MONITOR_ROMFUNC_WRITE|MINIMAL_LIBC_MALLOC_ARENA_SIZE)=' } |
    Set-Content -LiteralPath $defconfig -Encoding ascii
$boardCmake = Join-Path $localBoard 'CMakeLists.txt'
(Get-Content -Raw -LiteralPath $boardCmake).Replace('zephyr_library()', "zephyr_library_named(boards__arm__$boardName)") |
    Set-Content -LiteralPath $boardCmake -Encoding ascii -NoNewline
& $Python "$PSScriptRoot/media.py" embed
if ($LASTEXITCODE -ne 0) { throw 'Asset validation/generation failed' }
$configFiles = 'prj.conf;boards/ats3089c_dev_watch.conf;../config/power.conf;../config/input.conf;../config/media.conf'
& $CMake -S "$projectRoot/app" -B "$projectRoot/build" -G Ninja "-DBOARD=$boardName" "-DBOARD_ROOT=$boardRoot" "-DBOARD_DIR=$($localBoard.Replace('\', '/'))" "-DUSER_CACHE_DIR=$projectRoot/build/cache" "-DCONF_FILE=$configFiles" "-DPYTHON_EXECUTABLE=$pythonPath" "-DWEST_PYTHON=$pythonPath" "-DCMAKE_MAKE_PROGRAM=$ninjaPath"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& $CMake --build "$projectRoot/build" --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
if ($Package -or $Preflight) {
    & $Python "$PSScriptRoot/package.py" --sdk-root $SdkRoot
    if ($LASTEXITCODE -ne 0) { throw 'Packaging or validation failed' }
}
if ($Preflight) {
    & $Python "$PSScriptRoot/preflight.py" "$projectRoot/dist/wp2-badapple-ota.bin" --emulator $EmulatorRoot
    if ($LASTEXITCODE -ne 0) { throw 'Strict QEMU preflight failed' }
}
