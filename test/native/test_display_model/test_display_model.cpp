#include <unity.h>
#include <string.h>
#include "ns_display_model.h"
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* ── GlucosePageModel tests ────────────────────────────────────── */

void test_glucose_model_normal(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        7.5f, 135.0f, false,      // sgv mmol, mgdl, show_mgdl
        "Flat", 0,                 // direction, arrow_angle
        "+0.2",                    // delta_display
        14, 30,                    // hour, min
        1709312400, 1709312200,    // now, sensor_time (200s ago = 3 min)
        4.5f, 9.0f, 3.9f, 11.0f,  // thresholds
        ALARM_LEVEL_NORMAL, 0,     // alarm, snooze
        75, 0                      // battery, errors
    );

    TEST_ASSERT_EQUAL_STRING("14:30", m.time_str);
    TEST_ASSERT_EQUAL_STRING("7.5", m.glucose_str);
    TEST_ASSERT_EQUAL_INT(COLOR_GREEN, m.glucose_color);
    TEST_ASSERT_EQUAL_INT(FONT_SANS_BOLD_24, m.glucose_font);
    TEST_ASSERT_EQUAL_STRING("+0.2", m.delta_str);
    TEST_ASSERT_EQUAL_INT(0, m.arrow_angle);
    TEST_ASSERT_EQUAL_INT(COLOR_GREEN, m.arrow_color);
    TEST_ASSERT_FALSE(m.show_age);     // 3 min, not stale
    TEST_ASSERT_EQUAL_INT(75, m.battery_pct);
    TEST_ASSERT_FALSE(m.show_error_badge);
    TEST_ASSERT_FALSE(m.show_alarm_bar);
}

void test_glucose_model_high_yellow(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        10.5f, 189.0f, false,
        "SingleUp", -75, "+1.2",
        22, 15,
        1000, 900,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NORMAL, 0,
        50, 0
    );

    TEST_ASSERT_EQUAL_INT(COLOR_YELLOW, m.glucose_color);
    TEST_ASSERT_EQUAL_INT(-75, m.arrow_angle);
    TEST_ASSERT_EQUAL_INT(COLOR_YELLOW, m.arrow_color);
}

void test_glucose_model_low_red(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        3.2f, 57.6f, false,
        "DoubleDown", 90, "-0.5",
        3, 0,
        1000, 900,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_LOW_ALARM, 0,
        30, 2
    );

    TEST_ASSERT_EQUAL_INT(COLOR_RED, m.glucose_color);
    TEST_ASSERT_EQUAL_INT(90, m.arrow_angle);
    TEST_ASSERT_TRUE(m.show_alarm_bar);
    TEST_ASSERT_EQUAL_INT(COLOR_RED, m.alarm_bar_bg);
    TEST_ASSERT_TRUE(m.show_error_badge);  // 2 errors
}

void test_glucose_model_stale_sensor(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        7.5f, 135.0f, false,
        "Flat", 0, "+0.1",
        14, 30,
        1709312400, 1709311680,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NORMAL, 0,
        80, 0
    );

    TEST_ASSERT_TRUE(m.show_age);
    TEST_ASSERT_EQUAL_STRING("12 min", m.age_str);
    TEST_ASSERT_EQUAL_INT(COLOR_WHITE, m.age_color);
}

void test_glucose_model_very_stale_sensor(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        7.5f, 135.0f, false,
        "Flat", 0, "+0.1",
        14, 30,
        1709312400, 1709311200,  // 1200s = 20 min ago
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NO_READINGS, 0,
        80, 0
    );

    TEST_ASSERT_TRUE(m.show_age);
    TEST_ASSERT_EQUAL_STRING("20 min", m.age_str);
    TEST_ASSERT_EQUAL_INT(COLOR_RED, m.age_color);  // >15 min = red
}

void test_glucose_model_mgdl_mode(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        7.5f, 135.0f, true,    // show_mgdl = true
        "Flat", 0, "+5",
        12, 0,
        1000, 1000,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NORMAL, 0,
        -1, 0
    );

    TEST_ASSERT_EQUAL_STRING("135", m.glucose_str);
    TEST_ASSERT_EQUAL_INT(FONT_SANS_BOLD_24, m.glucose_font);
}

void test_glucose_model_snooze_countdown(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        3.5f, 63.0f, false,
        "SingleDown", 75, "-0.3",
        1, 0,
        1000, 900,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_LOW_WARNING, 300,  // 300s = 5 min snooze
        60, 0
    );

    TEST_ASSERT_TRUE(m.show_alarm_bar);
    TEST_ASSERT_EQUAL_INT(COLOR_YELLOW, m.alarm_bar_bg);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("SNOOZED 5 min", m.alarm_bar_text,
        "a bare number in the alarm bar does not say what it counts");
}

