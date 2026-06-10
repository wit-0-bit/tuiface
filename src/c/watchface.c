#include <pebble.h>

// -----------------------------------------------------------------------------
// Data Models & Structures
// -----------------------------------------------------------------------------

typedef enum {
  DATA_SOURCE_BATTERY,
  DATA_SOURCE_STEPS,
  DATA_SOURCE_SLEEP,
  DATA_SOURCE_WEATHER_TEMP,
  DATA_SOURCE_WEATHER_COND,
  DATA_SOURCE_HEART_RATE,
  DATA_SOURCE_DATE,
  DATA_SOURCE_DAY_NAME,
  DATA_SOURCE_BLUETOOTH,
  DATA_SOURCE_ACTIVE_MINUTES,
  DATA_SOURCE_EMPTY
} ComplicationDataSource;

typedef struct {
  ComplicationDataSource source;
  char label[8];
} SidebarComplicationSlot;

// Forward declaration of data provider
static void get_source_data(ComplicationDataSource source, char *val_buf, int val_len, int *percent);
static void update_time(void);

// Sensor & System Data Cache (must be defined before callback use)
static int s_battery_level = 100;
static int s_step_count = -1;        // -1 indicates no data
static int s_step_goal = 10000;
static int s_sleep_seconds = -1;     // -1 indicates no data
static int s_heart_rate = 0;         // Default to 0 (displays "--" if no HRM is present)
static int s_weather_temp = -999;    // -999 indicates no data
static char s_weather_cond[16] = "--";
static int s_active_minutes = 22;
static int s_active_minutes_goal = 30;
static bool s_connected = true;

static char s_day_name[4] = "MON";
static int s_date_day = 10;

// Settings configurations
static int s_settings_theme = 0;        // 0 = Auto, 1 = Day, 2 = Night
static int s_settings_units = 0;        // 0 = Imperial, 1 = Metric
static int s_settings_date_format = 0;  // 0 = Weekday + ISO, 1 = ISO + Weekday, 2 = Full Text

// Global settings configurations (can be loaded/saved in future)
static SidebarComplicationSlot s_left_sidebar = { .source = DATA_SOURCE_STEPS, .label = "STEP" };
static SidebarComplicationSlot s_right_sidebar = { .source = DATA_SOURCE_BATTERY, .label = "BATT" };

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

