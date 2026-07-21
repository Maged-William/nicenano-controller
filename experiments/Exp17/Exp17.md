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

- [x] ZMK project inits + west updates successfully in CI
- [x] Firmware builds with no errors, produces UF2 artifact (390KB)
- [ ] nice!nano enumerates as a HID keyboard on boot *(not yet tested — no hardware wired)*
- [ ] All 25 keys register keypresses *(not yet tested — no matrix wired)*
- [x] CI build time recorded: 4m 9s (first successful build after west cache)
- [ ] Flashing via Leonardo `b` + copy UF2 works *(not yet tested)*

## Challenges

- **ZMK main branch** — tracking `main` means potential breakage from upstream changes
- **Board support** — the nice!nano Supermini clone must be properly defined in ZMK (standard `nice_nano` board target should work)
- **New dev loop** — flashing script needs updating (ZMK has no Zephyr shell, so `flash-nicenano.ps1` must use only Leonardo RST approach)

## Build Iterations

| Attempt | Time | Issue |
|---------|------|-------|
| 1 | 22s | Docker image pull failed (`ghcr.io/zmkfirmware/zmk-build-arm:stable`); switched to Docker Hub `zmkfirmware/zmk-build-arm:stable` |
| 2 | 22s | Still using manual Docker; switched to `zmkfirmware/zmk/.github/workflows/build-user-config.yml@main` |
| 3 | 35s | Missing `build.yaml` at repo root — `Fetch Build Keyboards` failed |
| 4 | 30s | `west zephyr-export` failed — missing `import: app/west.yml` (only had `west-commands`) |
| 5 | 3m10s | Invalid SHIELD — `config/boards/` deprecated; moved to `boards/` at repo root with `zephyr/module.yml` |
| 6 | 3m34s | Shield still not found — missing `Kconfig.shield`, `Kconfig.defconfig`, `my_shield.zmk.yml` |
| 7 | 3m36s | Shield found but keymap parse error — missing `#include <behaviors.dtsi>` and `<dt-bindings/zmk/keys.h>` |
| **8** | **4m 9s** | **Build successful** — UF2 artifact: `my_shield-nice_nano__zmk-zmk.uf2` (390KB) |

## Key Learnings

- ZMK `main` requires a **module structure** (`boards/shields/` at repo root + `zephyr/module.yml`), not `config/boards/`
- Every shield needs `Kconfig.shield` with `$(shields_list_contains,)` for shield discovery
- `Kconfig.defconfig` or `my_shield.conf` provides default configuration
- `my_shield.zmk.yml` metadata file is recommended but not strictly required
- Keymap files must `#include <behaviors.dtsi>` and `<dt-bindings/zmk/keys.h>` for keycode resolution
- Board variant `nice_nano//zmk` is correct for nice!nano V2 (maps from `nice_nano_v2` → `nice_nano@2.0.0//zmk` → `nice_nano//zmk`)
- The reusable workflow `zmkfirmware/zmk/.github/workflows/build-user-config.yml@main` auto-detects modules with `zephyr/module.yml`

## Conclusion

**Verdict: ✅ Complete** — CI builds successfully, UF2 artifact produced. Hardware testing pending matrix wiring.
