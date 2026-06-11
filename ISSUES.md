# Known Issues

Bugs and suspect behavior, ordered roughly by user impact. Feature ideas live
in [TODOs.md](TODOs.md).

## 3. Auto theme flip leaves time/date text invisible

**Symptom:** If the watchface stays open across 06:00 or 18:00 in Auto theme
(or a theme settings change arrives), the background switches but the time and
date keep their old color — white-on-white or black-on-black.

**Cause:** `text_layer_set_text_color()` for `s_time_layer` and
`s_date_iso_layer` is only called in `main_window_load()`
(`src/c/main.c:134,141`). `apply_theme()` updates the window background, and
`refresh_complications()` re-colors the slot layers, but nothing re-colors the
time/date layers. Goes unnoticed because relaunching the face (issue 1) resets
the colors.

**Fix direction:** Re-apply `s_active_theme->text_primary` to both layers in
`update_time()` (or inside `apply_theme()`).

## 4. JS sends 0 instead of "no data" for AQI and UV

**Symptom:** When the AQI fetch fails or UV is missing from the forecast
response, the watch shows a green `0` instead of `--`.

**Cause:** `src/pkjs/index.js` initializes `uv = 0` and falls back to
`WEATHER_AQI: 0` in the `onerror`/parse-failure paths. The watch side has
proper `-1` sentinels (`data.c`) that render `--`, but they're unreachable —
0 is a legitimate-looking value and colors green.

**Fix direction:** Omit the keys (or send `-1`) when the value is unknown, and
parse accordingly in `inbox_received_callback()`.

## 5. Double-pulse vibration on every launch while phone is disconnected

**Symptom:** If the phone is out of range, the watch buzzes every time you
return to the watchface.

**Cause:** `init()` seeds connection state by calling
`handle_bluetooth(connection_service_peek_pebble_app_connection())`
(`src/c/main.c:201`), and `handle_bluetooth()` vibrates whenever
`connected == false` — it can't distinguish a real disconnection event from
the initial peek.

**Fix direction:** Only vibrate on a true state transition (track previous
state, or set the initial state without going through the handler).

## Minor / housekeeping

- **UV shows daily max, not current UV.** The JS requests `uv_index_max`
  (today's peak). Fine if intentional, but the label "UV" suggests current
  conditions; evening UV will look alarmingly high.
- **Main weather XHR has no error/timeout handler** (`index.js`) — on network
  failure it fails silently; the AQI XHR has `onerror` but no timeout. No
  retry happens until the next 30-minute tick.
- **Persisted settings are keyed by auto-generated message-key IDs.**
  Reordering or inserting entries in `package.json`'s `messageKeys` array
  renumbers the keys and silently scrambles previously persisted settings.
  Append-only is safe; reordering is not.
- **`s_active_minutes` defaults to 22** (`data.c:15`) — looks like a leftover
  test value; harmless since health data overwrites it, but `0`/`-1` would be
  consistent with the other sentinels.
- **Unimplemented enum values are selectable nowhere but exist in the enum**
  (`DATA_SOURCE_DAY_NAME`, `SUNRISE`, `SUNSET`, `HIGH/LOW_TEMP`,
  `UTC_OFFSET`): `get_source_label()` returns `???` and `get_source_data()`
  returns an empty string. Dormant until those TODOs are implemented.
- **README still says the project "is configured as a watchapp"** —
  `package.json` already sets `watchapp.watchface: true`.
- **`fix_config.py` / `update_config.py`** are one-off config mutators in the
  repo root; candidates for deletion or a `scripts/` folder.
- **Test coverage gap:** `messaging.c` inbox logic is untested (the mock
  `dict_find` always returns NULL), and there are no tests for the
  persistence round-trip.
