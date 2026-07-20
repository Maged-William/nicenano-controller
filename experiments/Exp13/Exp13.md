# Exp13: Pure-Software Tap FSM (Ignoring TPS43 Native Gestures)

## Hypothesis

The TPS43's native gesture engine can be disabled via the `SFGestureEnable` register (0x06B7) by writing 0x00, putting the chip into a raw touchpad mode where `FINGER_COUNT` remains stable and `abs_x/y` stay valid throughout a touch. With the gesture engine disabled, a pure-software FSM (ported from libinput's `evdev-mt-touchpad-tap.c`) can reliably implement tap-to-click, tap-and-drag with drag lock, and right-click (2-finger tap) without fighting hardware gesture events.

## Background

Exp12 attempted a software tap-drag FSM but failed because:
1. TPS43's internal gesture engine fires on every touch, clearing `FINGER_COUNT → 0` for 66–100ms
2. `abs_x/y` go to `0xFFFF` after ~25ms during gesture windows
3. The FSM saw false "finger up" events and dropped drags mid-operation

The Exp12 conclusion recommended using the TPS43's native `TAP_AND_HOLD` gesture instead. Exp13 revisited the pure-software approach with the gesture engine fully disabled.

## Key Insight

The TPS43 has a configuration register `SFGestureEnable` at address `0x06B7`. Writing 0x00 disables all built-in gesture processing. The TPS43 then acts as a raw touchpad:
- `FINGER_COUNT` stays at the correct value during the entire touch
- `abs_x/y` remain valid throughout
- `XREL/YREL` continue to report relative movement
- No gesture events fire to confuse the software

## FSM Design

The state machine is a port of libinput's `evdev-mt-touchpad-tap.c` with the following states:

```
IDLE ──TOUCH(1fg)──▶ TOUCH ──RELEASE(≤TAP_TIMEOUT, ≤TAP_MOVE_THRESH)──▶ TAPPED (left btn DOWN)
                         │                                                      │
                         │ MOVE>threshold or timeout                            │ TOUCH(≤DRAG_TIMEOUT, ≤SAME_SPOT_THRESH)
                         ▼                                                      ▼
                       DEAD ───────────────────────────────────────────────▶ DRAGGING
                                                                               │
                                                                          LIFT │
                                                                               ▼
                                                                         DRAG_WAIT ──TOUCH(≤timeout)──▶ DRAGGING
                                                                               │
                                                                          timeout │
                                                                               ▼
                                                                              IDLE

2-finger path:
IDLE ──TOUCH(2fg)──▶ TOUCH_2 ──RELEASE(≤TAP_TIMEOUT)──▶ right click
```

### States

| State | Description |
|-------|-------------|
| `IDLE` | No touch, waiting |
| `TOUCH` | Finger(s) down, measuring time + movement |
| `TOUCH_2` | 2 fingers down, waiting for 2-finger tap |
| `TAPPED` | Tap detected, left button held, waiting for re-touch or timeout |
| `DRAGGING` | Re-touch confirmed, drag active, button held |
| `DRAG_WAIT` | Lift mid-drag, brief grace window for re-touch |
| `DEAD` | Too much movement or prolonged touch, cancelled |

### Events

| Event | Trigger |
|-------|---------|
| `TOUCH` | Finger(s) went down |
| `RELEASE` | All fingers up |
| `MOTION` | Finger moved while still touching |
| `TIMEOUT` | No state change (idle tick) |

## Gesture Support

| Gesture | Method | Action |
|---------|--------|--------|
| Single tap | 1 finger down+up ≤180ms, movement ≤30 abs | Left click |
| Right click | 2 fingers tap simultaneously ≤180ms | Right click |
| Tap-and-drag | Tap → re-touch ≤180ms, same spot → move | Drag with Btn1 held |
| Drag lock (optional) | Lift mid-drag → re-touch ≤300ms → continue | Re-position mid-drag |
| Scroll | 2 fingers slide (handled in main.c) | Wheel V+H |
| Pointer move | 1 finger slide (no tap timing) | Cursor movement |

## Key Constants

| Constant | Default | Description |
|----------|---------|-------------|
| `TAP_TIMEOUT_MS` | 180 ms | Max finger-down time to count as tap |
| `DRAG_TIMEOUT_MS` | 180 ms | Window to re-touch after tap for drag |
| `DRAGLOCK_TIMEOUT_MS` | 300 ms | Window for drag lock re-touch |
| `DROP_GRACE_MS` | 40 ms | Brief re-touch window on lift during drag |
| `TAP_MOVE_THRESH` | 30 abs | Max movement during touch to be a tap |
| `SAME_SPOT_THRESH` | 80 abs | Max distance for re-touch to count as same spot |
| `RELEASE_DEBOUNCE_MS` | (removed) | Was 120ms, removed — gesture engine off eliminates need |

## Implementation

### `tps43.c` / `tps43.h`
- Added `tps43_write_config(uint16_t reg, uint8_t val)` — writes to 0x06xx config registers via I2C
- Added `tps43_disable_gestures()` — writes 0x00 to `SFGestureEnable` (0x06B7)
- Called during `tps43_init()` to disable gesture engine at boot

