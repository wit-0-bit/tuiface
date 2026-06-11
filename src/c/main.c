#include <pebble.h>
#include "main.h"
#include "data.h"
#include "theme.h"
#include "drawing.h"
#include "messaging.h"

// -----------------------------------------------------------------------------
// Data Updaters
// -----------------------------------------------------------------------------

static void update_date_info() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  s_date_day = tick_time->tm_mday;
}

static void update_health_info() {
#if defined(PBL_HEALTH)
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask step_mask = health_service_metric_accessible(HealthMetricStepCount, start, end);
  if (step_mask & HealthServiceAccessibilityMaskAvailable) {
    s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
    s_step_goal = 10000;
  } else {
    s_step_count = -1;
  }

  HealthServiceAccessibilityMask sleep_mask = health_service_metric_accessible(HealthMetricSleepSeconds, start, end);
  if (sleep_mask & HealthServiceAccessibilityMaskAvailable) {
    s_sleep_seconds = (int)health_service_sum_today(HealthMetricSleepSeconds);
  } else {
    s_sleep_seconds = -1;
  }
  
  HealthServiceAccessibilityMask active_mask = health_service_metric_accessible(HealthMetricActiveSeconds, start, end);
  if (active_mask & HealthServiceAccessibilityMaskAvailable) {
    s_active_minutes = (int)(health_service_sum_today(HealthMetricActiveSeconds) / 60);
  } else {
    s_active_minutes = 0;
  }

  // Heart rate accessibility must be checked at an instant, not over a day
  // range — the range form reports the metric unavailable and BPM never shows.
  HealthServiceAccessibilityMask hr_mask = health_service_metric_accessible(HealthMetricHeartRateBPM, end, end);
  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
    s_heart_rate = (int)hr;
  } else {
    s_heart_rate = 0;
  }
#else
  s_step_count = -1;
  s_sleep_seconds = -1;
  s_heart_rate = 0;
  s_active_minutes = 0;
#endif
}

void update_time() {
  apply_theme();

  // The theme can flip while the face is open (Auto mode at 06:00/18:00, or a
  // settings push); the time/date layers keep their load-time color unless
  // re-applied here.
  if (s_time_layer) text_layer_set_text_color(s_time_layer, s_active_theme->text_primary);
  if (s_date_iso_layer) text_layer_set_text_color(s_date_iso_layer, s_active_theme->text_primary);

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  char *time_str = s_time_buffer;
  if (!clock_is_24h_style() && s_time_buffer[0] == '0') {
    time_str++; // strip leading zero
  }
  text_layer_set_text(s_time_layer, time_str);
  
  static char s_date_iso_buffer[64];
  format_date_string(s_settings_date_format, tick_time, s_date_iso_buffer, sizeof(s_date_iso_buffer));
  text_layer_set_text(s_date_iso_layer, s_date_iso_buffer);
  
  update_date_info();
  update_health_info();
  refresh_complications();
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);

  // Request weather update every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    request_weather();
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  refresh_complications();
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void handle_bluetooth(bool connected) {
  s_connected = connected;
  refresh_complications();
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
  if (!connected) {
    vibes_double_pulse();
  }
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate || event == HealthEventHeartRateUpdate || event == HealthEventSleepUpdate) {
    update_health_info();
    refresh_complications();
    if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
  }
}
#endif

// -----------------------------------------------------------------------------
// Window Management
// -----------------------------------------------------------------------------

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  s_time_layer = text_layer_create(GRect(12, 57, 176, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, s_active_theme->text_primary);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_date_iso_layer = text_layer_create(GRect(12, 149, 176, 22));
  text_layer_set_background_color(s_date_iso_layer, GColorClear);
  text_layer_set_text_color(s_date_iso_layer, s_active_theme->text_primary);
  text_layer_set_text_alignment(s_date_iso_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_iso_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_date_iso_layer));

  // Init text layers for slots
  for (int i = 0; i < NUM_SLOTS; i++) {
    ComplicationSlot *slot = &s_complication_slots[i];
    GRect text_rect = GRect(slot->box_rect.origin.x, slot->box_rect.origin.y + 11, slot->box_rect.size.w, 20);
    slot->layer = text_layer_create(text_rect);
    text_layer_set_background_color(slot->layer, GColorClear);
    text_layer_set_text_color(slot->layer, s_active_theme->text_primary);
    text_layer_set_text_alignment(slot->layer, GTextAlignmentCenter);
    text_layer_set_font(slot->layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(slot->layer));
  }
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_iso_layer);
  layer_destroy(s_canvas_layer);
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (s_complication_slots[i].layer) {
      text_layer_destroy(s_complication_slots[i].layer);
    }
  }
}

static void init(void) {
  // Load settings from persistent storage
  if (persist_exists(MESSAGE_KEY_SETTINGS_THEME)) s_settings_theme = persist_read_int(MESSAGE_KEY_SETTINGS_THEME);
  if (persist_exists(MESSAGE_KEY_SETTINGS_UNITS)) s_settings_units = persist_read_int(MESSAGE_KEY_SETTINGS_UNITS);
  if (persist_exists(MESSAGE_KEY_SETTINGS_DATE_FORMAT)) s_settings_date_format = persist_read_int(MESSAGE_KEY_SETTINGS_DATE_FORMAT);
  if (persist_exists(MESSAGE_KEY_SLOT_1)) s_complication_slots[0].source = persist_read_int(MESSAGE_KEY_SLOT_1);
  if (persist_exists(MESSAGE_KEY_SLOT_2)) s_complication_slots[1].source = persist_read_int(MESSAGE_KEY_SLOT_2);
  if (persist_exists(MESSAGE_KEY_SLOT_3)) s_complication_slots[2].source = persist_read_int(MESSAGE_KEY_SLOT_3);
  if (persist_exists(MESSAGE_KEY_SLOT_4)) s_complication_slots[3].source = persist_read_int(MESSAGE_KEY_SLOT_4);
  if (persist_exists(MESSAGE_KEY_SLOT_5)) s_complication_slots[4].source = persist_read_int(MESSAGE_KEY_SLOT_5);

  s_main_window = window_create();
  apply_theme();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  // Subscriptions
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif

  // Initial states
  battery_callback(battery_state_service_peek());
  handle_bluetooth(connection_service_peek_pebble_app_connection());

  // AppMessage setup
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_open(512, 512);

  // Restore cached weather; only hit the network if the cache is stale
  if (!load_weather_cache()) {
    request_weather();
  }
  update_time();
}

static void deinit(void) {
  window_destroy(s_main_window);
}

#ifndef TEST_ENV
int main(void) {
  init();
  app_event_loop();
  deinit();
}
#endif
