#include <pebble.h>
#include "theme.h"
#include "drawing.h" // For s_main_window

const WatchTheme *s_active_theme = NULL;

const WatchTheme s_theme_night = {
  .center_bg = GColorBlack,
  .sidebar_bg = GColorBlack,
  .steps_fill = GColorChromeYellow,
  .battery_fill_high = GColorMintGreen,
  .battery_fill_med = GColorPastelYellow,
  .battery_fill_low = GColorSunsetOrange,
  .dividers = GColorDarkGray,
  .text_primary = GColorWhite,
  .text_secondary = GColorLightGray,
  .sidebar_text_unfilled = GColorWhite,
  .sidebar_text_filled = GColorBlack,
  .status_green = GColorMintGreen,
  .status_yellow = GColorPastelYellow,
  .status_red = GColorSunsetOrange
};

const WatchTheme s_theme_day = {
  .center_bg = GColorWhite,
  .sidebar_bg = GColorWhite,
  .steps_fill = GColorBlue,
  .battery_fill_high = GColorGreen,
  .battery_fill_med = GColorYellow,
  .battery_fill_low = GColorRed,
  .dividers = GColorLightGray,
  .text_primary = GColorBlack,
  .text_secondary = GColorDarkGray,
  .sidebar_text_unfilled = GColorBlack,
  .sidebar_text_filled = GColorWhite,
  .status_green = GColorGreen,
  .status_yellow = GColorYellow,
  .status_red = GColorRed
};

const WatchTheme* determine_theme(int theme_setting, int current_hour) {
  if (theme_setting == 1) return &s_theme_day;
  if (theme_setting == 2) return &s_theme_night;
  
  // Auto mode: Day = 6 AM to 5:59 PM
  if (current_hour >= 6 && current_hour < 18) {
    return &s_theme_day;
  }
  return &s_theme_night;
}

void apply_theme() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  s_active_theme = determine_theme(s_settings_theme, tick_time->tm_hour);
  
  if (s_main_window) {
    window_set_background_color(s_main_window, s_active_theme->center_bg);
  }
}

GColor get_source_color(ComplicationDataSource source) {
  if (!s_active_theme) return GColorWhite;
  
  switch(source) {
    case DATA_SOURCE_BATTERY:
      if (s_battery_level > 50) return s_active_theme->status_green;
      if (s_battery_level > 20) return s_active_theme->status_yellow;
      return s_active_theme->status_red;
    case DATA_SOURCE_STEPS:
    case DATA_SOURCE_ACTIVE_MINUTES:
      return s_active_theme->text_primary;
    case DATA_SOURCE_HEART_RATE:
      return s_active_theme->status_red;
    case DATA_SOURCE_WEATHER_TEMP:
    case DATA_SOURCE_WEATHER:
      if (s_settings_units == 1) { // Metric (Celsius)
        if (s_weather_temp > 29) return s_active_theme->status_red;
        if (s_weather_temp < 4) return s_active_theme->steps_fill; // Blue
      } else { // Imperial (Fahrenheit)
        if (s_weather_temp > 85) return s_active_theme->status_red;
        if (s_weather_temp < 40) return s_active_theme->steps_fill; // Blue
      }
      return s_active_theme->text_primary;
    case DATA_SOURCE_BLUETOOTH:
      return s_connected ? s_active_theme->status_green : s_active_theme->status_red;
    case DATA_SOURCE_AQI:
      if (s_weather_aqi == -1) return s_active_theme->text_primary;
      if (s_weather_aqi > 100) return s_active_theme->status_red;
      if (s_weather_aqi > 50) return s_active_theme->status_yellow;
      return s_active_theme->status_green;
    case DATA_SOURCE_UV:
      if (s_weather_uv == -1) return s_active_theme->text_primary;
      if (s_weather_uv >= 6) return s_active_theme->status_red;
      if (s_weather_uv >= 3) return s_active_theme->status_yellow;
      return s_active_theme->status_green;
    case DATA_SOURCE_AQI_UV: {
      if (s_weather_aqi == -1 && s_weather_uv == -1) return s_active_theme->text_primary;
      bool is_red = (s_weather_aqi > 100 || s_weather_uv >= 6);
      bool is_yellow = (s_weather_aqi > 50 || s_weather_uv >= 3);
      if (is_red) return s_active_theme->status_red;
      if (is_yellow) return s_active_theme->status_yellow;
      return s_active_theme->status_green;
    }
    default:
      return s_active_theme->text_primary;
  }
}
