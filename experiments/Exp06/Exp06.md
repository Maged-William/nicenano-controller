# Exp06: Zephyr RTOS — Hello World on nice!nano

## Hypothesis

Zephyr RTOS v4.1 can be set up as a standalone build targeting the nice!nano nRF52840 board, producing a UF2 firmware that prints "Hello World" over USB serial in a loop. The build runs entirely in GitHub Actions, eliminating the need for a local toolchain.

## Execution Plan

1. **Source code** — Create a minimal Zephyr application at the repo root:
   - `west.yml` — Zephyr manifest pointing to v4.1.0
   - `CMakeLists.txt` — App entry for Zephyr build system
   - `src/main.c` — Infinite loop with `printk("Hello World from Zephyr!\n")` every 3 seconds

2. **GitHub Actions CI** — Create `.github/workflows/build.yml`:
   - Install system deps (cmake, ninja, Python, etc.)
   - Download ARM GCC toolchain 12.2
   - `pip install west`
   - `west init -l . && west update` — fetch Zephyr v4.1 + modules
   - `west build -b nice_nano .` — build the app
   - Convert `zephyr.hex` → `zephyr.uf2` via `uf2conv.py`
   - Upload UF2 as a build artifact

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
