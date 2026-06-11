# Known Issues

Bugs and suspect behavior, ordered roughly by user impact. Feature ideas live
in [TODOs.md](TODOs.md).

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