static const WatchTheme s_theme_night = {
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

static const WatchTheme s_theme_day = {
  .center_bg = GColorWhite,
  .sidebar_bg = GColorWhite,
  .steps_fill = GColorOrange,
  .battery_fill_high = GColorDarkGreen,
  .battery_fill_med = GColorChromeYellow,
  .battery_fill_low = GColorRed,
  .dividers = GColorDarkGray,
  .text_primary = GColorBlack,
  .text_secondary = GColorDarkGray,
  .sidebar_text_unfilled = GColorBlack,
  .sidebar_text_filled = GColorWhite,
  .status_green = GColorIslamicGreen,
  .status_yellow = GColorChromeYellow,
  .status_red = GColorRed
};

static const WatchTheme *s_active_theme = &s_theme_night;

// Module callback implementations
static void get_weather_text(char *buffer, size_t len) {
  char cond_buf[16];
  char temp_buf[16];
  get_source_data(DATA_SOURCE_WEATHER_COND, cond_buf, sizeof(cond_buf), NULL);
  get_source_data(DATA_SOURCE_WEATHER_TEMP, temp_buf, sizeof(temp_buf), NULL);
  snprintf(buffer, len, "%s / %s", cond_buf, temp_buf);
}

static GColor get_weather_color(void) {
  return s_active_theme->text_primary;
}

static void get_sleep_text(char *buffer, size_t len) {
  get_source_data(DATA_SOURCE_SLEEP, buffer, len, NULL);
}

static GColor get_sleep_color(void) {
  return s_active_theme->text_primary;
}

static void get_bpm_text(char *buffer, size_t len) {
  get_source_data(DATA_SOURCE_HEART_RATE, buffer, len, NULL);
}

static GColor get_bpm_color(void) {
  if (s_heart_rate <= 0) return s_active_theme->text_primary;
  if (s_heart_rate > 140) return s_active_theme->status_red;
  if (s_heart_rate > 100) return s_active_theme->status_yellow;
  return s_active_theme->status_green;
}

static void get_step_text(char *buffer, size_t len) {
  get_source_data(DATA_SOURCE_STEPS, buffer, len, NULL);
}

static GColor get_step_color(void) {
  return s_active_theme->text_primary;
}

static void get_link_text(char *buffer, size_t len) {
  get_source_data(DATA_SOURCE_BLUETOOTH, buffer, len, NULL);
}

static GColor get_link_color(void) {
  return s_connected ? s_active_theme->status_green : s_active_theme->status_red;
}

typedef struct {
  char name[12];
  void (*get_text)(char *buffer, size_t buffer_len);
  GColor (*get_color)(void);
} ComplicationModule;

static const ComplicationModule s_module_weather = { "WEATHER", get_weather_text, get_weather_color };
static const ComplicationModule s_module_sleep   = { "SLEEP",   get_sleep_text,   get_sleep_color };
static const ComplicationModule s_module_bpm     = { "BPM",     get_bpm_text,     get_bpm_color };
static const ComplicationModule s_module_step    = { "STEP",    get_step_text,    get_step_color };
static const ComplicationModule s_module_link    = { "LINK",    get_link_text,    get_link_color };


#define NUM_SLOTS 5

typedef struct {
  GRect box_rect;
  TextLayer *layer;
  const ComplicationModule *module;
} ComplicationSlot;

static ComplicationSlot s_complication_slots[NUM_SLOTS] = {
  {
    .box_rect = {{10, 8}, {90, 36}},
    .module = &s_module_weather
  },
  {
    .box_rect = {{100, 8}, {90, 36}},
    .module = &s_module_sleep
  },
  {
    .box_rect = {{10, 184}, {60, 36}},
    .module = &s_module_step
  },
  {
    .box_rect = {{70, 184}, {60, 36}},
    .module = &s_module_bpm
  },
  {
    .box_rect = {{130, 184}, {60, 36}},
    .module = &s_module_link
  }
};

// Themes moved to top of file

// -----------------------------------------------------------------------------
// UI State & Variables
// -----------------------------------------------------------------------------

static Window *s_main_window;
static Layer *s_canvas_layer;

// Center Time Display
static TextLayer *s_time_layer;
static TextLayer *s_date_iso_layer;

// Parameterized complications are stored globally inside the slots array.

// Sensor & System Data Cache moved to the top of the file

// -----------------------------------------------------------------------------
// Data Providers
// -----------------------------------------------------------------------------

// Fetch value string and fill percentage for a given data source
static void get_source_data(ComplicationDataSource source, char *val_buf, int val_len, int *percent) {
  if (percent) *percent = 0;
  val_buf[0] = '\0';

  switch (source) {
    case DATA_SOURCE_BATTERY:
      snprintf(val_buf, val_len, "%d%%", s_battery_level);
      if (percent) *percent = s_battery_level;
      break;
    case DATA_SOURCE_STEPS:
      if (s_step_count == -1) {
        snprintf(val_buf, val_len, "--");
      } else if (s_step_count >= 10000) {
        int whole = s_step_count / 1000;
        int tenth = (s_step_count % 1000) / 100;
        snprintf(val_buf, val_len, "%d.%dk", whole, tenth);
      } else {
        snprintf(val_buf, val_len, "%d", s_step_count);
      }
      if (percent) {
        *percent = s_step_count > 0 ? (s_step_count * 100) / s_step_goal : 0;
        if (*percent > 100) *percent = 100;
      }
      break;
    case DATA_SOURCE_SLEEP: {
      if (s_sleep_seconds == -1) {
        snprintf(val_buf, val_len, "--");
      } else {
        int hrs = s_sleep_seconds / 3600;
        int mins = (s_sleep_seconds % 3600) / 60;
        snprintf(val_buf, val_len, "%dh %dm", hrs, mins);
      }
      if (percent) {
        *percent = s_sleep_seconds > 0 ? (s_sleep_seconds * 100) / 28800 : 0; // 8-hour goal
        if (*percent > 100) *percent = 100;
      }
      break;
    }
    case DATA_SOURCE_WEATHER_TEMP:
      if (s_weather_temp == -999) {
        snprintf(val_buf, val_len, "--");
      } else {
        snprintf(val_buf, val_len, "%d%s", s_weather_temp, (s_settings_units == 1) ? "C" : "F");
      }
      break;
    case DATA_SOURCE_WEATHER_COND:
      snprintf(val_buf, val_len, "%s", s_weather_cond);
      break;
    case DATA_SOURCE_HEART_RATE:
      if (s_heart_rate > 0) {
        snprintf(val_buf, val_len, "%d", s_heart_rate);
      } else {
        snprintf(val_buf, val_len, "--");
      }
      break;
    case DATA_SOURCE_DATE:
      snprintf(val_buf, val_len, "%d", s_date_day);
      break;
    case DATA_SOURCE_DAY_NAME:
      snprintf(val_buf, val_len, "%s", s_day_name);
      break;
    case DATA_SOURCE_BLUETOOTH:
      snprintf(val_buf, val_len, s_connected ? "OK" : "LOSS");
      if (percent) *percent = s_connected ? 100 : 0;
      break;
    case DATA_SOURCE_ACTIVE_MINUTES:
      snprintf(val_buf, val_len, "%dm", s_active_minutes);
      if (percent) {
        *percent = (s_active_minutes * 100) / s_active_minutes_goal;
        if (*percent > 100) *percent = 100;
      }
      break;
    default:
      break;
  }
}

// Update local date information
static void update_date_info() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  s_date_day = tick_time->tm_mday;
  strftime(s_day_name, sizeof(s_day_name), "%a", tick_time);
  for (int i = 0; s_day_name[i]; i++) {
    if (s_day_name[i] >= 'a' && s_day_name[i] <= 'z') {
      s_day_name[i] -= 32;
    }
  }
}