void test_glucose_model_double_digit_mmol(void) {
    GlucosePageModel m;
    buildGlucoseModel(&m,
        15.3f, 275.4f, false,
        "DoubleUp", -90, "+2.1",
        12, 0,
        1000, 1000,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_HIGH_ALARM, 0,
        50, 0
    );

    TEST_ASSERT_EQUAL_STRING("15.3", m.glucose_str);
    TEST_ASSERT_EQUAL_INT(FONT_SANS_BOLD_18, m.glucose_font);  // smaller font for 4 chars
}

/* ── StatusPageModel tests ─────────────────────────────────────── */

static void buildDir(GlucosePageModel *m, const char *dir, long age_sec) {
    buildGlucoseModel(m,
        7.5f, 135.0f, false,
        dir, directionToAngle(dir), "+0.1",
        14, 30,
        1709312400 + age_sec, 1709312400,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NORMAL, 0,
        80, 0);
}

void test_glucose_model_triple_trend_draws_double_arrow(void) {
    GlucosePageModel m;
    buildDir(&m, "TripleUp", 0);
    TEST_ASSERT_EQUAL_INT(ARROW_DOUBLE, m.arrow_style);
    TEST_ASSERT_EQUAL_INT(-90, m.arrow_angle);
}

void test_glucose_model_rate_out_of_range_has_its_own_glyph(void) {
    GlucosePageModel m;
    buildDir(&m, "RATE OUT OF RANGE", 0);
    TEST_ASSERT_EQUAL_INT(ARROW_RATE_OUT_OF_RANGE, m.arrow_style);
}

void test_glucose_model_ordinary_trend_is_single(void) {
    GlucosePageModel m;
    buildDir(&m, "Flat", 0);
    TEST_ASSERT_EQUAL_INT(ARROW_SINGLE, m.arrow_style);
}

void test_glucose_model_no_data_hides_arrow(void) {
    GlucosePageModel m;
    buildDir(&m, "TripleUp", 40 * 60);
    TEST_ASSERT_EQUAL_INT(ARROW_NONE, m.arrow_style);
}

static void buildLevel(GlucosePageModel *m, int level, int snooze_sec) {
    buildGlucoseModel(m,
        6.0f, 108.0f, false,
        "Flat", 0, "+0.1",
        12, 0,
        1000, 1000,
        4.5f, 9.0f, 3.9f, 11.0f,
        level, snooze_sec,
        60, 1);
}

void test_active_alarm_bar_says_how_to_silence_it(void) {
    GlucosePageModel m;
    buildLevel(&m, ALARM_LEVEL_LOW_ALARM, 0);
    TEST_ASSERT_TRUE(m.show_alarm_bar);
    TEST_ASSERT_EQUAL_STRING("TAP TO SNOOZE", m.alarm_bar_text);
}

void test_touch_labels_shown_when_no_alarm(void) {
    GlucosePageModel m;
    buildLevel(&m, ALARM_LEVEL_NORMAL, 0);
    TEST_ASSERT_FALSE(m.show_alarm_bar);
    TEST_ASSERT_TRUE_MESSAGE(m.show_touch_labels,
        "the bottom band is three buttons; without labels nothing says so");
}

void test_touch_labels_give_way_to_the_alarm_bar(void) {
    GlucosePageModel m;
    buildLevel(&m, ALARM_LEVEL_HIGH_WARNING, 0);
    TEST_ASSERT_FALSE(m.show_touch_labels);
    buildLevel(&m, ALARM_LEVEL_NORMAL, 120);
    TEST_ASSERT_FALSE(m.show_touch_labels);
}

void test_alarm_bar_leaves_room_for_badge_and_battery(void) {
    TEST_ASSERT_TRUE_MESSAGE(LAYOUT_BADGE_X + LAYOUT_BADGE_W <= LAYOUT_ALARM_BAR_X,
        "the error badge must stay visible during an alarm");
    TEST_ASSERT_TRUE_MESSAGE(LAYOUT_ALARM_BAR_X + LAYOUT_ALARM_BAR_W <= LAYOUT_BATTERY_X,
        "the battery must stay visible during an alarm");
    TEST_ASSERT_TRUE(LAYOUT_BATTERY_X + LAYOUT_BATTERY_W <= LAYOUT_SCREEN_W);
}

void test_labels_sit_in_their_touch_zones(void) {
    TEST_ASSERT_TRUE(LAYOUT_LABEL_LEFT_X < LAYOUT_SCREEN_W / 3);
    TEST_ASSERT_TRUE(LAYOUT_LABEL_RIGHT_X > 2 * LAYOUT_SCREEN_W / 3);
    TEST_ASSERT_TRUE(LAYOUT_LABEL_RIGHT_X < LAYOUT_BATTERY_X);
}

