# Exp15: 1-Finger Edge Scroll on TPS43

**Status: In Progress**

---

### Motivation

Current scroll requires 2 fingers. Adding 1-finger edge scroll (right edge = vertical, bottom edge = horizontal) for convenience, matching macOS/libinput behavior.

### Hypothesis

Using the TPS43's absolute finger position to detect edge zones and redirecting 1-finger relative motion to wheel accumulators will provide usable edge scrolling without changing the tap/drag FSM.

### Design

**No FSM changes.** Edge scroll lives entirely in `main.c`, before the 1-finger pointer branch:

```
One finger touching:
  ├─ Right edge (abs_x > threshold)  → tdy → mouse_wheel  (vertical scroll)
  ├─ Bottom edge (abs_y > threshold) → tdx → mouse_wheel_h (horizontal scroll)
  └─ Else                             → pointer motion (unchanged)

Two+ fingers: unchanged 2-finger scroll
Taps/clicks/drags: unchanged (FSM operates independently of scroll routing)
```

Edge zones are defined as a configurable percentage from the right/bottom edge, computed against configurable absolute max X/Y values.

### Changes

| File | Change |
|------|--------|
| `zephyr-app/Kconfig` | New `TPS43_EDGESCROLL_ENABLE` + edge zone width config |
| `zephyr-app/src/main.c` | Edge zone detection before 1-finger pointer branch |

### Kconfig

| Entry | Default | Range | Purpose |
|-------|---------|-------|---------|
| `TPS43_EDGESCROLL_ENABLE` | n | - | Master enable |
| `TPS43_ABS_MAX_X` | 1024 | 1–65535 | Max absolute X |
| `TPS43_ABS_MAX_Y` | 1024 | 1–65535 | Max absolute Y |
| `TPS43_EDGE_LEFT_PCT` | 0 | 0–50 | Left zone width (0=disabled) |
| `TPS43_EDGE_LEFT_AXIS` | 1 (X) | 0=Y, 1=X | Left scroll axis |
| `TPS43_EDGE_LEFT_SPEED_NUM/DENOM` | 1/10 | 1–100 | Left scroll speed |
| `TPS43_EDGE_LEFT_INVERT` | n | - | Invert left scroll |
| `TPS43_EDGE_RIGHT_PCT` | 15 | 0–50 | Right zone width |
| `TPS43_EDGE_RIGHT_AXIS` | 0 (Y) | 0=Y, 1=X | Right scroll axis |
| `TPS43_EDGE_RIGHT_SPEED_NUM/DENOM` | 1/10 | 1–100 | Right scroll speed |
| `TPS43_EDGE_RIGHT_INVERT` | n | - | Invert right scroll |
| `TPS43_EDGE_TOP_PCT` | 0 | 0–50 | Top zone height |
| `TPS43_EDGE_TOP_AXIS` | 1 (X) | 0=Y, 1=X | Top scroll axis |
| `TPS43_EDGE_TOP_SPEED_NUM/DENOM` | 1/10 | 1–100 | Top scroll speed |
| `TPS43_EDGE_TOP_INVERT` | n | - | Invert top scroll |
| `TPS43_EDGE_BOTTOM_PCT` | 15 | 0–50 | Bottom zone height |
| `TPS43_EDGE_BOTTOM_AXIS` | 0 (Y) | 0=Y, 1=X | Bottom scroll axis |
| `TPS43_EDGE_BOTTOM_SPEED_NUM/DENOM` | 1/10 | 1–100 | Bottom scroll speed |
| `TPS43_EDGE_BOTTOM_INVERT` | n | - | Invert bottom scroll |

### Behavior

When `TPS43_EDGESCROLL_ENABLE=y`:
- Touch starts in any edge zone (pct>0) → scroll mode for the whole gesture
- Each tick: all active zones contribute to V/H scroll accumulators (dynamic position)
- Each edge has its own: axis, speed, invert
- 1-finger in center → pointer motion
- 2-finger scroll **disabled**
- Taps/clicks/drags work normally (FSM unchanged)

### Success Criteria

- [ ] 1-finger on right edge → vertical scroll (up/down)
- [ ] 1-finger on bottom edge → horizontal scroll (left/right)
- [ ] 1-finger on left/top edges → configurable scroll
- [ ] Scroll speed configurable per edge
- [ ] Scroll axis configurable per edge (Y or X)
- [ ] Scroll direction invertible per edge
- [ ] Center → pointer, taps, drags all work
- [ ] CI build passes with no warnings