static int s_mock_steps_offset = 0;

// Fetch health metrics if available
static void update_health_info() {
#if defined(PBL_HEALTH)
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Steps
  if (health_service_metric_accessible(HealthMetricStepCount, start, end) & HealthServiceAccessibilityMaskAvailable) {
    s_step_count = (int)health_service_sum_today(HealthMetricStepCount) + s_mock_steps_offset;
    if (health_service_metric_averaged_accessible(HealthMetricStepCount, start, start + SECONDS_PER_DAY, HealthServiceTimeScopeDaily) & HealthServiceAccessibilityMaskAvailable) {
      s_step_goal = (int)health_service_sum_averaged(HealthMetricStepCount, start, start + SECONDS_PER_DAY, HealthServiceTimeScopeDaily);
    } else {
      s_step_goal = 10000;
    }
    if (s_step_goal == 0) s_step_goal = 10000;
  } else {
    s_step_count = -1;
    s_step_goal = 10000;
  }

  // Active Minutes
  if (health_service_metric_accessible(HealthMetricActiveSeconds, start, end) & HealthServiceAccessibilityMaskAvailable) {
    s_active_minutes = (int)health_service_sum_today(HealthMetricActiveSeconds) / 60;
  }

  // Sleep
  if (health_service_metric_accessible(HealthMetricSleepSeconds, start, end) & HealthServiceAccessibilityMaskAvailable) {
    s_sleep_seconds = (int)health_service_sum_today(HealthMetricSleepSeconds);
  } else {
    s_sleep_seconds = -1;
  }

  // Heart Rate (Pebble 2 / HRM models)
  HealthServiceAccessibilityMask hr_mask = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    s_heart_rate = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
  } else {
    s_heart_rate = 0;
  }
#endif
}

