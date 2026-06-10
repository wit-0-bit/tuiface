#define TEST_ENV

#include "unity/src/unity.h"
#include "pebble.h"

// Include the implementation file directly so we can test its static functions
#include "../src/c/watchface.c"

// Forward declarations of static functions we want to test
static void to_upper_str(char *str);
static int tuple_get_int(Tuple *tuple);
static const char* get_source_label(ComplicationDataSource source);
static void get_source_data(ComplicationDataSource source, char *val_buf, int val_len, int *percent);
static void apply_theme(void);

void setUp(void) {
  // Reset any global states if needed before each test
  s_settings_theme = 0; // Auto
  s_settings_units = 0; // Imperial
  s_battery_level = 100;
  s_step_count = -1;
  s_sleep_seconds = -1;
  s_heart_rate = 0;
  s_weather_temp = -999;
  strcpy(s_weather_cond, "--");
  s_connected = true;
}

void tearDown(void) {
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

void test_to_upper_str_should_convert_lowercase_to_uppercase(void) {
  char str1[] = "hello 123";
  to_upper_str(str1);
  TEST_ASSERT_EQUAL_STRING("HELLO 123", str1);

  char str2[] = "Mon";
  to_upper_str(str2);
  TEST_ASSERT_EQUAL_STRING("MON", str2);
  
  char str3[] = "ALREADY_UPPER";
  to_upper_str(str3);
  TEST_ASSERT_EQUAL_STRING("ALREADY_UPPER", str3);
}

void test_tuple_get_int_should_parse_strings_and_ints(void) {
  // We mock a tuple since we know its memory layout
  uint8_t buffer1[sizeof(Tuple) + 8];
  Tuple *t1 = (Tuple*)buffer1;
  t1->type = TUPLE_CSTRING;
  strcpy(t1->value->cstring, "42");
  TEST_ASSERT_EQUAL_INT(42, tuple_get_int(t1));

  uint8_t buffer2[sizeof(Tuple) + 4];
  Tuple *t2 = (Tuple*)buffer2;
  t2->type = TUPLE_INT;
  t2->value->int32 = 1234;
  TEST_ASSERT_EQUAL_INT(1234, tuple_get_int(t2));

  TEST_ASSERT_EQUAL_INT(0, tuple_get_int(NULL));
}

void test_get_source_label_should_return_correct_labels(void) {
  TEST_ASSERT_EQUAL_STRING("BATT", get_source_label(DATA_SOURCE_BATTERY));
  TEST_ASSERT_EQUAL_STRING("STEP", get_source_label(DATA_SOURCE_STEPS));
  TEST_ASSERT_EQUAL_STRING("WEATHER", get_source_label(DATA_SOURCE_WEATHER));
  TEST_ASSERT_EQUAL_STRING("", get_source_label(DATA_SOURCE_EMPTY));
}

void test_get_source_data_should_format_battery(void) {
  char buf[16];
  int percent = 0;
  
  s_battery_level = 85;
  get_source_data(DATA_SOURCE_BATTERY, buf, sizeof(buf), &percent);
  TEST_ASSERT_EQUAL_STRING("85%", buf);
  TEST_ASSERT_EQUAL_INT(85, percent);
}

void test_get_source_data_should_format_steps(void) {
  char buf[16];
  int percent = 0;
  
  s_step_goal = 10000;

  // No data
  s_step_count = -1;
  get_source_data(DATA_SOURCE_STEPS, buf, sizeof(buf), &percent);
  TEST_ASSERT_EQUAL_STRING("--", buf);
  TEST_ASSERT_EQUAL_INT(0, percent);

  // Normal steps
  s_step_count = 5000;
  get_source_data(DATA_SOURCE_STEPS, buf, sizeof(buf), &percent);
  TEST_ASSERT_EQUAL_STRING("5000", buf);
  TEST_ASSERT_EQUAL_INT(50, percent);

  // > 10k steps format
  s_step_count = 12500;
  get_source_data(DATA_SOURCE_STEPS, buf, sizeof(buf), &percent);
  TEST_ASSERT_EQUAL_STRING("12.5k", buf);
  TEST_ASSERT_EQUAL_INT(100, percent); // capped at 100%
}

void test_get_source_data_should_format_weather(void) {
  char buf[32];
  
  // No data
  s_weather_temp = -999;
  strcpy(s_weather_cond, "--");
  get_source_data(DATA_SOURCE_WEATHER, buf, sizeof(buf), NULL);
  TEST_ASSERT_EQUAL_STRING("-- / --", buf);

  // Imperial
  s_settings_units = 0;
  s_weather_temp = 72;
  strcpy(s_weather_cond, "SUN");
  get_source_data(DATA_SOURCE_WEATHER, buf, sizeof(buf), NULL);
  TEST_ASSERT_EQUAL_STRING("SUN / 72F", buf);

  // Metric
  s_settings_units = 1;
  s_weather_temp = 22;
  strcpy(s_weather_cond, "CLD");
  get_source_data(DATA_SOURCE_WEATHER, buf, sizeof(buf), NULL);
  TEST_ASSERT_EQUAL_STRING("CLD / 22C", buf);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_to_upper_str_should_convert_lowercase_to_uppercase);
  RUN_TEST(test_tuple_get_int_should_parse_strings_and_ints);
  RUN_TEST(test_get_source_label_should_return_correct_labels);
  RUN_TEST(test_get_source_data_should_format_battery);
  RUN_TEST(test_get_source_data_should_format_steps);
  RUN_TEST(test_get_source_data_should_format_weather);
  return UNITY_END();
}
