# Exp19: ADS1015 Game Joystick via zmk-hid-io

## Hypothesis

The existing `zmk-ads1015-input` I2C ADC driver (from Exp18) can be re-routed to `zmk-hid-io`'s joystick HID interface (HID_1) to present the joystick as a USB HID gamepad with 6 axes and 8 buttons, replacing the mouse-pointer use case entirely.

## Execution Plan

1. **Branch**: `Exp19` (forked from `Exp18`)
2. **New module in west.yml**: `zmk-hid-io` (badjeff/zmk-hid-io) — provides second HID device with joystick report descriptor
3. **Modify** `zmk-ads1015-input/src/ads1015_input.c`:
   - Track virtual joystick position (`joy_x`/`joy_y`) and emit `delta = target - previous` so REL-axis accumulation tracks stick position in both directions
   - Map raw ADC displacement to int8 range [-127, +127]
   - Always emit events (including zero) to keep host-side position synchronized
4. **Modify** `boards/shields/my_shield/my_shield.overlay`:
   - Remove `zmk,input-listener` mouse pointing node
   - Add `zmk,input-processor-fwd-to-hid-io` to route ADS1015 events → joystick
   - Add `zmk,behavior-hid-io-key-press` + `zmk,input-behavior-listener` for joystick buttons
5. **Modify** `boards/shields/my_shield/my_shield.conf`:
   - Remove `CONFIG_ZMK_POINTING`
   - Add `CONFIG_ZMK_HID_IO=y`, `CONFIG_ZMK_HID_IO_JOYSTICK=y`, `CONFIG_USB_HID_DEVICE_COUNT=2`
6. **Modify** `boards/shields/my_shield/my_shield.keymap`:
   - First 8 keys → `&hidiokp 0`–`7` (joystick buttons)
   - Remaining keys → keyboard (`&kp`)
7. **Build**: Via GitHub Actions CI
8. **Flash & Verify**: Leonardo → UF2 copy → USB logging, verify host sees gamepad

## Wiring

Same as Exp18 — no hardware changes:

| nice!nano Pin | ADS1015 Pin | Description |
|--------------|-------------|-------------|
| P0.17 | SDA | I2C Data |
| P0.20 | SCL | I2C Clock |
| VCC | VDD | 3.3V |
| GND | GND | Ground |
| float | ADDR | Address 0x48 |

| ADS1015 | Joystick |
|---------|----------|
| A0 | X axis |
| A1 | Y axis |

## Success Criteria

- [ ] Host recognizes device as USB HID gamepad/joystick (not mouse)
- [ ] X and Y axes respond to joystick movement
- [ ] Axes recenter when stick is released
- [ ] Joystick buttons 1–8 fire from keyboard key presses
- [ ] Keyboard keys still function as keyboard
- [ ] BLE advertising works (if tested)
- [ ] No mouse pointer movement from joystick

## Challenges

- zmk-hid-io joystick HID report uses REL axes (not ABS) — each report is a delta from previous host position, not an absolute position
- The driver must track a "virtual" joystick position and emit position-change deltas, not center-offset values
- When the stick is released, the driver must emit negative deltas to recenter the host-side accumulator
- `zmk-hid-io` requires `CONFIG_USB_HID_DEVICE_COUNT=2` — conflicts or interactions with ZMK's default HID_0 (keyboard) must be avoided

## Build Iterations

| Attempt | Issue | Fix |
|---------|-------|-----|
| 1 | Initial zmk-hid-io integration — joystick detected but REL-axis accumulator desyncs | Switch to ABS axes in descriptor |
| 2 | Per-axis squared curve causes diagonal distortion | Add radial (magnitude-based) curve |
| 3 | Radial curve still feels non-linear | Remove all curves, pure linear divisor-100, deadzone-4 |
| 4 | 16-bit HID axes (±32767) with squared curve — worse diagonal feel | Revert to 8-bit, keep divisor-100 linear |
| 5 | Pure linear 8-bit — smooth but values repeat at 45° cardinals | Identified quantization: 100 ADC units per HID step |
| 6 | 16-bit HID axes pure linear (divisor 16384) — no more mid-range sticking but can't reach ±1.000 | Divisor 16384 → 13000 |
| 7 | Divisor 13000 + 16-bit — ±1.000 reachable but clamp at edges eats dither | Divisor → 13250 then 13500 then 13600, add ±8 ADC dither |
| 8 | RAW mode: ADC displacement sent directly, zero processing | **Key finding: raw ADC repeats values for 3-30 frames** |

## Known-Good Commit

`f641ccf` — RAW mode final test

## Conclusion

**Status: Successful finding — hardware limitation identified.**

The perceived "snapping" at the 12/3/6/9 o'clock positions and 45° diagonals is caused by the **KY-023 joystick module** itself, not the firmware:

1. **12-bit ADC quantization**: The ADS1015 at ±4.096V PGA produces only 2048 distinct values (16 register units per step). With a joystick throw of ~13000 register units, each 12-bit code represents ~6.4 ADC units of physical throw. When moving slowly, the ADC reads the same code for 3-30 consecutive polls (9-90ms).

2. **Mechanical square gate**: The KY-023 uses a plastic restrictor that creates a rectangular path — 45° diagonal snapping is the stick hitting the gate edge, not firmware.

3. **Carbon-track potentiometers**: Wiper contact noise and friction cause voltage micro-stutters that the ADC faithfully digitizes.

All firmware approaches were tried: 8-bit/16-bit axes, divisor scaling, curves (per-axis and radial), EMA smoothing, dithering, deadzone tuning — none could overcome the hardware limit because **the ADC simply reads the same voltage repeatedly**.

**Recommendation**: Replace the KY-023 with a hall-effect joystick (e.g., ALPS with magnetic sensors, Gulikit, or PS5-style) for smooth, gapless readings across a circular gate.
