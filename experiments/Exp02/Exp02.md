# Exp02: ADC Identification & Joystick Reading

## Hypothesis
The ADC module (ADS1015 or ADS1115) can be identified by checking the bottom 4 bits of the conversion register — the ADS1015 stores a 12-bit value left-aligned (bits 3–0 always 0), while the ADS1115 uses all 16 bits. Once identified, live joystick X/Y readings from both joysticks can be streamed over Serial.

## Execution Plan
1. Create PlatformIO project (`experiments/Exp02/firmware/`)
2. Write firmware that:
   - Initializes I2C (Wire) on P0.17 (SDA) / P0.20 (SCL)
   - Scans I2C bus for device(s) — expect `0x48` (ADDR floating)
   - Reads the conversion register 20× and checks if bottom 4 bits are always 0
     - Always 0 → ADS1015 (12-bit)
     - Any variation → ADS1115 (16-bit)
   - Continuously reads all 4 single-ended channels (A0–A3) at ±4.096V range
   - Prints raw values over Serial (115200 baud)
   - Blinks LED (P0.15) on each read cycle as a heartbeat
3. Build via Windows PlatformIO → produce UF2
4. Flash by copying to `G:/` drive
5. Open Serial monitor to confirm identification and observe joystick readings

## Success Criteria
- [x] I2C scan detects device at `0x48`
- [x] ADC is identified as **ADS1015** (12-bit) with confidence
- [x] All 4 channels produce varying live readings when joysticks are moved
- [x] Build + flash pipeline works end-to-end (reuses Exp01 tooling)

## Challenges
- ADS1015 and ADS1115 share the same register map, I2C address, and protocol — no dedicated device ID register
- Both chips return identical numeric values for the same input voltage (ADS1015 left-shifts its 12-bit result by 4 into the 16-bit register)
- Identification relies on checking the bottom-4-bit pattern across multiple samples

## Conclusion

**ADC Identified**: ADS1015 (12-bit) — confirmed by bottom-4-bits test across 10 samples (sum = 0).

**Key Findings**:
- PlatformIO + Arduino framework for nicenano nRF52840 works end-to-end
- Default Wire pins in the nicenano variant are P1.04 (SDA) / P1.06 (SCL); must call `Wire.setPins(17, 20)` for P0.17/P0.20
- ADS1015 identification via bottom-4-bits is reliable when reading a non-zero channel (CH3 at ~1.65V/13520 counts showed bottom bits always 0)
- Live streaming of all 4 single-ended channels (A0–A3) at ±4.096V PGA works with single-shot mode
- CH0/CH1 flickering was a breadboard contact issue, not a firmware/ADC problem
- I2C address: 0x48 (ADDR floating)
- The `Wire.setPins()` API must be called *before* `Wire.begin()` on the Adafruit nRF52 Arduino core

**Pipeline**: Build → UF2 conversion → copy to G:/ → reboot → Serial at 115200 baud.
