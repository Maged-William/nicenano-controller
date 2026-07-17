param(
    [string]$OutputDir = ".",
    [string]$DriveLetter = "G",
    [int]$BaudRate = 115200,
    [int]$DriveTimeoutSec = 30,
    [string]$PortName = $null
)

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectDir

# ── Step 1: Build ────────────────────────────────────────────────────────────
Write-Host "Building firmware..."
& "$ProjectDir\build.ps1" -OutputDir $OutputDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

$Uf2File = Join-Path $OutputDir "firmware.uf2"
$DrivePath = "${DriveLetter}:\"

# ── Step 2: Find the serial port ─────────────────────────────────────────────
function Get-NiceNanoPort {
    $ports = [System.IO.Ports.SerialPort]::GetPortNames()
    $entities = Get-CimInstance Win32_PnPEntity -Filter "PNPClass = 'Ports'" -ErrorAction SilentlyContinue

    foreach ($p in $ports) {
        $match = $entities | Where-Object { $_.Name -like "*$p*" }
        if (-not $match) { continue }

        $hwid = $match.PNPDeviceID
        # nice!nano CDC ACM uses VID_1D50&PID_615E
        if ($hwid -match 'VID_1D50&PID_615E') {
            return [PSCustomObject]@{ Port = $p; Description = $match.Name; PNPDeviceID = $hwid }
        }
    }

    # Fallback: grab first "USB Serial Device" port
    foreach ($p in $ports) {
        $match = $entities | Where-Object { $_.Name -like "*$p*" -and $_.Name -like '*USB*' }
        if ($match) {
            return [PSCustomObject]@{ Port = $p; Description = $match.Name; PNPDeviceID = $match.PNPDeviceID }
        }
    }
    return $null
}

if (-not $PortName) {
    Write-Host "Scanning for nice!nano serial port..."
    $nano = Get-NiceNanoPort

    if (-not $nano) {
        Write-Host "Could not find nice!nano serial port." -ForegroundColor Red
        Write-Host "Available ports:"
        [System.IO.Ports.SerialPort]::GetPortNames()
        Write-Host "`nSpecify the port manually: .\flash.ps1 -PortName COM22"
        exit 1
    }
    $PortName = $nano.Port
    Write-Host "Found: $($nano.Port) — $($nano.Description)"
}

# ── Step 3: Send bootloader command ──────────────────────────────────────────
Write-Host "Sending BOOTLOADER command to $PortName..."
try {
    $sp = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, None, 8, One
    $sp.Open()
    $sp.ReadTimeout = 500
    Start-Sleep -Milliseconds 200
    $sp.WriteLine("BOOTLOADER")
    $resp = $sp.ReadExisting()
    Write-Host "Response: $resp"
    $sp.Close()
} catch {
    Write-Host "Serial error: $_" -ForegroundColor Yellow
    Write-Host "Make sure Putty isn't using the port." -ForegroundColor Yellow
    exit 1
}

# ── Step 4: Wait for NICENANO drive ──────────────────────────────────────────
Write-Host "Waiting for NICENANO drive..."

$elapsed = 0
$foundDrive = $null
while ($elapsed -lt $DriveTimeoutSec) {
    $drives = Get-Volume -ErrorAction SilentlyContinue | Where-Object { $_.DriveType -eq 'Removable' -and $_.DriveLetter }
    foreach ($d in $drives) {
        $path = "$($d.DriveLetter):\"
        if ((Test-Path "$path\INFO_UF2.TXT") -or ($d.FileSystemLabel -match '(?i)(NICENANO|FEATHER|ADA)') -or (Get-ChildItem $path -ErrorAction SilentlyContinue | Where-Object { $_.Name -like '*.uf2' -or $_.Name -eq 'INFO_UF2.TXT' })) {
            $foundDrive = $path
            $foundLetter = $d.DriveLetter
            break
        }
    }
    if ($foundDrive) {
        Write-Host "Found NICENANO drive at ${foundLetter}:\"
        break
    }
    Start-Sleep -Seconds 1
    $elapsed++
}
if (-not $foundDrive) {
    Write-Host "Timed out waiting for NICENANO drive." -ForegroundColor Red
    Write-Host "Available removable drives:"
    Get-Volume | Where-Object DriveType -eq 'Removable' | Select-Object DriveLetter, FileSystemLabel
    exit 1
}

# ── Step 5: Copy UF2 ─────────────────────────────────────────────────────────
Start-Sleep -Seconds 1
$TargetPath = Join-Path $foundDrive "firmware.uf2"
Write-Host "Copying firmware.uf2 to $TargetPath..."
Copy-Item $Uf2File $TargetPath -Force
Write-Host "Copied!"

# ── Step 6: Wait for flash to complete ───────────────────────────────────────
Write-Host "Waiting for flash to complete..."
$elapsed = 0
while ($elapsed -lt 30) {
    if (-not (Test-Path $foundDrive)) {
        Write-Host "Drive disconnected — flash complete!" -ForegroundColor Green
        exit 0
    }
    Start-Sleep -Seconds 1
    $elapsed++
}
Write-Host "Timed out waiting for drive to disconnect." -ForegroundColor Yellow
Write-Host "The UF2 may still have been flashed successfully." -ForegroundColor Yellow
