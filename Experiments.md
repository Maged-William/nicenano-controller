# Experiments Overview

| ID | Status | Description |
| --- | --- | --- |
| Exp01 | ✅ Complete | Toolchain validation — Blinky (LED on P0.15) |
| Exp02 | ✅ Complete | ADC identification (ADS1015) & joystick reading via I2C |
| Exp03 | ✅ Complete | Read speed optimization — ready-bit polling, I2C 400kHz, remove throttling |
| Exp04 | ✅ Complete | SPI connection to dual BMI160 IMU sensors — chip ID detect, accel+gyro streaming |
| Exp05 | ⚠️ Partial | Automated flashing via serial bootloader command — serial command works, bootloader enters DFU mode, but MSC drive doesn't enumerate after GPREGRET reset (only after double-tap) |
| Exp06 | ✅ Complete | Zephyr RTOS v4.1 — Hello World via GitHub Actions CI, UF2 output, serial monitor verification |
| Exp07 | ✅ Complete | USB HID Mouse — synthetic rectangle motion (100×100) via Zephyr, CDC ACM + HID composite |
| Exp08 | ✅ Complete | ADS1015 joystick reading via I2C — port from PlatformIO to Zephyr RTOS |
| Exp09 | ✅ Complete | BMI160 gyro + accel over SPI — port from PlatformIO to Zephyr RTOS |
| Exp10 | ✅ Complete | Dual-gyro HID mouse via saturation-weighted crossfade — Alpakka-style fusion with burst averaging, startup calibration, deadzone, 30+ Kconfigs |
| Exp11 | ✅ Complete | TPS43 touchpad integration — I2C polling, 2-finger scroll (V+H), tap-to-click, configurable sensitivity, combined gyro+touchpad HID mouse |
| Exp12 | ❌ Failed | Tap-and-drag (drag lock) FSM for TPS43 — software FSM fights TPS43 hardware gesture engine, TPS43 native TAP_AND_HOLD should be used instead |
| Exp13 | ✅ Complete | Pure-software tap FSM (libinput port) — TPS43 gesture engine disabled, single tap, drag, right-click via 2-finger tap, drag lock optional, no double-click |
| Exp14 | ✅ Complete | Grace buffer + click-first FSM — DROP_GRACE_MS (20ms) filters transient contact-loss during drags; click-on-entry, double-click, and intentional drag-only FSM (FIRST_TAP + SECOND_TOUCH) |
| Exp15 | ✅ Complete | 1-finger edge scroll on TPS43 — 4 configurable edges (L/R/T/B) with per-edge axis, speed, invert; no FSM changes |
| Exp16 | ✅ Complete | 49E linear Hall effect sensors on ADS1015 — decoupling analysis, 128 SPS data rate, zero-offset calibration, replacing one joystick |
| Exp17 | ✅ Complete | Initial ZMK migration — my_shield 5×5 unibody, ZMK build-user-config CI, `main` branch, module structure |
