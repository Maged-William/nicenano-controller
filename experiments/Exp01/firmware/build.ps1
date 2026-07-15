param(
    [string]$OutputDir = "."
)

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectDir

Write-Host "Building firmware..."
pio run
if ($LASTEXITCODE -ne 0) { exit 1 }

$HexFile = ".pio\build\nicenano\firmware.hex"
$Uf2File = ".pio\build\nicenano\firmware.uf2"
$TempDir = "$env:TEMP\uf2tools"

if (-not (Test-Path "$TempDir\uf2conv.py")) {
    New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2conv.py" -OutFile "$TempDir\uf2conv.py" -UseBasicParsing
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2families.json" -OutFile "$TempDir\uf2families.json" -UseBasicParsing
}

Write-Host "Converting to UF2..."
python "$TempDir\uf2conv.py" $HexFile -c -f 0xADA52840 -o $Uf2File
if ($LASTEXITCODE -ne 0) { exit 1 }

$OutputPath = Resolve-Path $OutputDir
Copy-Item $Uf2File "$OutputPath\firmware.uf2" -Force
Write-Host "UF2 saved to $OutputPath\firmware.uf2"
