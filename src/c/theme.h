#pragma once
#include <pebble.h>
#include "data.h"

typedef struct {
  GColor center_bg;
  GColor sidebar_bg;
  GColor steps_fill;
  GColor battery_fill_high;
  GColor battery_fill_med;
  GColor battery_fill_low;
  GColor dividers;
  GColor text_primary;
  GColor text_secondary;
  GColor sidebar_text_unfilled;
  GColor sidebar_text_filled;
  GColor status_green;
  GColor status_yellow;
  GColor status_red;
} WatchTheme;

extern const WatchTheme* s_active_theme;

extern const WatchTheme s_theme_night;
extern const WatchTheme s_theme_day;

const WatchTheme* determine_theme(int theme_setting, int current_hour);
void apply_theme();
GColor get_source_color(ComplicationDataSource source);
