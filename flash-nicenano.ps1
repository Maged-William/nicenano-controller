param(
    [Parameter(Mandatory=$true)]
    [string]$UF2Path,
    [string]$LeonardoCom = "COM8",
    [string]$NanoDrive = "G:"
)

Write-Host "=== NiceNano Headless Flasher (Leonardo + ZMK) ==="

Write-Host "[1/4] Sending 'b' to Leonardo on $LeonardoCom..."
$port = New-Object System.IO.Ports.SerialPort $LeonardoCom,115200,None,8,1
$port.ReadTimeout = 1000
$port.Open()
Start-Sleep -Milliseconds 500
$port.ReadExisting() | Out-Null
$port.WriteLine("b")
Start-Sleep -Seconds 1
try { $port.ReadExisting() | Out-Null } catch {}
$port.Close()

Write-Host "[2/4] Waiting for NICENANO drive..."
$found = $false
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 2
    $drive = Get-CimInstance Win32_LogicalDisk | Where-Object { $_.VolumeName -eq "NICENANO" }
    if ($drive) {
        Write-Host "[3/4] Found $($drive.DeviceID) — copying UF2..."
        Copy-Item $UF2Path "$($drive.DeviceID)\"
        Write-Host "[4/4] Done. Waiting for reboot..."
        $found = $true
        break
    }
    Write-Host "   ... waiting ($($i+1)/15)"
}

if (-not $found) {
    Write-Host "ERROR: NICENANO drive not found. Send 'r' to Leonardo to reset."
    exit 1
}

Start-Sleep -Seconds 3
Write-Host "Device should be running new firmware."
