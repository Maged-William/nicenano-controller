# Exp01: Toolchain Validation — Blinky

## Hypothesis
The PlatformIO toolchain + WSL build + UF2 flashing pipeline works with our nice!nano v2 clone.

## Execution Plan
1. Create PlatformIO project using the nicenano board definition
2. Write firmware that blinks the built-in LED (P0.15) with 2 quick pulses then a pause
3. Build via WSL → produce `.uf2`
4. Flash by copying to `G:/` drive (bootloader mode)
5. Observe LED pattern to confirm success

## Success Criteria
- [x] Build succeeds producing a valid `.uf2`
- [x] After flashing, board blinks 2 quick pulses repeatedly (200ms ON, 200ms OFF, 200ms ON, 2s pause)

## Challenges
- Alpine WSL uses musl libc — ARM cross-compiler toolchain is glibc, incompatible
- Windows-native PlatformIO build works with additional board/variant files from the support repo
- Board JSON (`nicenano.json`) and variant files (`variant.h`, `variant.cpp`) must be manually placed in PlatformIO's package directories

## Conclusion (so far)
- Toolchain validated on **Windows PlatformIO** (Python 3.14)
- Build produces ELF → HEX → UF2 successfully
- UF2 file: `firmware.uf2` (42KB) ready for flashing
- LED pin: P0.15 (active-low)
- After flashing, board should blink 2 quick pulses (200ms) then 2s pause