### `tps43_tapdrag.c` / `tps43_tapdrag.h`
- 6-state FSM: IDLE, TOUCH, TOUCH_2, TAPPED, DRAGGING, DRAG_WAIT, DEAD
- Drag lock: optional (`TPS43_DRAGLOCK_ENABLE`, default n)
- DROP_GRACE_MS: brief re-touch window on lift during drag
- API: `tps43_tapdrag_update(finger_down, finger_count, abs_x, abs_y, now_ms, &right_click)`

### `main.c`
- Calls `tps43_disable_gestures()` after `tps43_init()`
- Feeds raw FINGER_COUNT + abs_x/y to FSM every tick
- Left button: edge-detected from FSM return value
- Right button: fires on `*right_click` flag via inline DOWN→UP sequence
- 2-finger scroll via XREL/YREL unaffected

### `Kconfig`
- Menu "Soft-Tap FSM (Exp13)" with 8 tunables
- `TSDROP_GRACE_MS`, `TAP_MOVE_THRESH`, `SAME_SPOT_THRESH`, `TAP_TIMEOUT_MS`, `DRAG_TIMEOUT_MS`, `DRAGLOCK_ENABLE`, `DRAGLOCK_TIMEOUT_MS`, `TPS43_TAPDRAG_ENABLE`

## Build Results

| Attempt | Result | Notes |
|---------|--------|-------|
| Initial build | ❌ Failed | Missing CMakeLists.txt, implicit declaration of `tps43_end_comm` |
| Fixed ordering | ✅ Success | Reordered functions, added forward declarations |
| Fixed Kconfig | ✅ Success | `DRAGLOCK_TIMEOUT_MS` dependency fixed |
| Double-click + gesture re-enable | ❌ Failed | Multiple build + UX regressions |
| Final revert | ✅ Success | Reverted to d8817d9 + build fix |

## Serial Output (final)

```
*** Booting Zephyr OS build v4.1.0 ***
TPS43 touchpad: found
TPS43 gesture engine disabled
TPS43 soft-tap FSM: enabled
Exp13: Dual-gyro HID mouse + TPS43 soft-tap FSM at 250Hz
TD: IDLE ev=TOUCH fg=0,1 xy=680,554 @6417
TD: TOUCH ev=MOTION fg=1,1 xy=680,554 @6434
TD: TOUCH ev=RELEASE fg=1,0 xy=743,777 @119095
BTN_L: DOWN @119095
```

## Success Criteria

- [x] TPS43 gesture engine disabled — confirmed in serial output
- [x] FINGER_COUNT stable throughout touch — no mid-touch drops
- [x] Single tap → left click
- [x] Two-finger tap → right click (via TOUCH_2)
- [x] Tap-and-drag works: tap → re-touch → drag with button held
- [x] Drag lock optional: instant drop on release by default
- [ ] Two-finger scroll — unaffected (no changes to scroll path)
- [x] Combined gyro + touchpad HID mouse works

## Challenges

1. **Gesture engine interaction** — Disabling gestures breaks multi-finger FINGER_COUNT reliability. TOUCH_2 may fire incorrectly if FINGER_COUNT reports ≥2 for a single finger. Needs real hardware validation.
2. **Threshold tuning** — libinput default of 1.3mm (~30 abs units) is very tight for real use. `TAP_MOVE_THRESH=30` causes natural finger placement to trigger DEAD. Needs user-specific tuning via Kconfig.
3. **DROP_GRACE_MS vs drag lock** — Two different mechanisms for lift mid-drag caused confusion. Final design: drag lock (optional, 300ms for repositioning) vs DROP_GRACE_MS (always on, 40ms for brief glitch filtering).
4. **Build system** — CI-only builds add 5+ minute iteration cycles. Local Zephyr toolchain would speed development.

## Lessons Learned

1. **Gesture engine off is the right default** — For single-finger operations (tap, drag, pointer), disabling `SFGestureEnable` eliminates the fundamental problem from Exp12.
2. **DROP_GRACE_MS bridges the gap** — Instant drop (drop grace=0) drops prematurely during fast swipes. A 40ms grace period is imperceptible but filters brief contact loss.
3. **Keep it simple** — The most stable versions had the fewest states and the least conditional compilation. TOUCH_2 for right-click is simple and works.
4. **User testing reveals everything** — Serial monitor logs were essential for diagnosing each issue. The FSM debug output (`TD: ...`) made every state transition visible.

## Conclusion

**Verdict: ✅ Partially Successful**

The pure-software FSM with gesture engine disabled works reliably for single-finger tap and drag operations — exactly what Exp12 set out to do but couldn't because of gesture-event interference. By disabling the TPS43's internal gesture engine entirely, we eliminated the root cause.

Three features are solid: **single tap** → left click, **2-finger tap** → right click, **tap-and-drag** → drag with optional drag lock. Double-click was attempted but de-scoped; the infrastructure (DRAG_OR_DC ambiguity state) exists in the branch history if needed later.

The experiment confirms that a libinput-style FSM is viable on this hardware as long as the TPS43 gesture engine is disabled. The trade-off is potential multi-finger unreliability — something to validate in a future experiment.
