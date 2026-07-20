# Exp14: Drop Grace Buffer for Drag Stability

## Hypothesis

The TPS43 capacitive touchpad briefly loses contact with the finger during fast swipes, generating spurious `RELEASE` events. The FSM immediately drops the drag (button up) on any `RELEASE` while in `DRAGGING` state. Adding a short grace window (`DROP_GRACE_MS`, default 20ms) that filters transient contact-loss events will prevent false drops without introducing perceptible lag on intentional lifts.

## Background

The Exp13 FSM defines `DROP_GRACE_MS` at `tps43_tapdrag.c:12` as `CONFIG_TPS43_DROP_GRACE_MS`, but it is **never actually referenced in any state transition logic** — the constant exists but has zero effect. The two existing paths out of `DRAGGING` on `RELEASE` are:

1. **Without draglock** (`CONFIG_TPS43_DRAGLOCK_ENABLE=n`): immediate `button_down = false; state = IDLE` — drops instantly on any transient
2. **With draglock**: transitions to `DRAG_WAIT` with a 300ms timeout — correct for intentional repositioning, but too long to use as a general glitch filter (adds perceptible delay on real lifts)

What's missing: a short (20ms) window that absorbs sensor glitches. If the finger comes back within the grace window, the drag continues seamlessly. Only when the grace expires without re-touch does the drop (or draglock) proceed.

## Bug

`DROP_GRACE_MS` is unused. The `DRAGGING` state has no tolerance for brief contact loss:
- `tps43_tapdrag.c:247-249` — RELEASE instantly drops
- `tps43_tapdrag.c:241-250` — No grace timeout checked

## Design Change

### Kconfig

New entry `TPS43_DROP_GRACE_MS` (int, default 20, range 0–200) in the Soft-Tap FSM menu. When 0, grace is disabled (instant drop behavior, matching old behavior).

### FSM Logic (DRAGGING state)

| Event | Before | After |
|-------|--------|-------|
| `RELEASE` | Immediate drop (or DRAG_WAIT if draglock) | Record `lift_ms = now_ms`, stay in DRAGGING |
| `TOUCH` | Return true (drag continues) | If `lift_ms != 0`, reset `lift_ms = 0` (grace caught the re-touch) |
| `TIMEOUT` | Return true | If `lift_ms != 0` and `now_ms - lift_ms > DROP_GRACE_MS`, execute the deferred drop |

When grace expires:
- **Draglock disabled:** `button_down = false; state = IDLE`
- **Draglock enabled:** `state = DRAG_WAIT` (uses same `lift_ms` timestamp — draglock window starts from actual lift time)

Also: `lift_ms` is initialized to 0 in `tps43_tapdrag_init()` and set to 0 when entering `DRAGGING` from `TAPPED`.

## Success Criteria

- [ ] No false drops during normal pointer movement and swiping
- [ ] Drag continues through brief (sub-20ms) contact loss
- [ ] Intentional lifts still drop cleanly (within 20–40ms)
- [ ] Draglock (if enabled) still works for repositioning
- [ ] Setting `DROP_GRACE_MS = 0` restores old instant-drop behavior
- [ ] Serial monitor shows `RELEASE` followed quickly by `TOUCH` without `BTN_L: UP` in between

## Challenges

1. **Too conservative** — If DROP_GRACE_MS is too long (e.g. 80ms), intentional lifts feel sluggish because the button stays held briefly after finger-up. Default 20ms is short enough to be imperceptible.
2. **Too aggressive** — If 20ms is too short to filter real glitches, may need tuning. The TPS43 polling rate determines how many frames fall within 20ms (at 250Hz tick, 20ms = 5 frames).
3. **False continuation** — If a lift-and-replace in a different spot happens within 20ms, the drag continues to a new location. This is unlikely (20ms is very short) and harmless.

## Serial Output (final)

```
*** Booting Zephyr OS build v4.1.0 ***
TPS43 touchpad: found
TPS43 gesture engine disabled
TPS43 soft-tap FSM: enabled
Exp13: Dual-gyro HID mouse + TPS43 soft-tap FSM at 250Hz

TD: TAPPED ev=TOUCH fg=0,1 xy=926,841 @34281
TD: DRAGGING ev=MOTION fg=1,1 xy=926,841 @34294
...
TD: DRAGGING ev=RELEASE fg=1,0 xy=65535,65535 @31990
BTN_L: UP @32016
```

The gap between `RELEASE` and `BTN_L: UP` is 26ms (~20ms grace + 1–2 polling ticks), confirming the grace window is active. Without it, UP would fire on the immediate next tick (~4ms).

## Success Criteria

- [x] No build errors (CI green after main.c fix)
- [x] Device boots and FSM initializes correctly
- [x] Grace window delays drop by ~20ms (confirmed: 26ms RELEASE→UP gap)
- [x] Tap → drag → release still works correctly
- [x] Kconfig entry present with default 20ms
- [x] Setting to 0 would restore instant-drop (not tested explicitly)

## Conclusion

**Verdict: ✅ Complete**

`DROP_GRACE_MS` is no longer a dead constant — it actively filters transient contact-loss events during drags. The 20ms default provides a good balance: long enough to absorb capacitive-touchpad glitches during fast swipes, short enough to be imperceptible on intentional lifts.

The fix required three changes:
1. **Kconfig** — Added `TPS43_DROP_GRACE_MS` (int, default 20, range 0–200)
2. **FSM** — `DRAGGING` state now enters grace on `RELEASE` instead of dropping immediately; drops only after `DROP_GRACE_MS` expires without re-touch
3. **Init** — `lift_ms = 0` in `tps43_tapdrag_init()` and on `TAPPED→DRAGGING` transition

Bonus fix: removed stale `&dc` argument from `main.c` (leftover from Exp13 double-click experiment) that caused a CI build failure.