// -----------------------------------------------------------------------------
// Drawing & Render Helpers
// -----------------------------------------------------------------------------

// Draw a vertical progress bar with stacked label overlay
static void draw_sidebar_complication(GContext *ctx, GRect bounds, SidebarComplicationSlot slot, GColor fill_color, bool from_top) {
  int percent = 0;
  char val_str[16];
  get_source_data(slot.source, val_str, sizeof(val_str), &percent);

  // Background track (fully black)
  graphics_context_set_fill_color(ctx, s_active_theme->sidebar_bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Fill progress bar
  int fill_height = (bounds.size.h * percent) / 100;
  if (fill_height > bounds.size.h) fill_height = bounds.size.h;

  if (fill_height > 0) {
    graphics_context_set_fill_color(ctx, fill_color);
    int y_start = from_top ? bounds.origin.y : (bounds.origin.y + bounds.size.h - fill_height);
    graphics_fill_rect(ctx, GRect(bounds.origin.x, y_start, bounds.size.w, fill_height), 0, GCornerNone);
  }
}

static void draw_plus(GContext *ctx, int x, int y) {
  graphics_draw_line(ctx, GPoint(x - 2, y), GPoint(x + 2, y));
  graphics_draw_line(ctx, GPoint(x, y - 2), GPoint(x, y + 2));
}

static void draw_dashed_vline(GContext *ctx, int x, int y1, int y2) {
  for (int y = y1; y < y2; y += 6) {
    int len = (y + 3 < y2) ? 3 : (y2 - y);
    graphics_draw_line(ctx, GPoint(x, y), GPoint(x, y + len));
  }
}

static void draw_dashed_hline(GContext *ctx, int x1, int x2, int y) {
  for (int x = x1; x < x2; x += 6) {
    int len = (x + 3 < x2) ? 3 : (x2 - x);
    graphics_draw_line(ctx, GPoint(x, y), GPoint(x + len, y));
  }
}

static void draw_ascii_window(GContext *ctx, GRect rect, const char *title) {
  int x = rect.origin.x;
  int y = rect.origin.y;
  int w = rect.size.w;
  int h = rect.size.h;

  graphics_context_set_stroke_color(ctx, s_active_theme->text_primary);
  graphics_context_set_stroke_width(ctx, 1);

  // Draw vertical borders (dashed)
  draw_dashed_vline(ctx, x, y + 2, y + h - 2);
  draw_dashed_vline(ctx, x + w, y + 2, y + h - 2);

  // Draw bottom border (dashed)
  draw_dashed_hline(ctx, x + 2, x + w - 2, y + h);

  // Draw top border (dashed, with title gap)
  int title_len = strlen(title);
  int title_width = title_len * 8 + 4; // approx 8px per character + padding
  if (title_width > w - 10) title_width = w - 10;
  
  int x_mid = x + w / 2;
  int gap_left = x_mid - title_width / 2 - 3;
  int gap_right = x_mid + title_width / 2 + 3;

  draw_dashed_hline(ctx, x + 2, gap_left, y);
  draw_dashed_hline(ctx, gap_right, x + w - 2, y);

  // Draw four corner crosses (+)
  draw_plus(ctx, x, y);
  draw_plus(ctx, x + w, y);
  draw_plus(ctx, x, y + h);
  draw_plus(ctx, x + w, y + h);

  // Draw title text
  graphics_context_set_text_color(ctx, s_active_theme->text_secondary);
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(gap_left, y - 7, gap_right - gap_left, 14),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static GColor get_interpolate_color(int percent) {
#if defined(PBL_COLOR)
  int r = ((100 - percent) * 255) / 100;
  int g = (percent * 255) / 100;
  int b = 0;
  return GColorFromRGB(r, g, b);
#else
  return GColorWhite;
#endif
}

// Main canvas rendering (separators and sidebars)
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Clean background of center region (now starting at x=4, width=bounds.size.w - 8)
  graphics_context_set_fill_color(ctx, s_active_theme->center_bg);
  graphics_fill_rect(ctx, GRect(4, 0, bounds.size.w - 8, bounds.size.h), 0, GCornerNone);

  // Draw Left Sidebar Progress (Steps -> Slate/Gold/Yellow color scheme, width 4)
  int steps_percent = s_step_goal > 0 ? (s_step_count * 100) / s_step_goal : 0;
  if (steps_percent > 100) steps_percent = 100;
  draw_sidebar_complication(ctx, GRect(0, 0, 4, bounds.size.h), s_left_sidebar, get_interpolate_color(steps_percent), true);

  // Draw Right Sidebar Progress (Battery -> Green/Yellow/Red scheme, width 4)
  draw_sidebar_complication(ctx, GRect(bounds.size.w - 4, 0, 4, bounds.size.h), s_right_sidebar, get_interpolate_color(s_battery_level), false);

  // Draw TIME and DATE windows
  draw_ascii_window(ctx, GRect(10, 50, 180, 86), "TIME");
  draw_ascii_window(ctx, GRect(10, 142, 180, 36), "DATE");

  // Draw parameterized ASCII windows
  for (int i = 0; i < NUM_SLOTS; i++) {
    ComplicationSlot *slot = &s_complication_slots[i];
    if (slot->module) {
      draw_ascii_window(ctx, slot->box_rect, slot->module->name);
    }
  }
}

// -----------------------------------------------------------------------------
// UI Updates
// -----------------------------------------------------------------------------

// Refresh text layers with complication data
static void refresh_complications() {
  static char s_slot_buffers[NUM_SLOTS][40];

  for (int i = 0; i < NUM_SLOTS; i++) {
    ComplicationSlot *slot = &s_complication_slots[i];
    if (slot->module && slot->layer) {
      slot->module->get_text(s_slot_buffers[i], sizeof(s_slot_buffers[i]));
      text_layer_set_text(slot->layer, s_slot_buffers[i]);

#if defined(PBL_COLOR)
      text_layer_set_text_color(slot->layer, slot->module->get_color());
#else
      text_layer_set_text_color(slot->layer, s_active_theme->text_primary);
#endif
    }
  }
}

static void request_weather() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) return;
  
  dict_write_uint8(iter, MESSAGE_KEY_WEATHER_TEMP, 0); // Trigger fetch
  app_message_outbox_send();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Weather
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_TEMP);
  Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_COND);
  if (temp_tuple && cond_tuple) {
    s_weather_temp = temp_tuple->value->int32;
    snprintf(s_weather_cond, sizeof(s_weather_cond), "%s", cond_tuple->value->cstring);
  }

  // Settings: Theme
  Tuple *theme_tuple = dict_find(iterator, MESSAGE_KEY_SETTINGS_THEME);
  if (theme_tuple) {
    int val = 0;
    if (theme_tuple->type == TUPLE_CSTRING) {
      val = atoi(theme_tuple->value->cstring);
    } else {
      val = theme_tuple->value->int32;
    }
    s_settings_theme = val;
    persist_write_int(MESSAGE_KEY_SETTINGS_THEME, s_settings_theme);
  }

  // Settings: Units
  Tuple *units_tuple = dict_find(iterator, MESSAGE_KEY_SETTINGS_UNITS);
  bool units_changed = false;
  if (units_tuple) {
    int val = 0;
    if (units_tuple->type == TUPLE_CSTRING) {
      val = atoi(units_tuple->value->cstring);
    } else {
      val = units_tuple->value->int32;
    }
    if (s_settings_units != val) {
      s_settings_units = val;
      units_changed = true;
    }
    persist_write_int(MESSAGE_KEY_SETTINGS_UNITS, s_settings_units);
  }

  // Settings: Date Format
  Tuple *date_format_tuple = dict_find(iterator, MESSAGE_KEY_SETTINGS_DATE_FORMAT);
  if (date_format_tuple) {
    int val = 0;
    if (date_format_tuple->type == TUPLE_CSTRING) {
      val = atoi(date_format_tuple->value->cstring);
    } else {
      val = date_format_tuple->value->int32;
    }
    s_settings_date_format = val;
    persist_write_int(MESSAGE_KEY_SETTINGS_DATE_FORMAT, s_settings_date_format);
  }

  // If units changed, request new weather immediately to fetch correct unit
  if (units_changed) {
    request_weather();
  }

  // Redraw UI with new settings/weather
  update_time();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", reason);
}

