# Sidebar Complications Architecture

Reference for the design and purpose of the edge sidebars — the fill bars on
the left and right screen edges.

## Concept

A sidebar is a **progress bar**, not a text readout. It suits any metric with
a clear start and destination — a `0 → goal` or `0–100%` range that fills or
empties over time. The position is fixed (one bar per edge); the source is the
part meant to vary.

## Sources

**Today, the sources are hardwired:** steps on the left (progress toward the
daily step goal) and battery on the right (charge from 0% to 100%). There is
no source selection or "off" yet — `s_left_sidebar_source` and
`s_right_sidebar_source` are set in `data.c` and never changed.

**The intended direction** is a per-side selectable source: **off**, or any
progress-type metric, e.g.:

- **Steps** — progress toward the daily step goal
- **Battery** — charge from 0% to 100%
- **Distance** — toward a distance goal
- **Active minutes** — toward an active-time goal

Anything with a clear start and destination is fair game. Anything text-only
or discrete (Date, AQI, UV, Bluetooth status) is **not** — those belong in the
five main slots.

## Constraints

1. **Progress only.** A sidebar source must map to a `0–100%`/goal-progress
   value. Text or discrete-category complications never go in a sidebar.
2. **Fixed position.** There is one sidebar per edge and they do not move. The
   only thing that should ever vary is which source fills each one (or off) —
   never the position.
