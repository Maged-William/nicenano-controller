# Exp10: Dual-Gyro HID Mouse with Saturation-Weighted Crossfade

## Hypothesis

Two BMI160 sensors configured at different gyro ranges (±125°/s and ±500°/s), combined via saturation-weighted crossfade with asymmetric burst averaging (16:112 samples per tick), can produce low-noise, high-dynamic-range gyro data suitable for driving a USB HID mouse cursor at 250Hz from a nice!nano nRF52840.

## Background

Alpakka firmware uses a proven approach for dual-IMU gyro fusion:
1. **Sensors at different ranges** — one low-range (±125°/s) for precision, one high-range (±500°/s) for headroom
2. **Asymmetric burst averaging** — 128 total samples per tick, biased 7:1 toward the low-range sensor (112:16) for better noise floor
3. **Saturation-weighted crossfade** — smoothly blends between sensors based on how close the low-range sensor is to saturation
4. **No cross-time smoothing on gyro** — latency priority
5. **HSSNF** (High-Speed Saturation Non-linearity Filter) — Schlick-bias curve for small-value amplification

## Execution Plan

1. **Branch:** `Exp10` from current `Exp09` state
2. **prj.conf** — Add `CONFIG_USB_DEVICE_HID=y`, keep CDC ACM, I2C, SPI. Add `CONFIG_PRINTK_FMT_FLOAT=y` for debug output.
3. **app.overlay** — Add `zephyr,hid-device` node under `&usbd` as composite device (CDC ACM + HID)
4. **src/main.c** — Rewrite with:
   a. **BMI160 config by sensor**:
      - Sensor 1 (CS=P0.31): gyro ±500°/s, accel ±2g
      - Sensor 2 (CS=P0.29): gyro ±125°/s, accel ±2g
   b. **Burst averaging**: 128 samples/tick split 16 (high-range) + 112 (low-range), each reading 6 bytes of gyro data
   c. **Saturation-weighted crossfade**:
      - `weight = max(|gyro_low.x|, |gyro_low.y|) / 32768.0`
      - `weight_high = ramp_mid(weight, 0.2)`
      - `weight_low = 1.0 - weight_high`
      - `fused = gyro_high * weight_high + (gyro_low / 4.0) * weight_low`
   d. **Sensitivity & HSSNF**: apply sensitivity scalar, then HSSNF non-linear curve to small movements
   e. **Float accumulation**: accumulate fractional pixel values, send integer portion via HID
   f. **250Hz loop** via deadline scheduling (`k_uptime_get()`)
   g. **ADS1015 kept**: read 4 channels every 25 ticks (10Hz) for serial debug
5. **Build via GitHub Actions CI** → produce UF2 artifact
6. **Flash** via Leonardo automation
7. **Verify** cursor movement in host OS and serial monitor

## Success Criteria

- [x] Firmware builds with no errors on GitHub Actions, produces UF2 artifact
- [x] nice!nano enumerates as composite USB: CDC ACM (serial) + HID (mouse)
- [x] Cursor moves with physical board rotation — smooth at low speeds, responsive at high speeds
- [x] Serial output shows fused gyro values and ADS1015 ADC readings
- [x] LED heartbeats at ~2.5Hz

## Challenges

- **SPI timing** at 250Hz (4ms tick): 128 burst samples + I2C ADC reads + math must fit in budget. SPI speed increased from 1MHz to 8MHz.
- **Crossfade artifacts** at the transition boundary between sensors — `ramp_mid(weight, 0.2)` creates a deadzone but may produce a noticeable blend region.
- **Composite USB** (CDC ACM + HID) — must follow Exp07's proven pattern.
- **Mouse feel** — sensitivity, deadzone width, burst ratio all need empirical tuning from real-world testing.
- **nRF52840 floating point** — single-precision FPU only; avoid double-precision.

## Implementation

### Fixed Register Bug

The previous implementation had incorrect register addresses:
- `BMI160_ACCEL_CONF` was mapped to 0x41 (actually ACCEL_RANGE) — fixed to 0x40
- `BMI160_GYRO_CONF` was mapped to 0x43 (actually GYRO_RANGE) — fixed to 0x42

This means Exp09 sensors were running at ±250°/s (not the intended ODR). Exp10 properly programs:
- Gyro range via `BMI160_GYRO_RANGE` (0x43): 0x04 = ±125°/s, 0x02 = ±500°/s
- Accel range via `BMI160_ACCEL_RANGE` (0x41): 0x03 = ±2g
- ODR via `BMI160_ACCEL_CONF` (0x40) and `BMI160_GYRO_CONF` (0x42): 0x0C = 1600 Hz

### Data Flow

```
BMI160 #1 (±500°/s, 16 samples/tick) ─┐
                                      ├─ crossfade → fused_gyro → sensitivity → HSSNF → accumulate → HID report
BMI160 #2 (±125°/s, 112 samples/tick) ─┘

ADS1015 (4 channels, every 25th tick) → serial debug
```

## Conclusion

**Hypothesis confirmed.** Two BMI160 sensors at different gyro ranges (±125°/s and ±500°/s), combined via saturation-weighted crossfade with asymmetric burst averaging, successfully drive a USB HID mouse cursor from the nice!nano nRF52840.

### What Worked

