# Known Issues

Bugs and suspect behavior, ordered roughly by user impact. Feature ideas live
in [TODOs.md](TODOs.md).

## Minor / housekeeping

- **Unimplemented enum values exist but are selectable nowhere**
  (`DATA_SOURCE_DAY_NAME`, `SUNRISE`, `SUNSET`, `HIGH/LOW_TEMP`,
  `UTC_OFFSET`): `get_source_label()` returns `???` and `get_source_data()`
  returns an empty string. Dormant until those TODOs are implemented.
- **`fix_config.py` / `update_config.py`** are one-off config mutators in the
  repo root; candidates for deletion or a `scripts/` folder.
- **Test coverage gap:** `messaging.c`'s `inbox_received_callback()` is
  untested (the mock `dict_find` always returns NULL); making it return
  scripted tuples would let tests cover settings parsing and the
  units-changed refetch path.