static const char* get_ordinal_suffix(int day) {
  if (day == 1 || day == 21 || day == 31) return "st";
  if (day == 2 || day == 22) return "nd";
  if (day == 3 || day == 23) return "rd";
  return "th";
}

// Refresh time display
static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Determine active theme based on setting and current hour
  const WatchTheme *new_theme = &s_theme_night;
  if (s_settings_theme == 1) {
    new_theme = &s_theme_day;
  } else if (s_settings_theme == 2) {
    new_theme = &s_theme_night;
  } else {
    // Auto (Day/Night)
    if (tick_time->tm_hour >= 6 && tick_time->tm_hour < 18) {
      new_theme = &s_theme_day;
    }
  }

  bool theme_changed = (s_active_theme != new_theme);
  if (theme_changed) {
    s_active_theme = new_theme;
    if (s_main_window) {
      window_set_background_color(s_main_window, s_active_theme->center_bg);
    }
    if (s_time_layer) {
      text_layer_set_text_color(s_time_layer, s_active_theme->text_primary);
    }
    if (s_date_iso_layer) {
      text_layer_set_text_color(s_date_iso_layer, s_active_theme->text_primary);
    }
  }

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
  
  static char s_date_iso_buffer[64]; // Increased to 64 to prevent truncation warning
  if (s_settings_date_format == 0) {
    // Weekday + ISO (TUE 2026-06-09)
    strftime(s_date_iso_buffer, sizeof(s_date_iso_buffer), "%a %Y-%m-%d", tick_time);
    // Convert day abbreviation to uppercase
    for (int i = 0; i < 3; i++) {
      if (s_date_iso_buffer[i] >= 'a' && s_date_iso_buffer[i] <= 'z') {
        s_date_iso_buffer[i] -= 32;
      }
    }
  } else if (s_settings_date_format == 1) {
    // ISO + Weekday (2026-06-09 TUE)
    strftime(s_date_iso_buffer, sizeof(s_date_iso_buffer), "%Y-%m-%d %a", tick_time);
    // Convert day abbreviation to uppercase
    int len = strlen(s_date_iso_buffer);
    if (len >= 3) {
      for (int i = len - 3; i < len; i++) {
        if (s_date_iso_buffer[i] >= 'a' && s_date_iso_buffer[i] <= 'z') {
          s_date_iso_buffer[i] -= 32;
        }
      }
    }
  } else {
    // Full Text: TUE JUNE 9th, 2026
    char weekday_buf[8];
    char month_buf[16];
    char year_buf[8];
    int day = tick_time->tm_mday;
    
    strftime(weekday_buf, sizeof(weekday_buf), "%a", tick_time);
    strftime(month_buf, sizeof(month_buf), "%B", tick_time);
    strftime(year_buf, sizeof(year_buf), "%Y", tick_time);
    
    // Convert weekday to uppercase
    for (int i = 0; weekday_buf[i]; i++) {
      if (weekday_buf[i] >= 'a' && weekday_buf[i] <= 'z') weekday_buf[i] -= 32;
    }
    // Convert month to uppercase
    for (int i = 0; month_buf[i]; i++) {
      if (month_buf[i] >= 'a' && month_buf[i] <= 'z') month_buf[i] -= 32;
    }
    
    snprintf(s_date_iso_buffer, sizeof(s_date_iso_buffer), "%s %s %d%s, %s",
             weekday_buf, month_buf, day, get_ordinal_suffix(day), year_buf);
  }
  text_layer_set_text(s_date_iso_layer, s_date_iso_buffer);
  
  // Also refresh dynamic dates and sensors
  update_date_info();
  update_health_info();
  refresh_complications();
  layer_mark_dirty(s_canvas_layer);

  // Request weather update every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    request_weather();
  }
}

