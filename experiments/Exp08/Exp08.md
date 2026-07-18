# Exp08: ADS1015 Joystick Reading on Zephyr RTOS

## Hypothesis

The existing PlatformIO (Arduino framework) implementation for reading two joysticks via the ADS1015 12-bit ADC over I2C can be ported to Zephyr RTOS v4.1, reusing the existing board definition (I2C0 on P0.17/P0.20) and USB CDC ACM serial console.

## Execution Plan

1. **prj.conf** — Remove HID config, add I2C support. Keep CDC ACM for serial debug output.
2. **app.overlay** — Remove the HID device node (no longer needed).
3. **src/main.c** — Rewrite with Zephyr I2C API:
   - Initialize USB CDC ACM (DTR-gated, as in Exp06/Exp07)
   - Initialize I2C0 (already enabled in devicetree on P0.17/P0.20)
   - Scan for ADS1015 at address 0x48
   - Identify ADS1015 via bottom-4-bits test
   - Read all 4 single-ended channels (CH0=JoyA-X, CH1=JoyA-Y, CH2=JoyB-X, CH3=JoyB-Y) with ready-bit polling
   - Configure ±4.096V PGA, 1600 SPS, single-shot mode
   - Stream values over serial (printk) every ~100ms
   - Blink LED (P0.15) as heartbeat
4. **Build** — Via GitHub Actions CI (same pipeline as Exp06/Exp07)
5. **Flash** — Copy UF2 to NICENANO drive via Leonardo automation
6. **Verify** — Observe joystick readings in serial monitor (Putty, COM27 115200)

## Success Criteria

- [ ] Firmware builds with no errors on GitHub Actions, produces UF2 artifact
- [ ] ADS1015 detected at I2C address 0x48
- [ ] All 4 channels produce live readings that respond to joystick movement
- [ ] Readings are streamed over USB CDC ACM serial at 115200 baud
- [ ] LED blinks as heartbeat

## Challenges

- **Zephyr I2C API** — Must use `i2c_write()`/`i2c_write_read()` instead of Arduino `Wire` library
- **No built-in ADS1015 driver** — Raw register access required; custom I2C transaction code
- **Ready-bit polling** — Must convert the PlatformIO ready-bit check (config register bit 15) to Zephyr I2C reads
- **Timing** — Zephyr uses `k_sleep()`/`k_busy_wait()` instead of Arduino `delay()`
- **Serial output** — `printk()` instead of `Serial.print()`; no `Serial.flush()` equivalent needed

## Conclusion

(To be filled after experiment completion)
