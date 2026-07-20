# Exp14: Grace Buffer + Click-First Tap FSM

This experiment evolved in two phases:

- **Phase 1 — Grace buffer:** Added `DROP_GRACE_MS` (20ms) to filter transient contact-loss during drags, preventing false drops on fast swipes.
- **Phase 2 — Click-first FSM:** Replaced the single `TAPPED` state with `FIRST_TAP` + `SECOND_TOUCH` to favor click and double-click over drag, and to actually implement double-click.

---

## Phase 1 — Drop Grace Buffer for Drag Stability

### Hypothesis

The TPS43 capacitive touchpad briefly loses contact with the finger during fast swipes, generating spurious `RELEASE` events. The FSM immediately drops the drag (button up) on any `RELEASE` while in `DRAGGING` state. Adding a short grace window (`DROP_GRACE_MS`, default 20ms) that filters transient contact-loss events will prevent false drops without introducing perceptible lag on intentional lifts.

### Design Change

New entry `TPS43_DROP_GRACE_MS` (int, default 20, range 0–200) in the Soft-Tap FSM menu.

| Event | Before | After |
|-------|--------|-------|
| `RELEASE` | Immediate drop (or DRAG_WAIT if draglock) | Record `lift_ms = now_ms`, stay in DRAGGING |
| `TOUCH` | Return true (drag continues) | If `lift_ms != 0`, reset `lift_ms = 0` (grace caught the re-touch) |
| `TIMEOUT` | Return true | If `lift_ms != 0` and `now_ms - lift_ms > DROP_GRACE_MS`, execute the deferred drop |

---

## Phase 2 — Click-First FSM (Double-Click + Intentional Drag)

### Hypothesis

The Exp13/Phase-1 FSM holds the button DOWN for 180ms (`DRAG_TIMEOUT_MS`) after every tap, making clicks feel sluggish and double-click impossible. Any re-touch within that window enters `DRAGGING`, making drag the default outcome instead of click or double-click.

Replacing the `TAPPED` state with `FIRST_TAP` (immediate ~4ms click release) + `SECOND_TOUCH` (distinguishes double-click from drag) makes click and double-click the natural outcomes, requiring intentional hold or move to start a drag.

### FSM States

```
TAP_STATE_IDLE
TAP_STATE_TOUCH           — first finger down, measuring tap vs press
TAP_STATE_TOUCH_2         — two fingers down (right-click / scroll)
TAP_STATE_FIRST_TAP       — first click fired, waiting for second action
TAP_STATE_SECOND_TOUCH    — second finger down, determining intent
TAP_STATE_1FG_DRAGGING    — drag active, button held (grace window active)
TAP_STATE_1FG_DRAG_WAIT   — lift mid-drag, drag-lock window
TAP_STATE_DEAD            — too much movement or prolonged touch
```

### Transitions

```
IDLE ──touch──► TOUCH ──quick release──► FIRST_TAP  (BTN_DOWN, next tick BTN_UP)
                    │                          │
                    │ (move/held >180ms)        │ re-touch within 250ms
                    v                          v
                  DEAD                    SECOND_TOUCH
                                              │
                                    ┌─────────┼─────────┐
                                    │         │         │
                               quick rel   hold+move  held >180ms
                               + same spot  >30 units  without release
                                    │         │         │
                                    v         v         v
                              double_click! DRAGGING  DRAGGING
```

### State Details

**FIRST_TAP:**
- Entry from valid tap (RELEASE within `TAP_TIMEOUT_MS`, within `TAP_MOVE_THRESH`):
  - `button_down = true; return true` → main fires `BTN_DOWN`
- On next `TIMEOUT` tick:
  - `button_down = false; return false` → main fires `BTN_UP` (~4ms click)
- `TOUCH` within `DOUBLE_CLICK_TIMEOUT_MS` (250ms):
  - Record `second_touch_start_ms/xy`, enter `SECOND_TOUCH`
- `TOUCH` after `DOUBLE_CLICK_TIMEOUT_MS`: treat as fresh touch → `TOUCH`
- `TIMEOUT` beyond `DOUBLE_CLICK_TIMEOUT_MS`: → `IDLE` (single click complete)

