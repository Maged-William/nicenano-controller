# Exp09: BMI160 Gyro + Accel on Zephyr RTOS

## Hypothesis

The existing PlatformIO (Arduino framework) code for reading two BMI160 6-DoF IMU sensors over SPI can be ported to Zephyr RTOS v4.1, reusing the existing board definition (SPI1 on P0.06/P0.08/P0.02, CS on P0.31/P0.29) and USB CDC ACM serial console. The port should merge with the existing ADS1015 ADC reading from Exp08 for a combined sensor readout.

## Execution Plan

1. **prj.conf** — Add `CONFIG_SPI=y`. Keep existing I2C and CDC ACM config.
2. **app.overlay** — Update comment (no functional change — SPI1 already enabled in board DTS, CS pins are plain GPIOs).
3. **src/main.c** — Port the raw SPI register-access pattern from the PlatformIO code:
   - Add Zephyr SPI API includes (`<zephyr/drivers/spi.h>`)
   - Port `spiWriteReg`/`spiReadReg` to use `spi_write()`/`spi_transceive()` with manual GPIO CS toggling
   - Port `initBMI160` with `k_sleep()` instead of `delay()`
   - Port burst read of 12 bytes (gyro + accel X/Y/Z) starting at register 0x0C
   - Keep existing ADS1015 I2C ADC code from Exp08
   - In main loop: read all 4 ADC channels + both BMI160 sensors → print 4×ADC + 12×IMU = 16 values per line
4. **Build** — Via GitHub Actions CI (same pipeline as Exp06–Exp08)
5. **Flash** — Copy UF2 to NICENANO drive via Leonardo automation
6. **Verify** — Observe combined ADC + dual IMU readings in serial monitor (Putty, COM27 115200)

## Success Criteria

- [ ] Firmware builds with no errors on GitHub Actions, produces UF2 artifact
- [ ] SPI1 initializes correctly (SCK=P0.06, MOSI=P0.08, MISO=P0.02)
- [ ] Both BMI160s initialize successfully (chip ID 0xD1)
- [ ] Accel (X/Y/Z) and gyro (X/Y/Z) values stream from both sensors
- [ ] ADS1015 joystick channels continue to stream alongside IMU data
- [ ] Values change with physical board and joystick movement

## Challenges

- **Zephyr SPI API** — Must use `spi_write()`/`spi_transceive()` instead of Arduino `SPI.transfer()`
- **Manual CS control** — BMI160 uses separate CS per sensor; Zephyr SPI API expects CS via devicetree; must toggle CS manually via GPIO API
- **Burst read** — The 12-byte burst read at register 0x0C requires a 13-byte `spi_transceive()` (address byte + 12 dummy TX bytes), discarding the first RX byte
- **No built-in BMI160 driver** — Raw register access required, same as PlatformIO approach
- **Hardware** — Same SPI bus, two CS lines; must ensure CS toggling is clean to avoid bus contention

## Conclusion

*To be filled after experiment execution.*
