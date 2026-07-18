# Exp07: USB HID Mouse — Synthetic Rectangle Motion

## Hypothesis

The nice!nano nRF52840 can enumerate as a USB HID mouse (via Zephyr RTOS v4.1) and drive the host cursor in a synthetic rectangular pattern — right 100px → down 100px → left 100px → up 100px — repeating indefinitely. Serial debug output over CDC ACM confirms position tracking.

## Execution Plan

1. **Devicetree overlay** (`app.overlay`) — Add `zephyr,hid-device` node as child of `&usbd` with `protocol-code = "mouse"`, 4-byte report size, 1ms polling
2. **prj.conf** — Add `CONFIG_USB_HID=y`, keep existing CDC ACM for composite USB (serial debug + HID mouse)
3. **`src/main.c`** — Rewrite:
   - Initialize HID device with standard mouse report descriptor (buttons + relative X/Y + wheel)
   - Register and init HID via `usb_hid_register_device()` / `usb_hid_init()`
   - `usb_enable(NULL)` for composite USB
   - Wait for DTR (serial terminal)
   - Loop: right(100) → down(100) → left(100) → up(100) @ 10ms per step
   - Print `X:pos Y:pos` over serial each step
   - Toggle LED P0.15 each full loop
4. **CI** — Push to GitHub Actions; build produces UF2 artifact

## Success Criteria

- [ ] Firmware builds with no errors on GitHub Actions
- [ ] nice!nano enumerates as a HID mouse device on the host
- [ ] Cursor moves in a 100×100 pixel rectangle, repeating
- [ ] Serial output shows position tracking
- [ ] UF2 artifact downloadable from CI

## Challenges

- **Composite USB (CDC ACM + HID)** — Both interfaces must coexist on one USB device; Zephyr must compose them correctly
- **Devicetree binding** — `zephyr,hid-device` requires specific properties (`in-report-size`, `in-polling-period-us`) that must match the report descriptor
- **Enumeration timing** — HID reports sent before USB enumeration completes may be lost; need proper startup delay
- **Relative vs absolute positioning** — The cursor starts from wherever it currently is; rectangle will drift if any steps are dropped

## Conclusion

**Hypothesis confirmed.** The nice!nano nRF52840 successfully enumerates as a composite USB device (CDC ACM + HID) under Zephyr RTOS v4.1 and drives the host cursor in a synthetic 100×100 pixel rectangle, repeating indefinitely.

### What Worked

- ✅ **Composite USB device** — CDC ACM (serial debug via `printk`) and HID (mouse) coexist on a single USB device with two interfaces
- ✅ **HID report descriptor** — Standard 4-byte mouse report (buttons + relative X/Y + wheel) correctly interpreted by the host OS
- ✅ **Rectangle motion** — Firmware sends `+1,0` × 100 (right), `0,+1` × 100 (down), `-1,0` × 100 (left), `0,-1` × 100 (up) at 10ms/pixel, completing one full loop in ~4 seconds
- ✅ **Serial debug** — Position coordinates printed over CDC ACM each step confirm correct movement
- ✅ **CI pipeline** — GitHub Actions builds successfully in ~5m34s, produces UF2 artifact
- ✅ **Automated flashing** — Leonardo on COM29 triggers bootloader, UF2 copies to NICENANO drive

### What Didn't Work

- ❌ **`DEVICE_DT_GET(DT_NODELABEL(hid0))`** — The HID driver registers under a string name ("HID_0"), not via `DEVICE_DT_DEFINE`. Must use `device_get_binding("HID_0")` instead.
- ❌ **`CONFIG_USB_HID`** — The correct Kconfig symbol is `CONFIG_USB_DEVICE_HID` (not `USB_HID`), and `CONFIG_USB_DEVICE_INITIALIZE_AT_BOOT=n` must be set since `usb_enable(NULL)` is called manually after HID registration.

### Key Learnings

- The `app.overlay` devicetree overlay is the correct place to add the HID device node under `&usbd`
- `in-report-size` and `in-polling-period-us` are required properties in the `zephyr,hid-device` binding
- When both CDC ACM and HID are enabled, Zephyr automatically composes them into a single composite USB device
- DTR gating still works with composite devices — the serial port appears once the terminal connects

### Build Time

| Run | Time | Notes |
|-----|------|-------|
| First build (no cache) | 5m 02s | ARM GCC toolchain cached from Exp06; full west update |
| Second build (fix Kconfig) | 5m 43s | Full rebuild after config change |
| Third build (fix binding) | 5m 34s | Incremental compilation of app only |
