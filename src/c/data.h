#pragma once
#include <pebble.h>

typedef enum {
  DATA_SOURCE_BATTERY = 0,
  DATA_SOURCE_STEPS = 1,
  DATA_SOURCE_SLEEP = 2,
  DATA_SOURCE_WEATHER_TEMP = 3,
  DATA_SOURCE_WEATHER_COND = 4,
  DATA_SOURCE_WEATHER = 5,
  DATA_SOURCE_HEART_RATE = 6,
  DATA_SOURCE_DATE = 7,
  DATA_SOURCE_DAY_NAME = 8,
  DATA_SOURCE_BLUETOOTH = 9,
  DATA_SOURCE_ACTIVE_MINUTES = 10,
  DATA_SOURCE_SUNRISE = 11,
  DATA_SOURCE_SUNSET = 12,
  DATA_SOURCE_HIGH_TEMP = 13,
  DATA_SOURCE_LOW_TEMP = 14,
  DATA_SOURCE_HIGH_LOW_TEMP = 15,
  DATA_SOURCE_AQI = 16,
  DATA_SOURCE_UV = 17,
  DATA_SOURCE_AQI_UV = 18,
  DATA_SOURCE_UTC_OFFSET = 19,
  DATA_SOURCE_EMPTY = 20
} ComplicationDataSource;

extern int s_battery_level;
extern int s_step_count;
extern int s_step_goal;
extern int s_sleep_seconds;
extern int s_heart_rate;
extern int s_weather_temp;
extern char s_weather_cond[16];
extern int s_weather_aqi;
extern int s_weather_uv;
extern int s_active_minutes;
extern int s_active_minutes_goal;
extern bool s_connected;

extern int s_date_day;

extern int s_settings_theme;
extern int s_settings_units;
extern int s_settings_date_format;

extern ComplicationDataSource s_left_sidebar_source;
extern ComplicationDataSource s_right_sidebar_source;

#define NUM_SLOTS 5
typedef struct {
  GRect box_rect;
  TextLayer *layer;
  ComplicationDataSource source;
} ComplicationSlot;

extern ComplicationSlot s_complication_slots[NUM_SLOTS];

void get_source_data(ComplicationDataSource source, char *val_buf, int val_len, int *percent);
const char* get_source_label(ComplicationDataSource source);
void format_date_string(int format, struct tm *tick_time, char *buffer, int buf_size);
void to_upper_str(char *str);
int tuple_get_int(Tuple *tuple);
