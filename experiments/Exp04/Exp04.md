# Exp04: SPI Connection to Dual BMI160 IMUs

## Hypothesis
Two BMI160 6-DoF IMU sensors can be driven over SPI on the same bus (shared SCK/MOSI/MISO) using the hanyazou BMI160-Arduino library with separate chip-select pins (P0.31, P0.29).

## Execution Plan
1. Copy hanyazou BMI160-Arduino library sources into `firmware/lib/BMI160/`
2. Write firmware `main.cpp` that:
   - Initializes SPI on the nicenano nRF52840 at 400kHz
   - Creates two BMI160GenClass instances, one per CS pin
   - Verifies each sensor by chip ID (0xD1)
   - Reads accel (X/Y/Z) and gyro (X/Y/Z) data from both sensors in loop
   - Streams all 12 values over Serial at 115200 baud
3. Build via PlatformIO on Windows → produce UF2
4. Flash by copying to `G:/` drive
5. Open Serial monitor to confirm both sensors streaming live data

## SPI Wiring
| Function | nice!nano Pin |
|----------|--------------|
| SCK      | P0.06        |
| MOSI     | P0.08        |
| MISO     | P0.02        |
| CS1 (Sensor 1) | P0.31 |
| CS2 (Sensor 2) | P0.29 |

## Success Criteria
- [x] Both BMI160s respond with chip ID `0xD1`
- [x] Accel (X/Y/Z) and gyro (X/Y/Z) values stream from both sensors over Serial
- [x] Values change with physical board movement
- [x] All wiring matches the pin table

## Challenges
- Library was designed for Arduino UNO/101 — SPI timing on nRF52 may differ
- Both sensors share the SPI bus; CS toggling must be clean to avoid bus conflicts
- May need `SPI.beginTransaction()`/`endTransaction()` for proper SPI mode config
- The default SPI pins on the nicenano variant may not match our wiring
- `Serial.flush()` blocks forever on nRF52 TinyUSB if CDC port not yet opened by host
- `BMI160Class::initialize()` has infinite while loops on PMU status — hangs if SPI fails
- P0.29 and P0.31 are exposed NFC antenna pins but work as GPIO for CS

## Conclusion
**Experiment successful.** Both BMI160 IMU sensors communicate over SPI and stream 6-axis (accel + gyro) data.

### Key Findings:
1. **SPI pins**: Must call `SPI.setPins(miso, sck, mosi)` with `SPI.setPins(2, 6, 8)` for P0.02/P0.06/P0.08
2. **Chip IDs**: Both sensors returned `0xD1` confirming detection
3. **Sensor2 power issue**: One sensor had a loose power wire — fixed by reseating connections
4. **Data correct**: Accel Z ≈ -16000 counts (-1g), gyro near 0 when stationary, values respond to movement
5. **Byte order**: BMI160 uses little-endian — LSB at lower address
6. **SPI mode**: Mode 0 (CPOL=0, CPHA=0) at 1MHz works reliably
7. **CS pins**: P0.31 and P0.29 work as chip-select outputs despite being NFC-capable pins
8. **Raw SPI preferred**: Direct register SPI reads are simpler and more robust than the BMI160-Arduino library's `initialize()` which has no timeout on PMU status waits
