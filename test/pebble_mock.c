#include "pebble.h"
#include <stdarg.h>

// Mock Data
int32_t mock_persist_storage[256];
bool mock_persist_exists[256];

// Implementations
void app_event_loop(void) {}
void app_message_open(uint32_t size_inbound, uint32_t size_outbound) {}
void app_message_outbox_begin(DictionaryIterator **iterator) {}
void app_message_outbox_send(void) {}
void app_message_register_inbox_dropped(void (*callback)(AppMessageResult reason, void *context)) {}
void app_message_register_inbox_received(void (*callback)(DictionaryIterator *iterator, void *context)) {}

BatteryChargeState battery_state_service_peek(void) {
  BatteryChargeState state = { .charge_percent = 100, .is_charging = false, .is_plugged = false };
  return state;
}

void battery_state_service_subscribe(void (*handler)(BatteryChargeState charge)) {}
bool clock_is_24h_style(void) { return false; }
bool connection_service_peek_pebble_app_connection(void) { return true; }
void connection_service_subscribe(ConnectionHandlers handlers) {}

Tuple *dict_find(const DictionaryIterator *iter, uint32_t key) { return NULL; }
void dict_write_uint8(DictionaryIterator *iter, uint32_t key, uint8_t value) {}

GFont fonts_get_system_font(const char *font_key) { return NULL; }

void graphics_context_set_fill_color(GContext *ctx, GColor color) {}
void graphics_context_set_stroke_color(GContext *ctx, GColor color) {}
void graphics_context_set_stroke_width(GContext *ctx, uint8_t stroke_width) {}
void graphics_context_set_text_color(GContext *ctx, GColor color) {}
void graphics_draw_line(GContext *ctx, GPoint p0, GPoint p1) {}
void graphics_draw_text(GContext *ctx, const char *text, GFont font, GRect box, GTextOverflowMode overflow_mode, GTextAlignment alignment, GContext *layout_cache) {}
void graphics_fill_rect(GContext *ctx, GRect rect, uint16_t corner_radius, GCornerMask corner_mask) {}

void health_service_events_subscribe(void (*handler)(HealthEventType event, void *context), void *context) {}
void health_service_events_unsubscribe(void) {}
HealthServiceAccessibilityMask health_service_metric_accessible(HealthMetric metric, time_t time_start, time_t time_end) {
  return HealthServiceAccessibilityMaskAvailable;
}
HealthServiceAccessibilityMask health_service_metric_averaged_accessible(HealthMetric metric, time_t time_start, time_t time_end, HealthServiceTimeScope scope) {
  return HealthServiceAccessibilityMaskAvailable;
}
int32_t health_service_peek_current_value(HealthMetric metric) { return 0; }
int32_t health_service_sum_averaged(HealthMetric metric, time_t time_start, time_t time_end, HealthServiceTimeScope scope) { return 10000; }
int32_t health_service_sum_today(HealthMetric metric) { return 5000; }

void layer_add_child(Layer *parent, Layer *child) {}
Layer *layer_create(GRect frame) { return NULL; }
void layer_destroy(Layer *layer) {}
GRect layer_get_bounds(Layer *layer) { return GRect(0, 0, 144, 168); }
void layer_mark_dirty(Layer *layer) {}
void layer_set_hidden(Layer *layer, bool hidden) {}
void layer_set_update_proc(Layer *layer, void (*update_proc)(Layer *layer, GContext *ctx)) {}

bool persist_exists(const uint32_t key) { return mock_persist_exists[key % 256]; }
int32_t persist_read_int(const uint32_t key) { return mock_persist_storage[key % 256]; }
void persist_write_int(const uint32_t key, const int32_t value) {
  mock_persist_storage[key % 256] = value;
  mock_persist_exists[key % 256] = true;
}

TextLayer *text_layer_create(GRect frame) { return NULL; }
void text_layer_destroy(TextLayer *text_layer) {}
Layer *text_layer_get_layer(TextLayer *text_layer) { return NULL; }
void text_layer_set_background_color(TextLayer *text_layer, GColor color) {}
void text_layer_set_font(TextLayer *text_layer, GFont font) {}
void text_layer_set_text(TextLayer *text_layer, const char *text) {}
void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment) {}
void text_layer_set_text_color(TextLayer *text_layer, GColor color) {}

void tick_timer_service_subscribe(TimeUnits tick_units, void (*handler)(struct tm *tick_time, TimeUnits units_changed)) {}
time_t time_start_of_today(void) { return 0; }
void vibes_double_pulse(void) {}

Window *window_create(void) { return NULL; }
void window_destroy(Window *window) {}
Layer *window_get_root_layer(Window *window) { return NULL; }
void window_set_background_color(Window *window, GColor background_color) {}
void window_set_window_handlers(Window *window, WindowHandlers handlers) {}
void window_stack_push(Window *window, bool animated) {}

void APP_LOG(uint8_t level, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}