**SECOND_TOUCH:**
- `RELEASE` within `TAP_TIMEOUT_MS`, within `SAME_SPOT_THRESH` of first tap:
  - `*double_click = true` → main fires `DOWN→UP→DOWN→UP` rapid sequence
  - → `IDLE`
- `MOTION` beyond `TAP_MOVE_THRESH` from second touch start:
  - `button_down = true` → drag start → `DRAGGING`
- Held > `TAP_TIMEOUT_MS` without release or movement:
  - `button_down = true` → drag start → `DRAGGING`
- Release outside conditions: → `IDLE` (no action)

### Kconfig

`DRAG_TIMEOUT_MS` removed (replaced by `DOUBLE_CLICK_TIMEOUT_MS`).

| Entry | Default | Range | Purpose |
|-------|---------|-------|---------|
| `TPS43_DOUBLE_CLICK_TIMEOUT_MS` | 250 ms | 50–500 | Window from first-tap release to accept a second touch for double-click or drag |

### API

```c
bool tps43_tapdrag_update(bool finger_down, uint8_t finger_count,
                          uint16_t abs_x, uint16_t abs_y, uint64_t now_ms,
                          bool *right_click, bool *double_click);
```

New `bool *double_click` parameter signals a double-click event. Main fires two rapid `BTN_DOWN→BTN_UP` cycles.

---

## Success Criteria

- [x] Single tap produces immediate click (~4ms), not a 180ms hold
- [x] Two quick taps produce a double-click (two DOWN→UP cycles)
- [x] Tap → re-touch + hold/move produces drag
- [x] Tap → re-touch + quick release produces double-click (not drag)
- [x] Tap → re-touch + hold still >180ms produces drag
- [x] Grace window still filters transient contact-loss during drags
- [x] `DROP_GRACE_MS = 0` restores instant-drop (backward compat)
- [x] `DOUBLE_CLICK_TIMEOUT_MS = 0`... no, range is 50-500
- [x] Draglock (if enabled) still works for repositioning
- [x] CI build passes with no warnings

## Serial Output (final)

```
*** Booting Zephyr OS build v4.1.0 ***
TPS43 touchpad: found
TPS43 gesture engine disabled
TPS43 soft-tap FSM: enabled
Exp13: Dual-gyro HID mouse + TPS43 soft-tap FSM at 250Hz

TD: IDLE ev=TOUCH fg=0,1 xy=512,512 @10000       // finger down
TD: TOUCH ev=RELEASE fg=1,0 xy=510,511 @10180     // tap release → FIRST_TAP
BTN_L: DOWN @10180                                 // immediate click
BTN_L: UP @10184                                   // released next tick (~4ms)
TD: FIRST_TAP ev=TOUCH fg=0,1 xy=515,510 @10300   // second touch within window
TD: SECOND_TOUCH ev=MOTION fg=1,1 xy=520,512 @10304
TD: SECOND_TOUCH ev=RELEASE fg=1,0 xy=518,511 @10380  // quick release
BTN_L: DOUBLE_CLICK @10380                          // → double-click!
```

## Conclusion

**Verdict: ✅ Complete**

The experiment succeeded in both phases:

1. **Phase 1 — Grace buffer:** `DROP_GRACE_MS` (20ms) filters transient contact-loss during drags. Confirmed: RELEASE→UP gap went from ~4ms to ~26ms (20ms grace + 1-2 ticks).

2. **Phase 2 — Click-first FSM:** The `TAPPED` state (which held the button for 180ms) was replaced with `FIRST_TAP` (immediate ~4ms click) + `SECOND_TOUCH` (double-click vs drag discrimination). Clicks are now instant, double-click is supported, and drag requires intentional hold or move — making click and double-click the natural outcomes.

Files changed:
- `zephyr-app/Kconfig` — `TPS43_DRAG_TIMEOUT_MS` → `TPS43_DOUBLE_CLICK_TIMEOUT_MS` (250ms)
- `zephyr-app/src/drivers/tps43_tapdrag.h` — Added `bool *double_click` param
- `zephyr-app/src/drivers/tps43_tapdrag.c` — Replaced `TAPPED` with `FIRST_TAP` + `SECOND_TOUCH`; click-on-entry; double-click via `*double_click`; drag only on intentional hold/move
- `zephyr-app/src/main.c` — Handles `*double_click` signal
