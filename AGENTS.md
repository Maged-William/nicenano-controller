# Rules

- Don't scan other projects or folders outside this folder.
- Never visit any other folder unless it's `G:/` to copy uf2 to.
- We are starting Fresh, ignore any other attempts in other folders.

# Background

Using a nice nano V2 nrf52840 microcontroller, i want to test an i2c ADC module, the module is either ADS1015 or ADS1115, 12-bit or 16-bit, i need to identify it.


# Wiring

The following is how im currently wiring things:

| nice!nano Pin | ADC Pin | Description |
| --- | --- | --- |
| VCC | VDD | Power (use 3.3V) |
| GND | GND | Ground | 
| P0.17 | SDA | I2C Data | 
| P0.20 | SCL | I2C Clock |
| float | ADDR | I2C Address |

| ADC | joystick A | joystick B |
| --- | --- | --- |
| A0 | X | |
| A1 | Y | |
| A2 | | X |
| A3 | | Y |
| GND | GND | GND |
| VCC | VCC | VCC |

# Development

Use the following repo to develop for the nicenano nrf52 supermini clone:
https://github.com/ICantMakeThings/Nicenano-NRF52-Supermini-PlatformIO-Support


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
cd experiments\Exp01\firmware
.\build.ps1                # builds + converts to UF2
```

Or manually:
```
pio run
python uf2conv.py .pio\build\nicenano\firmware.hex -c -f 0xADA52840
```

The `.uf2` is saved to the project root.

## Flashing

Double-tap RST with GND to enter bootloader, then copy `firmware.uf2` to the `NICENANO` drive (`G:/`).

> side quest: Invest in making this autonomous

# Experiments

In this project we will conduct serise of experiments, each session will concern itself with one experiment.

To conduct an experiment ask the user about a topic, and you may suggest a topic then ask the user if it's aligned with the user needs.

An experiment is an .md file lives in /experiments/[ExpXX]/[ExpXX].md

An overview of all the experiments exists in /Experiments.md, which is a simple table with the id, status, short desc.

Each time you starting an experiment, you create a branch (ex: Exp02) where all the implementation exists in it.

Update the Experiment documents with your hypothesis, execution plan, Success Criteria, Challenges (if any)

When the experiment is done, Commit any uncommited changes, update the experiment document with your conclusion, update Experiments.md status.

# Success

You are successful when you have:
1. program and flashed the nicenano v2 using Arduino or PlatformIO
2. Identified the ADC.
3. Live reading from the joystick.
