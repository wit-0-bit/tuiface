#pragma once
#include <pebble.h>

// Weather cache persistence. Keys live well below the auto-generated
// MESSAGE_KEY_* range (10000+), which doubles as the settings persist range.
#define PERSIST_KEY_WEATHER_TEMP 1000
#define PERSIST_KEY_WEATHER_COND 1001
#define PERSIST_KEY_WEATHER_AQI 1002
#define PERSIST_KEY_WEATHER_UV 1003
#define PERSIST_KEY_WEATHER_TIMESTAMP 1004
#define WEATHER_CACHE_MAX_AGE_S (30 * 60)

void save_weather_cache(void);
bool load_weather_cache(void);

void request_weather();
void inbox_received_callback(DictionaryIterator *iterator, void *context);
void inbox_dropped_callback(AppMessageResult reason, void *context);
