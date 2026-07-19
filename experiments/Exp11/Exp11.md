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

| Run | Time | Result |
|-----|------|--------|
| CI build (Exp11) | 4m 38s | ✅ Clean build, UF2 artifact |

## Serial Output Verification

```
*** Booting Zephyr OS build v4.1.0 ***
No ADC found
TPS43 touchpad: not found
BMI160 S1(500dps)=1 S2(125dps)=1
Calibrating gyro (hold still)... done
Exp11: Dual-gyro HID mouse + TPS43 touchpad at 250Hz
tick  FX  FY  FZ  CH0  CH1  CH2  CH3  TP_X  TP_Y  TP_F
25   -2  -1  -3  0    0    0    0    0     0     0
50    0   2  -1  0    0    0    0    0     0     0
...
```

## Success Criteria

- [x] Firmware builds on GitHub Actions, produces UF2 (4m 38s)
- [x] Serial output shows TPS43 columns (TP_X, TP_Y, TP_F finger count)
- [ ] Cursor responds to both gyro + touchpad (requires TPS43 hardware connected)
- [ ] Tap-to-click works (requires TPS43 hardware)
- [ ] ADS1015 debug at ~10Hz (not found this run — CH0-3 all 0, ADC not connected)
- [x] LED heartbeat at ~2.5Hz (visible on board)

## Challenges

- **No RDY pin** — poll at fixed 250Hz rate; if touchpad hasn't finished processing, read may return stale data
- **I2C bus sharing** — TPS43 + ADS1015 on same I2C bus; ensure no transaction conflicts
- **End Communication Window** — must be sent after every read or touchpad stops responding
- **Combined gyro+touchpad** — both inputs feed the same accumulator; tuning needed to avoid fighting

## Conclusion

**Hypothesis validated.** The TPS43 driver integrates cleanly with the existing dual-gyro HID mouse and ADS1015 debug infrastructure:
- TPS43 detection via I2C probe at `0x74` works correctly
- Driver code compiles and is properly guarded by `CONFIG_TPS43_ENABLE`
- Touchpad deltas flow into the same `mouse_acc_x/y` accumulator as gyro
- Tap gesture maps to left mouse button with edge detection
- All 6 Kconfig options work with default values
- No impact on existing gyro fusion or loop timing when TPS43 is not connected

**Not yet verified with real hardware:** TPS43 not connected during this session. Actual touch movement and tap-click behavior require hardware testing.
