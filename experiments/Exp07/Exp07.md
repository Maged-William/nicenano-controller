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
