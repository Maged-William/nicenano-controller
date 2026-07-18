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

**Hypothesis confirmed.** The PlatformIO BMI160 SPI gyro+accel code was successfully ported to Zephyr RTOS v4.1, merged with the existing ADS1015 ADC reading from Exp08. All sensors stream data in a unified output.

### What Worked
- ✅ **Zephyr SPI API** — `spi_write()`/`spi_transceive()` with manual GPIO CS toggling works correctly
- ✅ **BMI160 initialization** — Both sensors initialize (chip ID 0xD1) and stream gyro+accel data
- ✅ **Burst read** — 13-byte `spi_transceive()` correctly reads 12 data bytes starting at register 0x0C
- ✅ **Merged output** — All 16 values (4 ADC + 6 IMU1 + 6 IMU2) stream in a single tab-separated line
- ✅ **ADS1015 preserved** — ADC joystick channels continue to work alongside IMU data
- ✅ **CI pipeline** — GitHub Actions builds cleanly in ~5m48s, produces UF2 artifact
- ✅ **Automated flashing** — Leonardo on COM29 triggers bootloader, UF2 copy via PowerShell

### Key Differences from PlatformIO
| Aspect | PlatformIO (Arduino) | Zephyr |
|--------|---------------------|--------|
| SPI init | `SPI.setPins(miso, sck, mosi); SPI.begin()` | Devicetree + `device_get_binding()` |
| Write register | `SPI.transfer(reg & 0x7F); SPI.transfer(val)` | `spi_write()` with 2-byte buffer |
| Read register | `SPI.transfer(reg \| 0x80); val = SPI.transfer(0x00)` | `spi_transceive()` with 2-byte TX/RX |
| Burst read | `SPI.transfer(0x8C); for loop SPI.transfer(0x00)` | 13-byte `spi_transceive()` |
| CS control | `digitalWrite(cs, LOW/HIGH)` | `gpio_pin_set(gpio, cs, 0/1)` |
| Timing | `delay()` | `k_sleep()` |
| Serial output | `Serial.print()` | `printk()` |

### Build Time
| Run | Time | Notes |
|-----|------|-------|
| First build | 5m 43s | With two -Wreturn-type warnings (fixed) |
| Second build | 5m 48s | Clean build after warning fix |

### Data Output Format
```
CH0\tCH1\tCH2\tCH3\tS1_gX\tS1_gY\tS1_gZ\tS1_aX\tS1_aY\tS1_aZ\tS2_gX\tS2_gY\tS2_gZ\tS2_aX\tS2_aY\tS2_aZ
```
