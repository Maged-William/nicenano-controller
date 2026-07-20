# Rules
- Translate everything in English English English
- Be autonomous, don't inturrupt the flow.
- Never ask the user to read the serial monitor, you the Assistant should read it, use Putty, most likely you want COM30 115200.
- Don't scan other projects or folders outside this folder.
- We are starting Fresh, ignore any other attempts in other folders.
- Always assume i want to flash (AAIWTF), never ask if i want to flash, proceed
- if i asked to flash an old firmware, download the artifact, do not re build it cost time.
- Follow this document on how to flash
- Always Check The Serial Monitor (ACTSM) after every flash (no exceptions).

# Background

Using a nice nano V2 nrf52840 microcontroller, i have an i2c ADC module ADS1015 12-bit, and 2 BMI160 sensors over SPI.

# Wiring

The following is how im currently wiring things:

| nice!nano Pin | ADC Pin | Description |
| --- | --- | --- |
| VCC | VDD | Power (use 3.3V) |
| GND | GND | Ground | 
| P0.17 | SDA | I2C Data | 
| P0.20 | SCL | I2C Clock |
| float | ADDR | I2C Address |

| i2c ADC | joystick A | joystick B |
| --- | --- | --- |
| A0 | X | |
| A1 | Y | |
| A2 | | X |
| A3 | | Y |
| GND | GND | GND |
| VCC | VCC | VCC |

| nice!nano pin  | i2c Azoteq TPS43 |
| --- | --- |
| P0.17 | SDA |
| P0.20 | SCL |
| GND | GND |
| VCC | VCC |

| BMI160 Sensor 1 Pin | BMI160 Sensor 2 Pin | nice!nano V2 | Pin Function Description |
| --- | --- | --- | --- |
| 3V3 | 3V3 | 3V3 | Power (3.3V) |
| GND | GND | GND | Ground |
| SCL | SCL | P0.06 | SPI Clock (SCK) |
| SDA | SDA | P0.08 | SPI Data In (MOSI) |
| SA0 | SA0 | P0.02 | SPI Data Out (MISO) |
| CS | Leave separate | P0.30 | CS for Sensor 1 |
| Leave separate | CS | P0.29 | CS for Sensor 2 |

Leonardo pin 9 → 1kΩ → NPN base, NPN collector → nice!nano RST, emitter → GND (common ground).

**Find the Leonardo COM port:**
```
Get-PnpDevice -Class Ports | Select-Object FriendlyName, Class, InstanceId
```
Look for `Arduino Leonardo (COMx)`. If multiple are connected, send `x` to each and listen for the `"Reset helper ready"` response.


# Development

Use the following repo to develop for the nicenano nrf52 supermini clone:
https://github.com/ICantMakeThings/Nicenano-NRF52-Supermini-PlatformIO-Support

## The dev loop:

As long as you havent reached the desired outcome, keep on looping, only the user will break this and ask about your status.

Write the Code -> build it -> Send 'b' to Leonardo -> copy the firmware to the nicenano -> using Putty inspect the serial monitor

If you are using github action:
Write the code -> commit, push -> check how much a successful build takes, note it, wait that amount -> check the logs

$runId = (gh run list --branch Exp06 --limit 1 --json databaseId --jq '.[0].databaseId'); Write-Output "Run ID: $runId"; gh run watch $runId --interval 15

# Building & Flashing

## Build (Windows)

The ARM toolchain requires glibc — Alpine WSL (musl) is incompatible. Build on Windows directly.

### Prerequisites (one-time)

PlatformIO must have the nicenano board definition and variant files:

1. Install PlatformIO: `pip install platformio`
2. Copy `nicenano.json` to `%USERPROFILE%\.platformio\platforms\nordicnrf52\boards\`
3. Copy `variant.h` and `variant.cpp` to `%USERPROFILE%\.platformio\packages\framework-arduinoadafruitnrf52\variants\nicenano\`

These files are from: https://github.com/ICantMakeThings/Nicenano-NRF52-Supermini-PlatformIO-Support

### Build

```
cd firmware
.\build.ps1                # builds + converts to UF2
```

Or manually:
```
pio run
python uf2conv.py .pio\build\nicenano\firmware.hex -c -f 0xADA52840
```

The `.uf2` is saved to the project root.

## Flashing

The nicenano require a Double-tap RST with GND to enter bootloader, But to make it autonomous, we are using the Arduino Leonardo with an NPN transistor and a 1kohm to short the RST and GND on command.
simply detect the leonardo and send 'b' and wait 3 seconds it should be in bootloader mode at (`G:/`) drive.

Then copy `firmware.uf2` to the `NICENANO` drive (`G:/`).

If you the nicenano stuck send 'r' to the leonardo it will send one pulse hence reseting the nicenano


# Insepcting the serial monitor

```
$ $port = New-Object System.IO.Ports.SerialPort "COM30", 115200, None, 8, 1
$port.ReadTimeout = 10000
$port.DtrEnable = $true
$port.Open()
Start-Sleep -Seconds 3
try {
    $data = $port.ReadExisting()
    Write-Output "=== SERIAL OUTPUT ==="
    Write-Output $data
} catch {
    Write-Output "No data received"
}
$port.Close()
```
Or use Putty


# Experiments

In this project we will conduct serise of experiments, each session will concern itself with one experiment.

To conduct an experiment ask the user about a topic, and you may suggest a topic then ask the user if it's aligned with the user needs.

An experiment is an .md file lives in /experiments/[ExpXX]/[ExpXX].md

An overview of all the experiments exists in /Experiments.md, which is a simple table with the id, status, short desc.

An experiment can only be one of the following state [In progress, Done, Failed, Abandoned, Partial, Unknown].

Each time you starting an experiment, you create a branch (ex: Exp02) where all the implementation exists in it.

Update the Experiment documents with your hypothesis, execution plan, Success Criteria, Challenges (if any)

When the experiment is done, Commit any uncommited changes, update the experiment document with your conclusion, update Experiments.md status.


# Resources

- https://github.com/inputlabs/alpakka_firmware/tree/main/src
- https://github.com/ICantMakeThings/Nicenano-NRF52-Supermini-PlatformIO-Support
- https://github.com/hanyazou/BMI160-Arduino
- https://github.com/thedalles77/USB_Laptop_Keyboard_Controller/blob/master/Example_Touchpads/Azoteq_TP.ino
- https://github.com/ilp0/qmk_firmware/blob/master/keyboards/disconnect72/IQS5xx.c
- https://github.com/rwalkr/eskarp/blob/main/firmware/device/src/touchpad.rs
- https://keycapsss.com/media/97/95/3a/1758971069/proxsense_i2c_trackpad_datasheet-1626845.pdf?ts=1758971108
- https://github.com/wayland-tablet/libinput
- https://wiki.archlinux.org/title/Libinput
- https://github.com/iberianpig/fusuma
- https://github.com/too1/ncs-esb-ble-mpsl-demo
- https://nrfconnectdocs.nordicsemi.com/ncs/2.6.4/nrf/samples/esb.html
- https://github.com/zmkfirmware/zmk/tree/main/app/src/split/wired
- https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/protocols/esb/index.html
- https://github.com/badjeff/zmk-feature-split-esb/
