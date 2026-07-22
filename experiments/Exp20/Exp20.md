# Exp20: Port TPS43 Touchpad to ZMK as `zmk-tps43-input` Module

## Hypothesis

The Azoteq TPS43 capacitive touchpad driver (I2C, addr 0x74) with its custom gesture pipeline (soft-tap FSM from Exp13/Exp14, 1-finger edge scroll from Exp15) can be ported from the legacy `zephyr-app/` Zephyr RTOS app to a standalone ZMK input driver module (`zmk-tps43-input/`), following the same pattern as the existing `zmk-ads1015-input/` module.

## Execution Plan

1. **Branch**: `Exp20` (forked from `Exp19`)
2. **New module**: `zmk-tps43-input/` at repo root:
   - `dts/bindings/zmk,tps43-input.yaml` — I2C device devicetree binding
   - `Kconfig` — 50+ config options (sensitivity, tap-FSM tuning, edge scroll per-edge settings)
   - `CMakeLists.txt` — build sources
   - `src/tps43_input.c` — ZMK input driver: I2C init, gesture disable, timer-based work-queue polling, input event emission
   - `src/tps43_regs.h` — register map (0x0D–0x1C, 0x0600–0x06BD)
   - `src/tps43_tapdrag.c` / `.h` — 8-state soft-tap FSM (single tap, 2-finger right-click, double-click, tap-and-drag with grace buffer, drag lock)
   - `src/tps43_edgescroll.c` / `.h` — edge scroll engine (4 configurable zones: L/R/T/B, per-edge axis/speed/invert)
3. **Modify**: `zephyr/module.yml` — add cmake/kconfig/dts_root for the new module
4. **Modify**: `boards/shields/my_shield/my_shield.overlay` — add TPS43 I2C node + input-listener
5. **Modify**: `boards/shields/my_shield/my_shield.conf` — enable touchpad, configure sensitivity, tap-FSM, edge scroll
6. **Build**: Via GitHub Actions CI
7. **Flash & Verify**: Leonardo → UF2 copy → USB logging

## Wiring

| nice!nano Pin | TPS43 Pin | Description |
|--------------|-----------|-------------|
| P0.17 | SDA | I2C Data |
| P0.20 | SCL | I2C Clock |
| VCC | VCC | 3.3V |
| GND | GND | Ground |

Shares the I2C bus with the ADS1015 (address 0x48). TPS43 at address 0x74.

## Success Criteria

- [ ] TPS43 detected at 0x74 on boot
- [ ] Gesture engine disabled (`SFGestureEnable=0` via reg 0x06B7) — no hardware gestures interfering with FSM
- [ ] Single-finger movement → mouse cursor
- [ ] Tap → left click
- [ ] 2-finger tap → right click
- [ ] Double-click works
- [ ] Tap-and-drag works (with grace buffer for transient contact loss)
- [ ] 1-finger edge scroll (4 configurable edges) works
- [ ] Keyboard keys still function as keyboard
- [ ] BLE advertising works
- [ ] No interference with ADS1015 joystick HID gamepad (both on same I2C bus)

## Challenges

- TPS43 shares I2C bus with ADS1015 — ensure both polling timers coexist without starvation
- FSM debug visibility via USB logging (need log level control)
- Late-init probe when touchpad is not ready at boot (same pattern as Exp11)

## Build Iterations

| Attempt | Issue | Fix |
|---------|-------|-----|

## Known-Good Commit

TBD

## Conclusion

TBD
