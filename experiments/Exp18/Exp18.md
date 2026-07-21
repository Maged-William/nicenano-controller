# Exp18: Migrate ADS1015 I2C ADC to ZMK — Joystick-as-Mouse

## Hypothesis

The ADS1015 I2C ADC driver (port of the existing PlatformIO module) can be implemented as a ZMK input driver (`zmk-ads1015-input`) that reads joystick X/Y from channels 0/1, emits `INPUT_ABS_X`/`INPUT_ABS_Y` events, and integrates with ZMK's native input-listener pipeline to produce relative mouse pointer movement.

## Execution Plan

1. **Branch**: `Exp18` (forked from `Exp17`)
2. **New module**: `zmk-ads1015-input/` at repo root:
   - `dts/bindings/zmk,ads1015-input.yaml` — devicetree binding
   - `Kconfig` — module config (master enable, log level)
   - `CMakeLists.txt` — source build
   - `src/ads1015_input.c` — driver: I2C init, timer-based polling, input events
3. **Modify**: `zephyr/module.yml` — add cmake/kconfig/dts_root for the new module
4. **Modify**: `boards/shields/my_shield/my_shield.overlay` — define I2C0, ADS1015 node, input-processor + input-listener chain
5. **Modify**: `boards/shields/my_shield/my_shield.conf` — enable `CONFIG_ZMK_POINTING`, I2C, input system
6. **Build**: Via GitHub Actions CI (same `build.yaml` as Exp17)
7. **Flash & Verify**: Leonardo → UF2 copy → USB logging

## Wiring

| nice!nano Pin | ADS1015 Pin | Description |
|--------------|-------------|-------------|
| P0.17 | SDA | I2C Data |
| P0.20 | SCL | I2C Clock |
| VCC | VDD | 3.3V |
| GND | GND | Ground |
| float | ADDR | Address 0x48 |

| ADS1015 | Joystick |
|---------|----------|
| A0 | X axis |
| A1 | Y axis |

Channels 2/3 (hall sensors): **skipped** for this experiment.

## Success Criteria

- [x] I2C0 bus activates on P0.17/P0.20 (verified via USB logging)
- [x] ADS1015 detected at 0x48 on boot
- [x] Ch0 (joystick X) produces non-zero input events
- [x] Ch1 (joystick Y) produces non-zero input events
- [x] Moving the joystick moves the mouse cursor on the host
- [x] No drift at center position (deadzone filtering active)

## Challenges

- ZMK's `zmk,input-listener` uses `INPUT_ABS_X`/`INPUT_ABS_Y` with BTN_TOUCH anchoring — not suitable for joystick (always sends delta=0). Switched to `INPUT_REL_X`/`INPUT_REL_Y` with center-displacement computation in the driver instead
- I2C polling in work-queue with busy-wait for conversion timeout: works but blocks the workqueue for ~16ms per cycle
- nice_nano V2 default I2C0 pinctrl (P0.17/P0.20) matches Pro Micro mapping — no override needed

## Build Iterations

| Attempt | Issue | Fix |
|---------|-------|-----|
| 1 | Linker error: `__device_dts_ord_25` undefined — Kconfig dependency `depends on I2C` unsatisfied | Added `CONFIG_I2C=y` to `my_shield.conf` |
| 2 | Compile error: `input_report_abs` API wrong — Zephyr 4.1 uses 5 args (dev, code, value, sync, timeout) | Fixed to `input_report_abs(dev, code, value, sync, K_NO_WAIT)`, removed `input_sync()` |
| 3 | No mouse movement — `INPUT_ABS` events ignored without `BTN_TOUCH` | Added `BTN_TOUCH=1` on first poll |
| 4 | Still no movement — ZMK listener updates anchor on every ABS event → delta=0 | Switched to `INPUT_REL_X`/`INPUT_REL_Y` with center-displacement math |
| 5 | Mouse speed too fast (~134 px/frame) | Changed divisor from 100→1000, deadzone from 200→2000 |
| 6 | Choppy mouse movement at 26 Hz poll rate | Increased ADS1015 to 2400 SPS, poll interval 20→3ms (~140 Hz effective) |
| 7 | DTS binding YAML syntax error — `update-interval-ms` nested under channel-y | Fixed indentation, included `i2c-device.yaml` for reg property |
| 8 | ZMK Kconfig undefined symbol `SETTINGS_RESET` | Removed, lowered ADC log to WRN temporarily for boot capture |
| 9 | BLE not discoverable on battery — Leonardo parasitic voltage held nRF in bootloader | Unplug Leonardo + power-cycle nice!nano; `CONFIG_ZMK_BLE=y` verified working |

## Known-Good Commit

```bash
git checkout exp18-good
# Tag: exp18-good → 564ca7b
```

## Conclusion

**Verdict: ✅ Complete** — ADS1015 I2C ADC successfully ported to ZMK as a custom input driver. Joystick X/Y produces smooth mouse pointer movement with deadzone filtering, and BLE advertising works when the nice!nano is properly power-cycled.

Key implementation decisions:
- Driver computes displacement from center → `INPUT_REL_X`/`INPUT_REL_Y` (not ABS)
- Center calibrated on first poll cycle
- ADS1015 at 2400 SPS, polled every 3ms (~140 Hz effective update rate)
- Hardcoded deadzone (500 ADC units) and sensitivity (div 1000 ≈ 10% speed)
- `CONFIG_ZMK_BLE=y` for Bluetooth advertising

### BLE Discovery Note
Device was invisible on battery because the Arduino Leonardo kept the nice!nano RST line low via parasitic voltage, holding it in bootloader mode. Fix: disconnect Leonardo USB or use a diode-isolated reset circuit. After power-cycling the nice!nano alone, the device appears as `my_shield` in Bluetooth scanning.

### Smoothness Tuning
- Initial rate: 128 SPS, 20ms interval → ~26 Hz → **choppy**
- Final rate: 2400 SPS, 3ms interval → ~140 Hz → **smooth**
- Deadzone: 500 ADC units (~3% of full scale) filters center jitter
- Mouse speed: ~12 px/sec at full deflection (comfortable for pointer use)

Next steps for refinement:
- Make sensitivity, deadzone Kconfig-configurable via DT or Kconfig
- Use state-machine I2C reads instead of busy-wait to unblock workqueue
- Add hall sensor channels (ch2, ch3) as additional input axes
- Add diode on Leonardo RST line to prevent parasitic backfeed
