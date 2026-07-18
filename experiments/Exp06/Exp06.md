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

- [ ] GitHub Action builds successfully with no errors
- [ ] UF2 artifact is produced and downloadable
- [ ] Flashing to nice!nano results in visible "Hello World from Zephyr!" output on serial monitor at 3-second intervals

## Challenges

- **First build time** — `west update` downloads the full Zephyr source tree and all modules (~1-2GB); first CI run will be slow
- **ARM toolchain download** — 200MB+ tarball must be fetched each run unless caching is added
- **Board compatibility** — The nice!nano v2 board definition must match the Zephyr `nice_nano` target exactly (pinout, flash size, etc.)
- **USB console config** — The board must properly route `printk()` output over USB CDC ACM; may need extra Kconfig settings if not default

## Conclusion

*(to be filled after execution)*