// -----------------------------------------------------------------------------
// Lifecycle & Handlers
// -----------------------------------------------------------------------------

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_canvas_layer);
  refresh_complications();
}

static void handle_bluetooth(bool connected) {
  s_connected = connected;
  if (!connected) {
    vibes_double_pulse();
  }
  layer_mark_dirty(s_canvas_layer);
  refresh_complications();
}

static void main_window_load(Window *window) {
  GRect bounds = layer_get_bounds(window_get_root_layer(window));

  // 1. Create Canvas Layer (sidebar & borders)
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);

  // 2. Center Time Layer (Digital Clock, scaled to LECO 60 non-bold, centered inside TIME box)
  s_time_layer = text_layer_create(GRect(12, 57, 176, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, s_active_theme->text_primary);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // 2b. ISO Date Layer (centered inside DATE box)
  s_date_iso_layer = text_layer_create(GRect(12, 149, 176, 22));
  text_layer_set_background_color(s_date_iso_layer, GColorClear);
  text_layer_set_text_color(s_date_iso_layer, s_active_theme->text_primary);
  text_layer_set_font(s_date_iso_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_iso_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_iso_layer));

  // 3. Create Parameterized Complications Text Layers
  for (int i = 0; i < NUM_SLOTS; i++) {
    ComplicationSlot *slot = &s_complication_slots[i];
    GRect text_rect = GRect(
      slot->box_rect.origin.x + 2,
      slot->box_rect.origin.y + (slot->box_rect.size.h - 20) / 2,
      slot->box_rect.size.w - 4,
      20
    );
    slot->layer = text_layer_create(text_rect);
    text_layer_set_background_color(slot->layer, GColorClear);
    text_layer_set_text_color(slot->layer, s_active_theme->text_primary);
    text_layer_set_font(slot->layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(slot->layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(slot->layer));
  }

  // Initialize Data
  battery_callback(battery_state_service_peek());
  handle_bluetooth(connection_service_peek_pebble_app_connection());
  update_time();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_iso_layer);

  for (int i = 0; i < NUM_SLOTS; i++) {
    text_layer_destroy(s_complication_slots[i].layer);
  }
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  update_health_info();
  refresh_complications();
  layer_mark_dirty(s_canvas_layer);
}
#endif

