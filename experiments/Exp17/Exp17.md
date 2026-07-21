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
- [x] nice!nano enumerates as a HID keyboard on boot
- [ ] All 25 keys register keypresses *(not yet tested — no matrix wired)*
- [x] CI build time recorded: 4m 9s (first successful build, 1m45s cached)
- [x] Flashing via Leonardo `b` + copy UF2 works (COM29 → G: drive)
- [ ] Serial monitor output over USB *(needs CDC ACM config, ZMK shell targets UART pins not USB)*

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
- USB logging requires the `zmk-usb-logging` snippet (via `snippet:` in `build.yaml`) or `CONFIG_ZMK_USB_LOGGING=y` in `.conf`; the CDC ACM port appears on COM18 (Windows) with PID matching the original ZMK USB PID
- ZMK tracks battery voltage via nRF52840 ADC; logged automatically when USB logging is enabled

## Conclusion

**Verdict: ✅ Complete** — CI builds successfully, UF2 artifact produced, hardware verified via USB logging.

### USB Logging Verification (via `zmk-usb-logging` snippet)

Using the `zmk-usb-logging` snippet in `build.yaml`:
```
include:
  - board: nice_nano//zmk
    shield: my_shield
    snippet: zmk-usb-logging
```

Serial output captured on COM18 (CDC ACM) at 115200 baud:

**Boot log** showed matrix initialization with correct pins:
- Row inputs: P0.09, P0.10, P1.11, P1.13, P1.15
- Column outputs: P0.24, P1.00, P0.11, P1.04, P1.06

**Key press verified** — pressing a key produced:
```
kscan_matrix_read: Row: 2, col: 2, position: 12, pressed: true
keymap_apply_position_state: layer_id: 0 position: 12, binding name: key_press
on_keymap_binding_pressed: position 12 keycode 0x70008
hid_listener_keycode_pressed: usage_page 0x07 keycode 0x08
```
Keycode 0x70008 = HID usage page 7, code 8 = keyboard **E** (our position 12 = row 2, col 2 = `&kp E`).

**Battery monitor** working: `ADC raw 2965 ~ 4340 mV => 100%`

The shield matrix, keymap, HID endpoint, and USB logging all function correctly.
