# Future Ideas

## Complications
- [ ] Distance Walked
- [ ] UTC Offset (e.g. -07:00 or +08:00)
- [ ] Sunrise / Sunset Time (designed for wider top slots)
- [ ] Daily High / Low Temp (Combined version for top slots, split individual versions for bottom slots)
- [x] Air Quality Index (AQI) / UV Index (Separated color-coding implemented for individual and combined views)

## Interactions
- [ ] Sustained touch / hold on the screen to temporarily display a secondary time zone over the main time (reverts instantly when released)
  - Feasible on Pebble Time 2's touchscreen via the touch service:
    `touch_service_subscribe()` delivers `TouchEvent_Touchdown` /
    `TouchEvent_PositionUpdate` / `TouchEvent_Liftoff` — show the secondary
    TZ on Touchdown, revert on Liftoff. Guard with `PBL_TOUCH` at compile
    time and `touch_service_is_enabled()` at runtime (returns false on
    non-touch platforms and when the user disables touch in Settings).
    Subscribing powers the touch sensor on (and triggers backlight per
    event), so subscribe in the window `appear` handler and unsubscribe on
    `disappear` to limit battery cost. `TouchEvent` carries `int16_t x, y`
    screen coordinates. Requires SDK 4.9+ (this repo's pebble-env has
    4.9.169, so we're set).
    Guide: <https://developer.repebble.com/guides/events-and-services/touch/>
    API reference: <https://developer.repebble.com/docs/c/Foundation/Event_Service/TouchService/>
