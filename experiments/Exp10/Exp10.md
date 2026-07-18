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
- ✅ **Float accumulation** — Fractional pixel values accumulate between ticks, sub-pixel precision preserved
- ✅ **250Hz loop** — After fixing the next_tick initialization to post-init, loop runs at consistent 250Hz
- ✅ **ADS1015 preserved** — Joystick channels continue to stream for debug alongside gyro data

### What Was Fixed

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| **Loop running at >1000Hz** | `next_tick` captured before ~2s of init code → deadline always in past → sleep skipped | Moved `next_tick = k_uptime_get()` to right before `while(1)` |
| **Wrong register addresses** | `BMI160_ACCEL_CONF` mapped to 0x41 (actually 0x40), `BMI160_GYRO_CONF` mapped to 0x43 (actually 0x42) | Corrected to 0x40 and 0x42; range writes go to 0x41 and 0x43 |

### Build Time

| Run | Time | Notes |
|-----|------|-------|
| First build (fix Kconfig) | 4m 55s | Failed on undefined CONFIG_PRINTK_FMT_FLOAT |
| Second build (Kconfig fix) | 5m 35s | Clean build, UF2 artifact uploaded |
