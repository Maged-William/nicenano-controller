# Exp17: Initial ZMK Migration — Basic 5×5 Shield

## Hypothesis

A ZMK-based 5×5 unibody shield (`my_shield`) with GPIO-matrix scanning can replace the manual Zephyr RTOS application as the primary build target, using ZMK's official GitHub Actions workflow and latest `main` branch.

## Execution Plan

1. **Branch**: `Exp17` (forked from `Exp16` state)
2. **Repo structure**:
   - `config/west.yml` — ZMK manifest pointing to `zmkfirmware/zmk` at `main`
   - `config/boards/shields/my_shield/` — shield definition
3. **Shield files**:
   - `my_shield.overlay` — 5×5 GPIO-matrix (col2row), kscan0, default matrix transform
   - `my_shield.conf` — `CONFIG_ZMK_KEYBOARD_NAME`
   - `my_shield.keymap` — plain 5×5 grid bindings (alpha-numeric placeholders)
4. **CI**: Replace `.github/workflows/build.yml` with ZMK Docker build (`ghcr.io/zmkfirmware/zmk-build-arm:stable`); keep `zephyr-app/` for reference (no longer CI-built)
5. **No extras** — no OLED, encoder, RGB, or sensor integration

## Matrix Pin Assignment

| Col | Pin | Row | Pin |
|-----|-----|-----|-----|
| C0  | P0.24 | R0 | P0.09 |
| C1  | P1.00 | R1 | P0.10 |
| C2  | P0.11 | R2 | P1.11 |
| C3  | P1.04 | R3 | P1.13 |
| C4  | P1.06 | R4 | P1.15 |

Diode direction: **COL2ROW**

## Success Criteria

- [ ] ZMK project inits + west updates successfully in CI
- [ ] Firmware builds with no errors, produces UF2 artifact
- [ ] nice!nano enumerates as a HID keyboard on boot
- [ ] All 25 keys register keypresses (tested via serial HID report or USB)
- [ ] CI build time recorded
- [ ] Flashing via Leonardo `b` + copy UF2 works

## Challenges

- **ZMK main branch** — tracking `main` means potential breakage from upstream changes
- **Board support** — the nice!nano Supermini clone must be properly defined in ZMK (standard `nice_nano` board target should work)
- **New dev loop** — flashing script needs updating (ZMK has no Zephyr shell, so `flash-nicenano.ps1` must use only Leonardo RST approach)

## Conclusion

*To be filled after experiment execution.*
