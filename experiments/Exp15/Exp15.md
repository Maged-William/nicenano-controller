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
| `TPS43_EDGESCROLL_ENABLE` | n | - | Master enable for 1-finger edge scroll |
| `TPS43_ABS_MAX_X` | 1024 | 1–65535 | Max absolute X value the touchpad reports |
| `TPS43_ABS_MAX_Y` | 1024 | 1–65535 | Max absolute Y value the touchpad reports |
| `TPS43_EDGE_RIGHT_PCT` | 15 | 3–50 | % from right edge where vertical scroll activates |
| `TPS43_EDGE_BOTTOM_PCT` | 15 | 3–50 | % from bottom edge where horizontal scroll activates |

### Behavior

When `TPS43_EDGESCROLL_ENABLE=y`:
- 1-finger in right edge → vertical scroll (tdy → mouse_wheel)
- 1-finger in bottom edge → horizontal scroll (tdx → mouse_wheel_h)
- 1-finger in center → pointer motion
- 2-finger scroll **disabled** (edge scroll replaces it)
- Taps/clicks/drags work normally (FSM unchanged)

### Success Criteria

- [ ] 1-finger on right edge → vertical scroll (up/down)
- [ ] 1-finger on bottom edge → horizontal scroll (left/right)
- [ ] 1-finger in center → pointer motion (unchanged)
- [ ] 2-finger scroll disabled when edge scroll enabled
- [ ] Taps/clicks/drags still work in edge zone (FSM unchanged)
- [ ] CI build passes with no warnings
