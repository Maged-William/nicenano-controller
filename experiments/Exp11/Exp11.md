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

## Implementation

### TPS43 Driver

The TPS43 driver (`main.c:215-283`) implements:

1. **Protocol**: Write register pointer `[0x00, 0x0D]`, repeated-start read 16 bytes, send End Communication Window `[0xEE, 0xEE, 0x00]`
2. **Register parsing**: Extract 16-bit signed relative X/Y from bytes 5-8, finger count from byte 4, tap gesture from byte 0 bit 0
3. **Non-linear response curve** (`tps43_map_v`): Quadratic curve `a²/512` — small finger movements are fine, large sweeps amplified
4. **Float accumulation**: Touchpad deltas add to `mouse_acc_x/y` (same accumulator as gyro) with configurable sensitivity scalar
5. **Button edge detection**: Tap gesture updates `mouse_buttons` only on state change
6. **Polling**: Every tick (4ms/250Hz), unconditional — no RDY pin needed

### Kconfig Options (`Kconfig:228-276`)

```
menu "TPS43 Touchpad"
├── TPS43_ENABLE             (bool, default y)
├── TPS43_SENSITIVITY_NUM    (int, 1-100, default 1)
├── TPS43_SENSITIVITY_DENOM  (int, 1-100, default 2)
├── TPS43_INVERT_X           (bool, default n)
├── TPS43_INVERT_Y           (bool, default n)
├── TPS43_TAP_ENABLE         (bool, default y)
└── TPS43_DEBUG              (bool, default n) — register dump
```

## Build Results

| Run | Time | Result | Commit |
|-----|------|--------|--------|
| Initial build | 4m 38s | ✅ Clean build | 8caadd0 |
| Linear raw scaling + sensitivity | 5m 17s | ✅ Clean build | 51f1e3d |
| Halve sensitivity to 2.5x | 5m 58s | ✅ Clean build | 060c983 |
| Gesture debug + 2-finger scroll + wheel | 5m 16s | ✅ Clean build | 2d93bda |
| Scroll sensitivity + horizontal wheel | 5m 56s | ✅ Clean build | 9c0effb |
| Scroll divider (60) | 5m 26s | ✅ Clean build | c5804ff |

### Features implemented (all working)

| Feature | How |
|---------|-----|
| Linear raw × sensitivity | Removed quadratic curve, raw delta × `NUM/DENOM` (default 5/2 = 2.5x) |
| 2-finger vertical scroll | Y delta → vertical wheel, divided by `SCROLL_DIVIDER` (60) |
| 2-finger horizontal scroll | X delta → horizontal wheel (AC Pan HID usage) |
| Independent scroll sensitivity | `SCROLL_SENS_NUM/DENOM` (default 2/1 = 2x) separate from cursor |
| 5-byte HID report | buttons, X, Y, wheel V, wheel H |
| Tap-to-click | Gesture0 bit 0 → left click |
| Gesture debug | `GST g0=0xXX g1=0xYY f=N` serial output on gesture change |

### Attempted but reverted: Tap-and-drag FSM

A libinput-inspired state machine (`IDLE→TOUCHING→HELD→ACTIVE`) with absolute position tracking was implemented across 7 commits but ultimately reverted due to:
- **Init instability**: Changed from `i2c_write_read` to `i2c_write(NULL)` probe — TPS43 stopped being detected on boot
- **Absolute position unreliability**: TPS43 absolute X/Y registers didn't update reliably enough for movement threshold detection
- **Complexity**: The combined FSM + absolute position tracking made the touchpad non-functional

## Serial Output Verification (final working build)

```
*** Booting Zephyr OS build v4.1.0 ***
ADC at 0x48
ADS1015 (12-bit)
TPS43 touchpad: found
BMI160 S1(500dps)=1 S2(125dps)=1
Calibrating gyro (hold still)... done
Exp11: Dual-gyro HID mouse + TPS43 touchpad at 250Hz
tick  FX  FY  FZ  CH0  CH1  CH2  CH3  TP_X  TP_Y  TP_F
```

## Success Criteria

- [x] Firmware builds on GitHub Actions, produces UF2
- [x] Serial output shows TPS43 columns (TP_X, TP_Y, TP_F finger count)
- [x] Cursor responds to both gyro + touchpad
- [x] Tap-to-click works
- [x] ADS1015 debug at ~10Hz
- [x] LED heartbeat at ~2.5Hz
- [x] 2-finger scroll (vertical + horizontal)
- [x] Configurable cursor and scroll sensitivity via Kconfig

## Challenges

- **No RDY pin** — poll at fixed 250Hz rate; if touchpad hasn't finished processing, read may return stale data
- **I2C bus sharing** — TPS43 + ADS1015 on same I2C bus; ensure no transaction conflicts
- **End Communication Window** — must be sent after every read or touchpad stops responding
- **Combined gyro+touchpad** — both inputs feed the same accumulator; tuning needed to avoid fighting
- **Init reliability** — TPS43 requires `i2c_write_read` (not just addr probe) to wake up; timeout must be generous
- **Absolute position not reliable** — TPS43 abs X/Y registers don't track well enough for position-based drag detection

## Conclusion

**Hypothesis validated.** The TPS43 touchpad driver integrates cleanly with the existing dual-gyro HID mouse and ADS1015 debug infrastructure:
- TPS43 detection via `i2c_write_read` at `0x74` with 5 retries
- 16-bit relative X/Y deltas → linear raw × configurable sensitivity → `mouse_acc_x/y` accumulator
- Tap gesture (gesture0 bit 0) → left mouse button with edge detection
- 2-finger scrolling with independent sensitivity, both vertical and horizontal axes
- 14 Kconfig options covering enable, cursor sensitivity, scroll sensitivity, scroll divider, axis invert, tap enable, debug
- Horizontal wheel via Consumer page AC Pan HID usage (5-byte report)
- No impact on existing gyro fusion or loop timing

### What didn't work

**Tap-and-drag** proved unreliable on this hardware. The TPS43 absolute X/Y registers don't track finger position consistently enough for position-delta-based drag detection, and altering the I2C init sequence from `i2c_write_read` to a simple probe broke detection entirely. A future experiment could revisit drag with a timer-only approach (no absolute position), or by using relative delta integration to track movement from tap origin.

### Late addition: Background retry on init failure

When the TPS43 doesn't respond during boot init (e.g., not yet powered/ready), `tps43_poll()` now attempts a lightweight probe every 4ms tick without blocking the main loop (`tps43.c:59-72`). Once the sensor becomes ready and the probe succeeds, it sets `tps43_found = true` and a `"TPS43 touchpad: found (late init)"` message appears in the serial output — no reboot required. This covers the common case where the sensor is physically connected but takes longer than the boot retry window (~1s) to initialize.
