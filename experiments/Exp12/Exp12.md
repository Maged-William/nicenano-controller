# Exp12: Tap-and-Drag (Drag Lock) FSM for TPS43 Touchpad

## Hypothesis

A software-based tap-and-drag finite state machine (modeled after libinput's `evdev-mt-touchpad-tap.c`) can replace the TPS43's hardware tap-to-click (`gesture0` bit 0) to provide full tap-and-drag with drag lock, using the touchpad's absolute X/Y registers for position-based re-touch detection and movement confirmation.

## Background

The current TPS43 driver drives the left mouse button directly from the hardware gesture register's tap bit (`gesture0 & 0x01`). This provides simple tap-to-click but no tap-and-drag capability. libinput implements tap-and-drag with the following state machine:

```
IDLE → TOUCH → TAP → TAP_2 (armed) → DRAGGING
```

Plus a drag lock sub-state: when the finger lifts mid-drag, a timer starts; if the finger returns within the timeout, dragging continues.

The libinput defaults are well-tuned from years of user feedback:
- Tap timeout: ~180ms (max finger-down time to count as a tap)
- Drag timeout: 500ms (window to re-touch after lift)
- Move threshold: a few mm converted to device units

This experiment adds one extra branch not in libinput: a **confirm window** (timer2). After re-touching to arm the drag, if the finger moves before timer2 expires, the drag is confirmed. If it moves after timer2 expires, the button is released and the pointer moves normally. This is a deliberate deviation — we'll test both with and without this branch.

## Execution Plan

1. **Branch:** `Exp12` from `Exp11`
2. **`zephyr-app/src/drivers/tps43_tapdrag.h`** — Declare `tps43_tapdrag_update()` and `tps43_tapdrag_init()`
3. **`zephyr-app/src/drivers/tps43_tapdrag.c`** — Full FSM implementation:
   - 6 states: `ST_IDLE`, `ST_TOUCH`, `ST_TAP_WAIT`, `ST_ARMED`, `ST_DRAGGING`, `ST_LOCK_WAIT`
   - 3 timers: tap timeout, re-down window, drag lock timeout
   - 1 confirm window: timer2 for the extra branch
   - 2 position thresholds: tap move threshold, same-spot threshold
   - Cache `lastX/lastY` only while finger is down to avoid stale register reads
4. **`zephyr-app/Kconfig`** — Add `menu "Tap-and-Drag"` under TPS43 with 7 options:
   - `TPS43_TAPDRAG_ENABLE` (bool, default n)
   - `TPS43_TAP_MOVE_THRESH` (int, default 60)
   - `TPS43_SAME_SPOT_THRESH` (int, default 80)
   - `TPS43_TAP_MAX_TIME` (int, default 200ms)
   - `TPS43_REDOWN_WINDOW` (int, default 400ms)
   - `TPS43_CONFIRM_WINDOW` (int, default 150ms)
   - `TPS43_DRAG_LOCK_TIMEOUT` (int, default 500ms)
5. **`zephyr-app/src/main.c`** — Integrate FSM call every tick:
   - Extract absolute X/Y from `tps43_regs[9..12]`
   - Call `tps43_tapdrag_update()` always (not only when touched)
   - Feed `drag_left_button` into existing button edge-detection
   - Disable hardware `gesture0` tap bit handling when FSM is active
6. **Build** via GitHub Actions (or local PlatformIO)
7. **Flash** via Leonardo automation
8. **Verify** serial output + cursor behavior

## Implementation

### tps43_tapdrag.h

```c
#ifndef TPS43_TAPDRAG_H
#define TPS43_TAPDRAG_H

#include <stdbool.h>
#include <stdint.h>

void tps43_tapdrag_init(void);
bool tps43_tapdrag_update(bool finger_down, uint16_t abs_x, uint16_t abs_y, uint64_t now_ms);

#endif
```

### tps43_tapdrag.c

Complete FSM with 6 states, 3+1 timers, 2 position thresholds as described above. All tunables come from Kconfig.

### Integration in main.c

The FSM call replaces the `gesture0`-based tap logic:

```c
uint16_t abs_x = ((uint16_t)tps43_regs[9] << 8) | tps43_regs[10];
uint16_t abs_y = ((uint16_t)tps43_regs[11] << 8) | tps43_regs[12];
tps43_left_btn = tps43_tapdrag_update(fingers > 0, abs_x, abs_y, k_uptime_get());
```

The existing edge-detection pattern `(tps43_left_btn != tps43_left_btn_prev)` is reused unchanged.

### Kconfig Options

```
menu "Tap-and-Drag"
    depends on TPS43_ENABLE

config TPS43_TAPDRAG_ENABLE
    bool "Enable tap-and-drag with drag lock"
    default n
    help
      Software FSM replaces hardware tap-to-click. Provides tap-and-drag
      with drag lock (lift and re-touch within timeout to continue dragging).

config TPS43_TAP_MOVE_THRESH
    int "Tap movement threshold (abs counts)"
    default 60
    range 1 1000
    help
      Maximum finger movement during a touch for it to still count as a tap.

config TPS43_SAME_SPOT_THRESH
    int "Re-touch distance threshold (abs counts)"
    default 80
    range 1 1000
    help
      How close the re-touch must land to the tap-up position to arm the drag.

config TPS43_TAP_MAX_TIME
    int "Tap max hold time (ms)"
    default 200
    range 10 1000
    help
      Max finger-down duration for a touch to count as a tap.

config TPS43_REDOWN_WINDOW
    int "Tap re-touch window (ms)"
    default 400
    range 10 2000
    help
      Timer1: how long the user has to re-touch after a tap to arm a drag.

config TPS43_CONFIRM_WINDOW
    int "Drag confirm window (ms)"
    default 150
    range 10 1000
    help
      Timer2: if the finger moves within this window after re-touch,
      the drag is confirmed; if it moves later, the button is released
      and it becomes a normal pointer move.

config TPS43_DRAG_LOCK_TIMEOUT
    int "Drag lock timeout (ms)"
    default 500
    range 10 2000
    help
      Timer3: how long the user has to re-touch after lifting mid-drag
      to continue dragging (drag lock).
```

## Build Results

| Run | Time | Result | Notes |
|-----|------|--------|-------|
| Initial build | 5m 29s | ❌ Build failed | Missing CMakeLists.txt entry for tps43_tapdrag.c |
| Fixed CMakeLists.txt | 5m 06s | ✅ Clean build | UF2 artifact produced |

### Serial Output (at boot)

```
*** Booting Zephyr OS build v4.1.0 ***
ADC at 0x48
ADS1015 (12-bit)
TPS43 touchpad: found
TPS43 tap-drag FSM: enabled
BMI160 S1(500dps)=1 S2(125dps)=1
Calibrating gyro (hold still)... done
Exp11: Dual-gyro HID mouse + TPS43 touchpad at 250Hz
tick  FX  FY  FZ  CH0  CH1  CH2  CH3  TP_X  TP_Y  TP_F
```

All columns present, touchpad & FSM initialized, no crashes.

## Success Criteria

- [x] Firmware builds on GitHub Actions, produces UF2
- [x] Serial output shows normal operation (heartbeat, ADC, data rows)
- [x] "TPS43 tap-drag FSM: enabled" appears in serial output
- [ ] Tap-to-click still works (single tap produces a click) — **needs physical test**
- [ ] Tap-and-drag works: tap, lift, re-touch in same spot, drag moves cursor with button held — **needs physical test**
- [ ] Drag lock works: lift mid-drag, re-touch within timeout, drag continues — **needs physical test**
- [ ] Drag lock timeout works: lift mid-drag, wait >timeout, button released — **needs physical test**
- [ ] Confirm window branch works: re-touch, wait >confirm window, move → button released — **needs physical test**
- [ ] Normal touchpad scrolling (2-finger) is unaffected — **needs physical test**
- [ ] Combined gyro + touchpad operation is unaffected — **needs physical test**

## Challenges

- **Absolute position reliability** — Exp11 found TPS43 abs X/Y registers don't track consistently. Mitigated by caching `lastX/lastY` only while finger is down.
- **Timer granularity** — 4ms tick means all timers have ±4ms jitter. Thresholds should be >> 4ms.
- **Coordinate system** — Need to verify byte order and full-scale range of abs X/Y registers for meaningful threshold values.
- **Testing without hardware** — Position thresholds may need multiple tuning rounds on real hardware.

## Conclusion

**Verdict: FAILED — software tap-and-drag FSM is not viable with the TPS43.**

The Azoteq TPS43 touch controller has an internal gesture engine that fires hardware gesture events (SINGLE_TAP, TAP_AND_HOLD, SWIPE_x, etc.) on every touch. When any gesture fires, the `FINGER_COUNT` register drops to 0 for 66–100ms, and `abs_x/y` may hold stale values for ~25ms before going to 0xFFFF. This makes a software-based tap-drag FSM fundamentally unreliable:

1. **Gesture0 glitch**: Every touch (including re-touch during drag lock) triggers `gesture0` bit 0 or 1, which clears `FINGER_COUNT` to 0. The FSM sees a false "finger up" and cancels the drag.
2. **Abs coordinate unreliability**: The `abs_x/y` registers don't track consistently during the gesture window — sometimes they hold valid positions, sometimes 0xFFFF, sometimes stale values.
3. **Debounce/grace period workarounds insufficient**: Adding a grace period to ignore the glitch improves reliability but the underlying TPS43 behavior (hardware gesture firing on every touch) cannot be worked around cleanly — the hardware and software fight each other.

**What the TPS43 provides natively (and what we should use instead):**

| Register | Bit | Gesture | 
|----------|-----|---------|
| GestureEvents0 (0x0D) | 0x01 | SINGLE_TAP |
| GestureEvents0 (0x0D) | 0x02 | **TAP_AND_HOLD** |
| GestureEvents0 (0x0D) | 0x04–0x20 | SWIPE directions |
| GestureEvents1 (0x0E) | 0x01 | TWO_FINGER_TAP |
| GestureEvents1 (0x0E) | 0x02 | SCROLL |
| GestureEvents1 (0x0E) | 0x04 | ZOOM |

The TPS43 already has a native `TAP_AND_HOLD` gesture (bit 1 of GestureEvents0). It is configurable via `HoldTime` (0x06BD), `TapTime` (0x06B9), and `SFGestureEnable` (0x06B7). This should be the basis for any future tap-drag implementation.

### Changes made during this experiment

- `zephyr-app/src/drivers/tps43_tapdrag.c` — 6-state FSM with debounce, abs-validity grace period, confirm window
- `zephyr-app/Kconfig` — Tunables for tap time, move threshold, same-spot threshold, redown window, confirm window, drag lock timeout, arm move threshold, glitch grace period
- All code lives on branch `Exp12`

### What was learned

- TPS43 gesture engine is aggressive — it fires on every touch and cannot be easily bypassed
- Software FSM for tap-drag on this hardware is fighting the built-in gesture detection
- The TPS43's native `TAP_AND_HOLD` gesture is the correct mechanism for drag operations
- Abs X/Y are unreliable as a lift discriminator; gesture register bits should be used instead
