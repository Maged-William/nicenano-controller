# Exp13: Pure-Software Tap FSM (Ignoring TPS43 Native Gestures)

## Hypothesis

The TPS43's native gesture engine can be disabled via the `SFGestureEnable` register (0x06B7) by writing 0x00, putting the chip into a raw touchpad mode where `FINGER_COUNT` remains stable and `abs_x/y` stay valid throughout a touch. With the gesture engine disabled, a pure-software FSM (ported from libinput's `evdev-mt-touchpad-tap.c`) can reliably implement tap-to-click, tap-and-drag with drag lock, right-click (2-finger tap), and double-click without fighting hardware gesture events.

## Background

Exp12 attempted a software tap-drag FSM but failed because:
1. TPS43's internal gesture engine fires on every touch, clearing `FINGER_COUNT → 0` for 66–100ms
2. `abs_x/y` go to `0xFFFF` after ~25ms during gesture windows
3. The FSM saw false "finger up" events and dropped drags mid-operation

The Exp12 conclusion recommended using the TPS43's native `TAP_AND_HOLD` gesture instead. However, the user wants full control over the gesture interpretation (click, double-click, right-click, drag-n-drop, scroll) via a unified software FSM.

## Key Insight

The TPS43 has a configuration register `SFGestureEnable` at address `0x06B7`. If we write 0x00 to this register, all built-in gesture processing is disabled. The TPS43 then acts as a raw touchpad:
- `FINGER_COUNT` stays at the correct value during the entire touch
- `abs_x/y` remain valid throughout
- `XREL/YREL` continue to report relative movement
- No gesture events fire to confuse the software

## FSM Design

The state machine is a direct port of libinput's `evdev-mt-touchpad-tap.c` with the following states:

```
TAP_STATE_IDLE
TAP_STATE_TOUCH               — finger down, measuring time + movement
TAP_STATE_1FGTAP_TAPPED       — 1-finger tap occurred (left button down)
TAP_STATE_2FGTAP_TAPPED       — 2-finger tap occurred (right button down)
TAP_STATE_1FGTAP_DRAGGING     — re-touch confirmed, drag active
TAP_STATE_1FGTAP_DRAGGING_WAIT — lift mid-drag, drag lock timer running
TAP_STATE_DEAD                — too much movement or timeout, not a tap
```

### State Transitions

```
IDLE ──TOUCH(1fg)──▶ TOUCH ──RELEASE(≤180ms, ≤1.3mm)──▶ 1FGTAP_TAPPED
                         │                                      │
                         │ MOVE>threshold or timeout            │ TOUCH(≤300ms, same spot)
                         ▼                                      ▼
                       DEAD ───────────────────────────────▶ 1FGTAP_DRAGGING
                                                               │
                                                          LIFT│
                                                               ▼
                                                     DRAGGING_WAIT ──TOUCH(≤300ms)──▶ DRAGGING
                                                               │
                                                          timeout│
                                                               ▼
                                                              IDLE

2-finger path:
IDLE ──TOUCH(2fg)──▶ TOUCH ──RELEASE(≤180ms)──▶ 2FGTAP_TAPPED (right click)
```

## Gesture Plan

| Gesture | Method | Action |
|---------|--------|--------|
| Single tap | 1 finger down+up ≤180ms, movement ≤1.3mm | Left click (Btn1) |
| Double tap | Tap → tap again within 180ms | Two left clicks |
| Right click | 2 fingers tap simultaneously | Right click (Btn2) |
| Tap-and-drag | Tap → re-touch ≤300ms at same spot → move | Hold Btn1 while dragging |
| Drag lock | Lift mid-drag → re-touch ≤300mm → continue | Continue drag after re-position |
| Scroll | 2 fingers slide | Vertical + horizontal wheel |
| Pointer move | 1 finger slide (no tap timing) | Normal cursor movement |

## Key Constants (libinput defaults)

| Constant | Value | Source |
|----------|-------|--------|
| `TAP_TIMEOUT` | 180 ms | libinput DEFAULT_TAP_TIMEOUT_PERIOD |
| `DRAG_TIMEOUT_BASE` | 160 ms | libinput DEFAULT_DRAG_TIMEOUT_PERIOD_BASE |
| `DRAG_TIMEOUT_PER_FINGER` | 20 ms | libinput DEFAULT_DRAG_TIMEOUT_PERIOD_PERFINGER |
| `DRAG_LOCK_TIMEOUT` | 300 ms | libinput DEFAULT_DRAGLOCK_TIMEOUT_PERIOD |
| `TAP_MOVE_THRESHOLD` | 1.3 mm (~30 TPS43 abs units) | libinput DEFAULT_TAP_MOVE_THRESHOLD |

## Execution Plan

1. **Branch:** `Exp13` from `main` (fresh start)
2. **`tps43.c`**: Add `tps43_write_config()` to write to 0x06xx registers, call it in `tps43_init()` to disable gestures
3. **`tps43.h`**: Add write function declaration and register defines
4. **`tps43_tapdrag.c`**: Complete rewrite with libinput-style FSM
5. **`tps43_tapdrag.h`**: Update API
6. **`main.c`**: Update integration — feed raw finger state to FSM, ignore gesture registers
7. **`Kconfig`**: Update with libinput-tuned constants
8. **Build** via GitHub Actions
9. **Flash** via Leonardo automation
10. **Verify** serial output + cursor behavior

## Success Criteria

- [ ] TPS43 gesture engine disabled (no GestureEvents0/GestureEvents1 bits set during touch)
- [ ] FINGER_COUNT remains stable throughout a touch
- [ ] Single tap produces left click
- [ ] Double tap produces two left clicks
- [ ] Two-finger tap produces right click
- [ ] Tap-and-drag works: tap, re-touch, drag with button held
- [ ] Drag lock works: lift mid-drag, re-touch within timeout, drag continues
- [ ] 2-finger scroll unaffected
- [ ] Combined gyro + touchpad HID mouse works

## Challenges

- **Register write protocol**: Need to verify TPS43 I2C write protocol for 0x06xx registers
- **Move threshold in abs units**: Need to calibrate 1.3mm to TPS43 abs coordinate units
- **2-finger detection**: Distinguishing 2-finger tap from sequential 1-finger touches
- **Finger_count reliability**: Even with gestures disabled, need to verify FINGER_COUNT is stable

## Conclusion

TBD
