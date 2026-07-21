# Exp16: Linear Hall Effect Sensors (49E) on ADS1015

## Hypothesis

Two 49E linear Hall effect sensors can replace one joystick for analog distance sensing via the ADS1015 ADC, with stable readings achieved through proper decoupling, lower sample rate, and zero-offset calibration.

## Execution Plan

1. **Hardware wiring** (user):
   - Keep Joystick A on ADS1015 A0/A1
   - Replace Joystick B with Hall sensor 1 on A2, Hall sensor 2 on A3
   - Add 0.1µF ceramic decoupling cap between VCC and GND at each 49E
   - Add optional 0.1µF output filter cap at each 49E (output to GND)
   - Ground any unused ADC channel

2. **Software changes** (implemented):
   - **`ads1015.h`** — Add `ADS1015_RATE_*` constants, `ads1015_set_data_rate()`, `ads1015_calibrate()`, `ads1015_read_calibrated()` API
   - **`ads1015.c`** — Default data rate changed from 1600 SPS to 128 SPS; add per-channel static offset array; calibrate() averages N samples on boot and stores offset; read_calibrated() subtracts offset from raw read
   - **`main.c`** — On boot: set 128 SPS data rate, calibrate channels 2 and 3 (32-sample average), use `ads1015_read_calibrated()` for Hall sensor debug output

3. **Branch**: `Exp16` (forked from `Exp15`)

## Noise Diagnosis

### Floating A3 → random garbage
Expected behavior. ADS1015 high-impedance (~1.5MΩ) inputs float to ambient EMI. Unused inputs must be grounded or left unread.

### Noisy A2 (49E connected, long breadboard wires)
Three causes identified:

1. **No VCC–GND decoupling** — The 49E's internal amplifier draws supply current; breadboard inductance causes VCC ripple. **Critical fix**: add 0.1µF ceramic cap across VCC–GND at each sensor.

2. **Long unshielded wires** — Act as antennas for environmental EMI. Partially mitigated by the output filter cap (output–GND 0.1µF), though the ADS1015's internal digital filter at 128 SPS is the dominant noise rejection mechanism.

3. **1600 SPS data rate** — The ADS1015's internal digital filter bandwidth scales with data rate. Dropping from 1600 SPS to 128 SPS gives ~3.5× noise reduction.

### Boot calibration
Sensors are zeroed at startup by averaging 32 samples. Subsequent reads subtract this offset so stationary readings center on zero.

## Success Criteria

- [ ] A2 Hall sensor reading changes smoothly with magnet proximity
- [ ] A3 Hall sensor reading changes smoothly with magnet proximity
- [ ] At rest (no magnet), both channels read ~0 ± noise
- [ ] Joystick A on A0/A1 still works normally
- [ ] Firmware builds on GitHub Actions, produces UF2 artifact
- [ ] Serial output shows calibrated Hall readings

## Challenges

- **Noise floor** — Even with decoupling and 128 SPS, long breadboard wires and the magnetically-noisy environment produce ~±N LSB noise. Software averaging (moving window or IIR) could further reduce this.
- **Temperature drift** — The 49E output voltage drifts with temperature (~0.1%/°C). Boot calibration fixes short-term offset but a long session may see drift. A continuous idle-recenter (like Steam Controller) could track it, but would fight intentional movement.
- **Single-ended vs differential** — Currently using single-ended mode (AINx vs GND). For noisy environments, differential mode (AINx vs AINy) with a pseudo-differential connection could improve CMRR.

## Conclusion

TBD after verification.

## Files Changed

```
zephyr-app/src/drivers/ads1015.h   — Added rate constants + calibration API
zephyr-app/src/drivers/ads1015.c   — 128 SPS default, offset array, calibrate/read_calibrated
zephyr-app/src/main.c              — Calibrate A2/A3 on boot, calibrated reads in debug output
experiments/Exp16/Exp16.md         — This document
Experiments.md                     — Add Exp16 entry
```
