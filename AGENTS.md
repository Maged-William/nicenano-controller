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

Use WSL to build the .uf2, save it here `/mnt/d/DIY/controller/ideation/collection`

I'll double tap the RST with GND to enter the bootloader and copy the files.

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
