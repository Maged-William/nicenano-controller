# Exp06: Zephyr RTOS — Hello World on nice!nano

## Hypothesis

Zephyr RTOS v4.1 can be set up as a standalone build targeting the nice!nano nRF52840 board, producing a UF2 firmware that prints "Hello World" over USB serial in a loop. The build runs entirely in GitHub Actions, eliminating the need for a local toolchain.

## Execution Plan

1. **Source code** — Create a minimal Zephyr application at `zephyr-app/`:
   - `west.yml` — Zephyr manifest pointing to `zmkfirmware/zephyr` v4.1.0+zmk-fixes fork (includes nice_nano board)
   - `CMakeLists.txt` — App entry for Zephyr build system
   - `src/main.c` — Infinite loop with `printk("Hello World from Zephyr!\n")` every 3 seconds
   - `module/boards/nicekeyboards/nice_nano/` — Out-of-tree board definition for nice!nano (based on ZMK's definition)
     - `board.yml` — HWMv2 board metadata (vendor: nicekeyboards, SoC: nrf52840, revs: 1.0.0/2.0.0)
     - `Kconfig.nice_nano` — Selects SOC_NRF52840_QIAA
     - `nice_nano.dts` — Devicetree (USBD, I2C0, SPI1, partitions)
     - `nice_nano-pinctrl.dtsi` — Pin control: I2C=SDA:P0.17/SCL:P0.20, SPI=SCK:P0.06/MOSI:P0.08/MISO:P0.02
     - `nice_nano_2_0_0_defconfig` — Board defaults (MPU, pinctrl, GPIO, UF2 output)
     - `board.cmake` — Runner config for nrfjprog/UF2

2. **GitHub Actions CI** — Create `.github/workflows/build.yml`:
   - All west commands run in `zephyr-app/` directory
   - Install system deps (cmake, ninja, Python, etc.)
   - Download ARM GCC toolchain 12.2
   - `pip install west`
   - `west init -l . && west update` — fetch Zephyr + modules
   - `west build -b nice_nano . -- -DZEPHYR_EXTRA_MODULES=$PWD/module` — build with out-of-tree board
   - Convert `build/zephyr/zephyr.hex` → `zephyr.uf2` via `uf2conv.py`
   - Upload UF2 as build artifact

3. **Dev loop** — Write code → commit & push → wait for action → download artifact via `gh` CLI → flash via existing Leonardo automation

4. **Verification** — Connect Putty to the nice!nano's serial port, observe "Hello World" messages repeating every 3 seconds

## Success Criteria

- [x] GitHub Action builds successfully with no errors
- [x] UF2 artifact is produced and downloadable
- [x] Flashing to nice!nano results in visible "Hello World from Zephyr!" output on serial monitor at 3-second intervals

## Challenges

- **First build time** — `west update` downloads the full Zephyr source tree and all modules (~1-2GB); first CI run will be slow
- **ARM toolchain download** — 200MB+ tarball must be fetched each run unless caching is added
- **Board compatibility** — The nice!nano v2 board definition must match the Zephyr `nice_nano` target exactly (pinout, flash size, etc.)
- **USB console config** — The board must properly route `printk()` output over USB CDC ACM; may need extra Kconfig settings if not default

## Conclusion

**Hypothesis confirmed.** Zephyr RTOS v4.1 builds, flashes, and runs on the nice!nano, printing "Hello World" over USB serial at 3-second intervals — all via GitHub Actions CI with no local toolchain.

### What Worked

- ✅ **GitHub Actions CI** — Fully automated build pipeline produces UF2 artifacts in ~5 minutes (first build: 6m36s; subsequent with ARM GCC cache: ~5m)
- ✅ **Out-of-tree board definition** — Custom `nice_nano` board with correct pin mappings (I2C: P0.17/P0.20, SPI: P0.06/P0.08/P0.02) using HWMv2 format
- ✅ **UF2 output** — `CONFIG_BUILD_OUTPUT_UF2=y` produces valid UF2 at `build/zephyr/zephyr.uf2`
- ✅ **USB CDC ACM enumeration** — Three-part fix:
  1. `cdc_acm_uart0` as a child node of `&usbd` in the devicetree
  2. `zephyr,console` + `zephyr,shell-uart` chosen entries pointing to it
  3. `CONFIG_CONSOLE=y` + `CONFIG_UART_CONSOLE=y` in Kconfig
- ✅ **DTR gating** — Firmware waits for terminal to open the port before printing, preventing early-output loss
- ✅ **USB serial number** — `CONFIG_USB_DEVICE_SN="NICENANO-DEV-01"` provides a stable identifier to distinguish this board from other nice!nano devices
- ✅ **Leonardo flash automation** — `b` command triggers bootloader, UF2 copy to `G:\` completes within seconds

### What Didn't Work

- ❌ **Initial approach** — Using the bare `&usbd` without a CDC ACM child node. The USB device controller enabled, but no ACM interface was added to the USB descriptor, so no COM port appeared despite successful `usb_enable(NULL)`. This is the key learning: in Zephyr v4.1, CDC ACM is devicetree-driven.
- ❌ **Standalone `cdc_acm_uart0` root node** — Adding the node outside `&usbd` failed with a build-time static assertion (`"node is not assigned to a USB device controller"`). The node must be a *child* of the USB device controller.

### Build Times

| Run | Time | Notes |
|-----|------|-------|
| First build | 6m 36s | Cold cache — ARM GCC download + full `west update` |
| Subsequent (toolchain cached) | ~5m | Only source rebuild (~2m) + toolchain cache restore (~3m) |
| Incremental (source change only) | ~5m | Most time is setup overhead; compile itself is fast (~1m) |

### Final Project Structure

```
Dev/
├── .github/workflows/build.yml   # GitHub Actions CI
├── zephyr-app/                   # Zephyr application root
│   ├── west.yml                  # Manifest (upstream Zephyr v4.1.0)
│   ├── CMakeLists.txt            # App entry
│   ├── prj.conf                  # USB CDC ACM + console config
│   ├── src/main.c                # Hello world with DTR gating
│   └── module/                   # Out-of-tree board definition
│       ├── zephyr/module.yml
│       └── boards/nicekeyboards/nice_nano/
│           ├── board.yml
│           ├── Kconfig.nice_nano
│           ├── nice_nano.dts
│           ├── nice_nano-pinctrl.dtsi
│           ├── arduino_pro_micro_pins.dtsi
│           ├── nice_nano_2_0_0_defconfig
│           ├── nice_nano_1_0_0_defconfig
│           ├── pre_dt_board.cmake
│           └── board.cmake
└── builds/                       # Downloaded CI artifacts (gitignored)
    └── Exp06-*/
```