void test_status_model_no_errors(void) {
    StatusPageModel m;
    buildStatusModel(&m,
        NULL, NULL, 0, 0,
        250000, 86400000UL,
        "192.168.1.42", "CoreS3",
        90
    );

    TEST_ASSERT_EQUAL_INT(0, m.error_count);
    TEST_ASSERT_EQUAL_INT(0, m.display_count);
    TEST_ASSERT_EQUAL_STRING("Free Heap = 250000", m.heap_str);
    TEST_ASSERT_EQUAL_STRING("Up time = 01d 00:00:00", m.uptime_str);
    TEST_ASSERT_EQUAL_STRING("192.168.1.42", m.ip_str);
    TEST_ASSERT_EQUAL_STRING("CoreS3", m.version_str);
    TEST_ASSERT_EQUAL_INT(90, m.battery_pct);
}

void test_status_model_with_errors(void) {
    int codes[] = {1001, -11, 404};
    char dates[][16] = {"01.03.14:00", "01.03.14:05", "01.03.14:10"};
    StatusPageModel m;
    buildStatusModel(&m,
        codes, dates, 3, 5,
        200000, 3600000UL,
        "10.0.0.1", "v2",
        45
    );

    TEST_ASSERT_EQUAL_INT(5, m.error_count);
    TEST_ASSERT_EQUAL_INT(3, m.display_count);

    TEST_ASSERT_EQUAL_STRING("01.03.14:00", m.errors[0].date_str);
    TEST_ASSERT_EQUAL_STRING("JSON parse failed", m.errors[0].desc_str);
    TEST_ASSERT_EQUAL_INT(COLOR_YELLOW, m.errors[0].color);

    TEST_ASSERT_EQUAL_STRING("01.03.14:05", m.errors[1].date_str);
    TEST_ASSERT_EQUAL_STRING("HTTP err -11", m.errors[1].desc_str);
    TEST_ASSERT_EQUAL_INT(COLOR_RED, m.errors[1].color);  // negative = red

    TEST_ASSERT_EQUAL_STRING("HTTP 404", m.errors[2].desc_str);
    TEST_ASSERT_EQUAL_INT(COLOR_YELLOW, m.errors[2].color);
}

void test_status_model_null_errors_no_crash(void) {
    StatusPageModel m;
    buildStatusModel(&m,
        NULL, NULL, 5, 10,  // NULL arrays but display_count > 0
        200000, 3600000UL,
        "10.0.0.1", "v2",
        50
    );
    TEST_ASSERT_EQUAL_INT(10, m.error_count);
    TEST_ASSERT_EQUAL_INT(0, m.display_count);  // clamped to 0
}

void test_status_model_uptime_formatting(void) {
    StatusPageModel m;
    // 2 days 13 hours 45 min 30 sec
    unsigned long ms = (2UL * 86400 + 13 * 3600 + 45 * 60 + 30) * 1000UL;
    buildStatusModel(&m,
        NULL, NULL, 0, 0,
        100000, ms,
        "0.0.0.0", "test",
        -1
    );
    TEST_ASSERT_EQUAL_STRING("Up time = 02d 13:45:30", m.uptime_str);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    /* Glucose model */
    RUN_TEST(test_glucose_model_normal);
    RUN_TEST(test_glucose_model_high_yellow);
    RUN_TEST(test_glucose_model_low_red);
    RUN_TEST(test_glucose_model_stale_sensor);
    RUN_TEST(test_glucose_model_very_stale_sensor);
    RUN_TEST(test_glucose_model_mgdl_mode);
    RUN_TEST(test_glucose_model_snooze_countdown);
    RUN_TEST(test_glucose_model_double_digit_mmol);

    /* Status model */
    RUN_TEST(test_glucose_model_triple_trend_draws_double_arrow);
    RUN_TEST(test_glucose_model_rate_out_of_range_has_its_own_glyph);
    RUN_TEST(test_glucose_model_ordinary_trend_is_single);
    RUN_TEST(test_glucose_model_no_data_hides_arrow);
    RUN_TEST(test_active_alarm_bar_says_how_to_silence_it);
    RUN_TEST(test_touch_labels_shown_when_no_alarm);
    RUN_TEST(test_touch_labels_give_way_to_the_alarm_bar);
    RUN_TEST(test_alarm_bar_leaves_room_for_badge_and_battery);
    RUN_TEST(test_labels_sit_in_their_touch_zones);
    RUN_TEST(test_status_model_no_errors);
    RUN_TEST(test_status_model_with_errors);
    RUN_TEST(test_status_model_null_errors_no_crash);
    RUN_TEST(test_status_model_uptime_formatting);

    return UNITY_END();
}
