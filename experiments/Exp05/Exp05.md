# Exp05: Automated Flashing via Serial Bootloader

## Hypothesis

By adding a serial command listener to the firmware that writes `0x57` to `NRF_POWER->GPREGRET` and calls `NVIC_SystemReset()`, we can trigger the UF2 bootloader remotely — eliminating the physical double-tap reset. A host-side PowerShell script can then automate the entire build → flash cycle.

## Execution Plan

1. **Firmware** — Add `checkSerialCommand()` in `loop()` that buffers incoming serial bytes. On receiving `"BOOTLOADER\n"`, write magic value `0x57` to `NRF_POWER->GPREGRET` and call `NVIC_SystemReset()`. Send `"OK_RESET\n"` before resetting so the host knows the command was received.

2. **Host script**  — Create `flash.ps1` that:
   - Runs `build.ps1` to produce the UF2
   - Enumerates COM ports, opens the nice!nano's serial, sends `"BOOTLOADER\n"`
   - Polls for the `NICENANO` drive to appear
   - Copies `firmware.uf2` to the drive
   - Waits for the drive to disappear (firmware running)

3. **Build & test** — Verify firmware compiles, then test the full cycle.

## Success Criteria

- [ ] Sending `"BOOTLOADER\n"` via any serial terminal causes the board to disconnect and re-enumerate as the `NICENANO` mass storage drive
- [ ] The `flash.ps1` script autonomously builds, sends the command, waits for the drive, copies the UF2, and confirms the flash completed

## Challenges

- **Port detection** — COM port number changes per connection; must filter by USB VID/PID or device description
- **Drive detection race** — Must handle the timing gap between serial disconnect and mass storage mount
- **No serial feedback after reset** — Board disappears, so confirmation relies on drive appearing
- **Port in use** — If Putty has the port open, the script cannot connect

## Conclusion

### What Worked
- ✅ Serial command listener (`checkSerialCommand()`) reliably detects `"BOOTLOADER"` and triggers bootloader entry
- ✅ `enterUf2Dfu()` / `enterSerialDfu()` from the Arduino core properly write GPREGRET and reset the MCU
- ✅ The bootloader's CDC serial re-enumerates correctly at PID `239A:8029` (COM27) after the reset
- ✅ `flash.ps1` can auto-detect the serial port by VID/PID (`1D50:615E`) and send the command
- ✅ The build pipeline works and produces all three output formats (HEX, UF2, DFU zip)

### What Didn't Work
- ❌ The **MSC mass storage drive does not appear** after a GPREGRET-based bootloader entry (it works with physical double-tap reset). Only the CDC serial interface enumerates (MI_00), never the mass storage (MI_01). This prevents UF2 drag-and-drop flashing.
- ❌ Attempted fixes for MSC — `NVIC_SystemReset()`, WDT timeout reset, `NRF_USBD->USBPULLUP = 0` with 1-second delay, `enterUf2Dfu()` — none triggered MSC enumeration
- ❌ **Serial DFU** via `enterSerialDfu()` + `adafruit-nrfutil dfu serial` timed out — the bootloader's CDC serial appears to be debug-output-only, not a DFU command interface
- ❌ `adafruit-nrfutil` reports "Target is not in DFU mode" after the reset

### Root Cause
The nice!nano's bootloader (Adafruit nRF52 Bootloader variant) appears to **disable the mass storage interface** when entered via GPREGRET, enabling it only on a hardware pin reset (double-tap). This is a bootloader-level behavior that cannot be overridden from application firmware.

### Practical Outcome
The serial command mechanism works reliably for controlled bootloader entry, but the flash step still requires manual intervention (double-tap reset). The experiment is **partially successful** — the software path is complete, but the USB enumeration limitation prevents full automation.

### Future Directions
1. **Hardware VBUS switch** — Add a MOSFET or load switch to physically disconnect/reconnect USB VBUS after the serial command, forcing a full re-enumeration
2. **Bootloader replacement** — Flash a custom bootloader (e.g., UF2 bootloader from Adafruit with MSC always enabled) that properly supports GPREGRET-based entry with MSC
3. **BLE DFU** — Use the nRF52840's built-in BLE DFU capability for over-the-air updates
4. **Raw USB CDC protocol** — Implement a custom DFU-like protocol over the existing serial connection for flashing the application
