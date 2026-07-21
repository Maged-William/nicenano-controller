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

- [ ] I2C0 bus activates on P0.17/P0.20 (verified via USB logging)
- [ ] ADS1015 detected at 0x48 on boot
- [ ] Ch0 (joystick X) produces non-zero `INPUT_ABS_X` events
- [ ] Ch1 (joystick Y) produces non-zero `INPUT_ABS_Y` events
- [ ] Moving the joystick moves the mouse cursor on the host
- [ ] No drift at center position (or deadzone filtering active)

## Challenges

- ZMK's `zmk,input-listener` conversion semantics for `INPUT_ABS` events from a joystick (absolute displacement) vs. a touchpad (absolute position) — may need custom input-processor for centering and deadzone
- I2C polling in a work-queue context: using `k_msleep()` inside a work handler is acceptable for initial experiment but should be optimized later
- The nice_nano V2 default I2C0 pinctrl must match P0.17/P0.20 (should be standard Pro Micro mapping)
