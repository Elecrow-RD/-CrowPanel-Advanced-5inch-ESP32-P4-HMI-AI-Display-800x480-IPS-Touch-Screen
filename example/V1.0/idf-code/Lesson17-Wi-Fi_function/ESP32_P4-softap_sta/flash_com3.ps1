# Build + flash this example on COM3.
#
# Why this script exists: on this machine ESP-IDF v5.5.4 is installed in the
# ESP-IDF Installation Manager layout (tools under ...\v5.5.4\tools), the
# "export.ps1" of that layout is not usable here, and ESP-IDF 5.5 does NOT set
# ESP_IDF_VERSION. The esp_wifi_remote/esp_hosted components need it:
#   - esp_hosted/Kconfig            -> $(ESP_IDF_VERSION) >= "6.1"
#   - esp_wifi_remote/Kconfig       -> orsource "./idf_v$ESP_IDF_VERSION/Kconfig.wifi.in"
# so it must be "5.5" (matching the idf_v5.5 folder), NOT "5.5.4".
# Without it the Kconfig parse fails, and with a wrong value the Wi-Fi remote
# options (CONFIG_WIFI_RMT_*) disappear and the build stops.
#
# Usage:  powershell -ExecutionPolicy Bypass -File .\flash_com3.ps1
#         powershell -ExecutionPolicy Bypass -File .\flash_com3.ps1 -Port COM5

param(
    [string]$Port = 'COM3',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

$IdfRoot  = 'E:\ESP-IDF\.espressif\v5.5.4\esp-idf'
$Tools    = 'E:\ESP-IDF\.espressif\v5.5.4\tools'
$PythonV  = "$Tools\python\v5.5.4\venv"

$env:IDF_PATH              = $IdfRoot
$env:IDF_TOOLS_PATH        = $Tools
$env:IDF_PYTHON_ENV_PATH   = $PythonV
$env:ESP_IDF_VERSION       = '5.5'          # major.minor, matches idf_v5.5
$env:ESP_ROM_ELF_DIR       = "$Tools\esp-rom-elfs\20241011"
$env:PATH = (@(
    "$Tools\cmake\3.30.2\bin",
    "$Tools\ninja\1.12.1",
    "$Tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64",
    "$Tools\riscv32-esp-elf\esp-14.2.0_20260121\riscv32-esp-elf\bin",
    "$PythonV\Scripts"
) -join ';') + ';' + $env:PATH

$idfPy = "$PythonV\Scripts\python.exe"
if (-not (Test-Path $idfPy)) { throw "Python env not found: $idfPy" }

$ports = [System.IO.Ports.SerialPort]::GetPortNames()
Write-Host "Serial ports on this PC: $($ports -join ', ')"
if ($ports -notcontains $Port) {
    throw "Port $Port not found. Connect/power the board (or pass -Port COMx). Note: COM1 is the on-board legacy port."
}

Push-Location $PSScriptRoot
try {
    if (-not $SkipBuild) {
        & $idfPy "$IdfRoot\tools\idf.py" build
        if ($LASTEXITCODE -ne 0) { throw "build failed" }
    }
    & $idfPy "$IdfRoot\tools\idf.py" -p $Port flash
    if ($LASTEXITCODE -ne 0) { throw "flash failed" }
} finally {
    Pop-Location
}

Write-Host "`nDone. For the serial log:  idf.py -p $Port monitor"
