# Ideas

Unvetted brainstorm space — nothing here is decided, let alone promised.
When an idea gets the nod it graduates to [TODOs.md](TODOs.md); when it's
rejected it gets deleted (git history remembers). Proposals welcome — see
[CONTRIBUTING.md](CONTRIBUTING.md) for what a good pitch includes.

## Layout

- Flexible slot rows: 2 or 3 complications on top and bottom (2x2 / 3x2 /
  2x3 / 3x3) instead of the hardcoded 2-top/3-bottom. Nothing structural
  blocks it — slot rects are just hardcoded numbers; row width is 180px
  split evenly. Two shapes:
  - (a) explicit "row count" Clay settings — works, but two new knobs;
  - (b) auto-collapse: rows redistribute width among non-empty slots
    (Empty already exists), plus a new `SLOT_6` so the top row can hold
    three. Zero new settings; layout falls out of slot choices.
  - Wrinkle either way: wide complications (combined Weather, AQI/UV) need
    ~90px and would clip in a 3-up row — offer-and-let-clip, label them
    "(wide)" in the dropdown, or auto-fallback to a narrow rendering.

## Complications

- Distance Walked
- UTC Offset (e.g. -07:00 or +08:00)
- Sunrise / Sunset Time (designed for wider top slots)
- Daily High / Low Temp (combined version for top slots, split individual
  versions for bottom slots)

## Interactions

> **Platform note (June 2026):** touchscreen interactions are off the table
> for watchfaces. PebbleOS reserves the touch sensor for watchapps — a
> watchface's `touch_service_subscribe()` silently no-ops (see
> `src/fw/applib/touch_service.c` in
> [coredevices/PebbleOS](https://github.com/coredevices/PebbleOS): "Touch is
> reserved for watchapps; watchfaces must not consume it"). Note that
> `touch_service_is_enabled()` still returns true on watchfaces, so don't
> trust it. Accelerometer tap (`accel_tap_service_subscribe`) remains the
> only gesture input available to watchfaces.
