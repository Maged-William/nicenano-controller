# Exp03: Read Speed Optimization

## Hypothesis
The firmware's print rate is artificially throttled by a fixed `delay(100)` in `loop()` and a conservative `delay(10)` per channel in `readChannel()`. By removing the loop delay and replacing the per-channel fixed delay with ready-bit polling (checking bit 15 of the config register), the read rate can be increased from ~7 reads/sec to ~125+ reads/sec without any risk of reading stale conversion data.

## Execution Plan
1. Copy Exp02 firmware to root `firmware/` directory
2. Optimize `firmware/src/main.cpp`:
   - Add `Wire.setClock(400000)` after `Wire.begin()` — boost I2C from 100kHz to 400kHz
   - Replace `delay(10)` in `readChannel()` with a ready-bit poll loop on config register bit 15
   - Remove `delay(100)` at end of `loop()`
3. Update `AGENTS.md` build path to point at `firmware/`
4. Build via PlatformIO on Windows → produce UF2
5. Flash to nice!nano
6. Open Serial monitor at 115200 baud and verify significantly faster streaming

## Success Criteria
- [x] Firmware builds without errors
- [x] Serial output rate is visibly faster than Exp02 (~7 reads/sec → ~125+ reads/sec)
- [x] ADC still correctly identified as ADS1015
- [x] All 4 joystick channels produce live readings that respond to movement
- [x] No stale or corrupted readings from ready-bit polling

## Challenges
- Ready-bit polling must not hang indefinitely if the ADC fails to complete a conversion
- The I2C bus runs at 400kHz now — must ensure wiring integrity (short wires, clean connections)
- The LED blink in `loop()` now runs at ~125 Hz — may appear constantly on to the eye (persistence of vision)

## Conclusion

**Experiment successful.** The read speed was increased from ~7 reads/sec to an estimated ~300+ reads/sec by:

1. **Ready-bit polling** — replaced the fixed `delay(10)` per channel with a poll on config register bit 15, eliminating wasted wait time (ADS1015 converts in ~625µs at 1600 SPS, not 10ms)
2. **I2C 400kHz** — doubled the I2C bus speed from 100kHz, reducing transaction overhead
3. **Removed loop throttle** — the artificial `delay(100)` at end of `loop()` was the biggest bottleneck

All 4 joystick channels stream live data correctly. The LED blinks too fast to perceive visually (stays constantly lit). The UF2 was built and flashed to the nice!nano.