static void init(void) {
  // Load settings from persistent storage
  if (persist_exists(MESSAGE_KEY_SETTINGS_THEME)) {
    s_settings_theme = persist_read_int(MESSAGE_KEY_SETTINGS_THEME);
  }
  if (persist_exists(MESSAGE_KEY_SETTINGS_UNITS)) {
    s_settings_units = persist_read_int(MESSAGE_KEY_SETTINGS_UNITS);
  }
  if (persist_exists(MESSAGE_KEY_SETTINGS_DATE_FORMAT)) {
    s_settings_date_format = persist_read_int(MESSAGE_KEY_SETTINGS_DATE_FORMAT);
  }

  // Determine active theme initially
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  if (s_settings_theme == 1) {
    s_active_theme = &s_theme_day;
  } else if (s_settings_theme == 2) {
    s_active_theme = &s_theme_night;
  } else {
    // Auto (Day/Night)
    if (tick_time->tm_hour >= 6 && tick_time->tm_hour < 18) {
      s_active_theme = &s_theme_day;
    } else {
      s_active_theme = &s_theme_night;
    }
  }

  s_main_window = window_create();
  window_set_background_color(s_main_window, s_active_theme->center_bg);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  // Enable backlight constant on for emulator debugging (commented out for physical watch battery life)
  // light_enable(true);

  // Subscriptions
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif
  // accel_tap_service_subscribe(tap_handler);

  // AppMessage setup
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_open(128, 128);

  // Initial weather request
  request_weather();
}

static void deinit(void) {
  // light_enable(false);
  // accel_tap_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
