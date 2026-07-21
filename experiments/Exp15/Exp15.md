# Exp15: 1-Finger Edge Scroll on TPS43

**Status: ✅ Complete**

---

### Hypothesis

Using the TPS43's absolute finger position to detect edge zones and redirecting 1-finger relative motion to wheel accumulators provides usable edge scrolling without changing the tap/drag FSM. The FSM is left unchanged — only motion routing changes.

### Design Evolution

| Iteration | Change | Result |
|-----------|--------|--------|
| Initial | Per-tick edge zone check, 2-finger scroll combined | Scroll speed too fast, abs_x/y bug broke taps |
| Fix 1 | 10x speed reduction, always read abs registers | Taps/drags fixed, scroll smoother |
| Fix 2 | Touch-start locks scroll mode for whole gesture | No mid-gesture mode switching |
| Final | 4 configurable edges (L/R/T/B), each with axis/speed/invert | Full flexibility, both axes scroll in mode |

### Behavior (final)

- **Touch starts** in any edge zone (pct > 0) → scroll mode for the whole gesture
- **In scroll mode**: tdy → vertical wheel, tdx → horizontal wheel
- Speed from whichever edge zone finger is currently in; defaults to 0.1x in center
- 1-finger center → pointer motion (unchanged)
- 2-finger scroll **disabled** when edge scroll enabled
- Taps/clicks/drags work normally (FSM unchanged)

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

- [x] 1-finger on right edge → vertical scroll (up/down)
- [x] 1-finger on bottom edge → horizontal scroll (left/right)
- [x] 1-finger on left/top edges → configurable scroll (axis, speed, invert)
- [x] Scroll speed configurable per edge (speed num/denom per edge)
- [x] Scroll axis configurable per edge (Y=0 or X=1)
- [x] Scroll direction invertible per edge (bool per edge)
- [x] Center → pointer, taps, drags all work (FSM untouched)
- [x] CI build passes with no warnings

### Conclusion

**Verdict: ✅ Complete**

The experiment succeeded. The four-edge scroll system provides configurable 1-finger edge scrolling on the TPS43 touchpad:

- **Dynamic per-tick**: scroll axes and speed follow the finger's current position across edges
- **Locked on touch-start**: scroll mode activates only when a touch begins in an edge zone, preventing accidental scroll mid-gesture
- **Full Kconfig flexibility**: each edge has independent zone width (%), axis (Y or X), speed (num/denom), and invert
- **No FSM changes**: tap, drag, double-click, right-click all work as before

Files changed:
- `zephyr-app/Kconfig` — Added `TPS43_EDGESCROLL_ENABLE`, 4-edge config (L/R/T/B) with ABS_MAX_X/Y, axis, speed, invert
- `zephyr-app/src/main.c` — Edge scroll mode detection, dynamic per-tick edge routing, debug ABS_X/ABS_Y columns
- `experiments/Exp15/Exp15.md` — Experiment document
