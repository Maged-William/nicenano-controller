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

## Known-Good Commit

```bash
git checkout exp18-good
# Tag: exp18-good → 6cf8580
```

## Conclusion

**Verdict: ✅ Complete** — ADS1015 I2C ADC successfully ported to ZMK as a custom input driver. Joystick X/Y produces mouse pointer movement at a usable speed with deadzone filtering.

Key implementation decisions:
- Driver computes displacement from center → `INPUT_REL_X`/`INPUT_REL_Y` (not ABS)
- Center calibrated on first poll cycle
- Hardcoded deadzone (2000 ADC units ≈ 125 LSB) and sensitivity (div 1000)
- Polling interval: 20ms (effective ~38ms with I2C read delays ≈ 26 Hz)

Next steps for refinement:
- Make sensitivity, deadzone Kconfig-configurable
- Use state-machine I2C reads instead of busy-wait
- Add hall sensor channels (ch2, ch3) as additional input axes