- ✅ **Composite USB** — CDC ACM (serial) + HID (mouse) coexisting on one device
- ✅ **Dual-range init** — BMI160 sensors properly configured at ±125°/s and ±500°/s (register addresses fixed from Exp09)
- ✅ **Burst averaging** — 128 total samples (16 high + 112 low) per tick provides noise reduction
- ✅ **Saturation-weighted crossfade** — `ramp_mid(weight, 0.2)` smoothly blends between low-range precision and high-range headroom
- ✅ **HSSNF filter** — Schlick-bias curve amplifies small movements, improving fine cursor control feel
- ✅ **Startup calibration** — 500-sample stationary offset measurement subtracts gyro bias, eliminating drift at rest
- ✅ **Accumulation deadzone** — Sub-0.5 pixel threshold prevents residual noise from accumulating into cursor movement
- ✅ **Float accumulation** — Fractional pixel values accumulate between ticks, sub-pixel precision preserved
- ✅ **250Hz loop** — `next_tick` properly initialized after init phase; consistent 250Hz
- ✅ **ADS1015 preserved** — Joystick channels continue to stream for debug alongside gyro data
- ✅ **Full Kconfig exposure** — All 35+ tunable parameters exposed as `CONFIG_GYRO_MOUSE_*` symbols in `zephyr-app/Kconfig`
- ✅ **Per-axis source + invert** — `X_SOURCE`, `Y_SOURCE`, `X_INVERT`, `Y_INVERT` for arbitrary gyro→mouse mappings
- ✅ **DTR timeout** — `CONFIG_GYRO_MOUSE_DTR_TIMEOUT_MS` (default 0) skips serial wait, boots HID immediately on plug-in
- ✅ **Hardcoded calibration** — `CONFIG_GYRO_MOUSE_HARDCODED_CAL` + six per-sensor per-axis offset values, bypasses auto-cal

### What Was Fixed

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| **Loop running at >1000Hz** | `next_tick` captured before ~2s of init code → deadline always in past → sleep skipped | Moved `next_tick = k_uptime_get()` to right before `while(1)` |
| **Wrong register addresses** | `BMI160_ACCEL_CONF` mapped to 0x41 (actually 0x40), `BMI160_GYRO_CONF` mapped to 0x43 (actually 0x42) | Corrected to 0x40 and 0x42; range writes go to 0x41 and 0x43 |
| **Gyro drift at rest** | Static bias offset (~20–30 counts) accumulated in mouse float → reached ±1 px → cursor drifted | Startup calibration (500 samples) + accumulation deadzone (0.5 px) |
| **Turn-ratio mismatch** | User expected gyro X→mouse X, gyro Y→mouse Y | Removed accidental swap, then added `CONFIG_GYRO_MOUSE_SWAP_AXES` toggle |

### Kconfig Structure

```
zephyr-app/Kconfig (source "Kconfig.zephyr" + custom menu)
├── GYRO_MOUSE_ENABLE               # Master toggle
├── GYRO_MOUSE_SWAP_AXES            # Axis swap
├── Sensor Hardware                  # SPI freq, CS pins, ranges, ODR
├── Sampling & Timing                # Burst counts, tick period
├── Mouse Sensitivity                # NUM/DENOM, deadzone
├── Filters                          # HSSNF enable/t/k, crossfade z
├── Calibration                      # Auto-cal, sample count, hardcoded offsets
└── Debug                            # ADC/LED decimation
```

All floats represented as int ×1000 (e.g., `CROSSFADE_Z` range 50–450 = 0.05–0.45). Gyro range and ODR are human-readable (125, 500, 1600…) with `#elif` chains converting to BMI160 register values.

### Per-Axis Source + Invert

Replaced the simple `SWAP_AXES` bool with independent per-axis source and invert:

```
CONFIG_GYRO_MOUSE_X_SOURCE      # 0=X, 1=Y, 2=Z (default 0)
CONFIG_GYRO_MOUSE_X_INVERT      # bool (default n)
CONFIG_GYRO_MOUSE_Y_SOURCE      # 0=X, 1=Y, 2=Z (default 1)
CONFIG_GYRO_MOUSE_Y_INVERT      # bool (default n)
```

This enables arbitrary mappings like **Z→mouse X, Y→-mouse Y**:
```
CONFIG_GYRO_MOUSE_X_SOURCE=2
CONFIG_GYRO_MOUSE_Y_SOURCE=1
CONFIG_GYRO_MOUSE_Y_INVERT=y
```

### DTR Timeout

Added `CONFIG_GYRO_MOUSE_DTR_TIMEOUT_MS` (default `0`). When `0`, the mouse boots immediately without waiting for a serial terminal — no need to open Putty/Serial Monitor for the HID to work. When set to e.g. `5000`, waits up to 5s for a terminal to connect before proceeding anyway.

### Build Time

| Run | Time | Notes |
|-----|------|-------|
| First Kconfig attempt | 4m 55s | Failed — `CONFIG_PRINTK_FMT_FLOAT` undefined |
| Kconfig fix | 5m 35s | Clean build |
| Register fix + cal | 5m 11s | Added burst averaging, startup calibration, deadzone |
| Flash persistence | 6m 01s | Failed — linker error `__device_dts_ord_73` |
| Final (Kconfig exposure) | 6m 11s | Clean build, all 30+ options exposed |

### Outstanding

- **Flash persistence** — Saving calibration offsets to the `storage_partition` (0xEC000) failed with a linker error (`__device_dts_ord_73` — the nRF52 flash driver device ordinal wasn't resolved). This requires adding `CONFIG_SOC_FLASH_NRF=y` and using `DT_CHOSEN(zephyr_flash)` for the device binding. Worth revisiting in a follow-up experiment.
