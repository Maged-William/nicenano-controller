# Exp11: TPS43 Touchpad Integration alongside ADS1015 + Dual-Gyro Mouse

## Hypothesis

The Azoteq TPS43 capacitive touchpad on I2C (address `0x74`) can be polled at 250Hz alongside the ADS1015 ADC on the same bus, with its relative X/Y deltas feeding into the existing dual-gyro HID mouse accumulator to produce a combined gyro+touchpad cursor control.

## Background

The TPS43 is an older Azoteq capacitive touch controller (predating the IQS5xx series). It communicates over I2C with a 16-byte register map starting at `0x000D`. It supports:
- Relative X/Y movement (16-bit signed)
- Absolute X/Y position
- Touch count, touch area, touch strength
- Gesture register (bit 0 = tap-to-click)

The TPS43 requires an explicit "End Communication Window" command (`0xEE 0xEE 0x00`) after every read transaction.

## Wiring

Per AGENTS.md: SDA=P0.17, SCL=P0.20, VCC=3.3V, GND=GND. No RDY or RST_N pins connected — the touchpad is polled unconditionally every tick.

## Execution Plan

1. **Branch:** `Exp11` from `Exp10`
2. **src/main.c** — Add TPS43 driver section:
   - Register definitions, I2C read helpers
   - `tps43_poll()`: write register pointer `0x00 0x0D`, read 16 bytes, send End Communication Window, parse relative X/Y and gesture
   - Non-linear response curve (`map_v()`) from eskarp reference: `0.7 * delta^1.6`
   - Float accumulation of touchpad deltas (same pattern as gyro)
   - Tap gesture → left mouse button with edge detection
   - Integrate into main loop: poll every tick, feed into `mouse_acc_x/y`
3. **Kconfig** — Add `menu "TPS43 Touchpad"` with enable, sensitivity, invert, tap enable toggles
4. **app.overlay** — No change needed (existing HID report covers buttons + 2-axis)
5. **prj.conf** — No change needed (I2C already on)
6. **CI** — GitHub Actions build producing UF2 artifact
7. **Flash** via Leonardo automation
8. **Verify** serial output + cursor behavior

## Success Criteria

- [ ] Firmware builds on GitHub Actions, produces UF2
- [ ] Serial output shows TPS43 finger count, mapped X/Y, and ADC CH0–CH3
- [ ] Cursor responds to both board rotation (gyro) and finger swipes (touchpad)
- [ ] Tap-to-click triggers left mouse button
- [ ] ADS1015 continues printing debug data at ~10Hz
- [ ] LED heartbeat at ~2.5Hz

## Challenges

- **No RDY pin** — poll at fixed 250Hz rate; if touchpad hasn't finished processing, read may return stale data
- **I2C bus sharing** — TPS43 + ADS1015 on same I2C bus; ensure no transaction conflicts
- **End Communication Window** — must be sent after every read or touchpad stops responding
- **Combined gyro+touchpad** — both inputs feed the same accumulator; tuning needed to avoid fighting
