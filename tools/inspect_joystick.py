import hid
import time
import struct
import sys

REPORT_ID = 0x02  # zmk-hid-io joystick report ID

def find_joystick():
    for dev in hid.enumerate():
        # ZMK USB VID/PID: 1d50:615e
        if dev["vendor_id"] == 0x1d50 and dev["product_id"] == 0x615e:
            if dev["usage_page"] == 1 and dev["usage"] == 4:  # Generic Desktop / Joystick
                return dev
        # Also try matching by interface number (HID_1 = interface 1)
    # Fallback: any joystick usage
    for dev in hid.enumerate():
        if dev["usage_page"] == 1 and dev["usage"] == 4:
            return dev
    return None

info = find_joystick()
if not info:
    print("No joystick HID device found.")
    print("Check that nano-joy is connected and visible in joy.cpl")
    sys.exit(1)

print(f"Device: {info.get('manufacturer_string', '?')} {info.get('product_string', '?')}")
print(f"VID:PID = {info['vendor_id']:04x}:{info['product_id']:04x}")
print(f"Path: {info['path']}")
print()

try:
    dev = hid.device()
    dev.open_path(info["path"])
    print(f"Report size: {info.get('report_descriptor_size', '?')} bytes")
    print(f"Opened. Reading raw reports (Ctrl+C to quit)...\n")

    dev.set_nonblocking(True)

    while True:
        raw = dev.read(64, timeout_ms=200)
        if not raw:
            continue
        report = bytes(raw) if isinstance(raw, list) else raw
        if len(report) > 0 and report[0] == REPORT_ID:
            body = report[1:]
            if len(body) >= 7:
                d_x = struct.unpack_from("<b", body, 0)[0]
                d_y = struct.unpack_from("<b", body, 1)[0]
                d_z = struct.unpack_from("<b", body, 2)[0]
                d_rx = struct.unpack_from("<b", body, 3)[0]
                d_ry = struct.unpack_from("<b", body, 4)[0]
                d_rz = struct.unpack_from("<b", body, 5)[0]
                btn = body[6]

                # Normalize to -1.0 .. 1.0
                nx = d_x / 127.0
                ny = d_y / 127.0

                hex_str = " ".join(f"{b:02x}" for b in report[:16])
                sys.stdout.write(
                    f"\rX={nx:+7.3f} Y={ny:+7.3f}  "
                    f"raw({d_x:+4d},{d_y:+4d},{d_z:+4d},{d_rx:+4d},{d_ry:+4d},{d_rz:+4d})  "
                    f"btns={btn:02x}  [{hex_str}]"
                )
                sys.stdout.flush()
        time.sleep(0.01)

except KeyboardInterrupt:
    print("\n\nDone.")
except Exception as e:
    print(f"\nError: {e}")
finally:
    try:
        dev.close()
    except:
        pass
