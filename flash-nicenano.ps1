param(
    [Parameter(Mandatory=$true)]
    [string]$UF2Path,
    [string]$ComPort = "COM8"
)

Write-Host "=== NiceNano Headless Flasher ==="

Write-Host "[1/4] Entering bootloader..."
$port = New-Object System.IO.Ports.SerialPort $ComPort,115200,None,8,1
$port.ReadTimeout = 1000
$port.Open()
Start-Sleep -Milliseconds 500
$port.ReadExisting() | Out-Null
$port.WriteLine("devmem 0x4000051C 32 0x00000057")
Start-Sleep -Milliseconds 500
$port.WriteLine("kernel reboot cold")
Start-Sleep -Seconds 1
try { $port.ReadExisting() | Out-Null } catch {}
$port.Close()

Write-Host "[2/4] Waiting for NICENANO drive..."
$found = $false
for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Seconds 2
    $drive = Get-CimInstance Win32_LogicalDisk | Where-Object { $_.VolumeName -eq "NICENANO" }
    if ($drive) {
        Write-Host "[3/4] Found $($drive.DeviceID) — copying UF2..."
        Copy-Item $UF2Path "$($drive.DeviceID)\"
        Write-Host "[4/4] Done. Waiting for reboot..."
        $found = $true
        break
    }
    Write-Host "   ... waiting ($($i+1)/10)"
}

if (-not $found) {
    Write-Host "ERROR: NICENANO drive not found"
    exit 1
}

for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Seconds 2
    $com = Get-CimInstance Win32_SerialPort | Where-Object { $_.DeviceID -eq $ComPort }
    if ($com) {
        Write-Host "Device back online on $ComPort"
        exit 0
    }
}

Write-Host "Device rebooted (drive disconnected)"
