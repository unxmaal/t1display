#include <unity.h>
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* ── alarmLevel ────────────────────────────────────────────────── */

/* Test thresholds, deliberately not the shipped defaults:
 *   snd_alarm=3.0 (low alarm)  snd_warning=4.0 (low warning)
 *   snd_alarm_high=14.0        snd_warning_high=10.0
 *   snd_no_readings=20 (minutes)
 */

static const float ALARM_LO   = 3.0f;
static const float WARN_LO    = 4.0f;
static const float WARN_HI    = 10.0f;
static const float ALARM_HI   = 14.0f;
static const unsigned int NO_READ_MIN = 20;

void test_alarmLevel_normal_glucose(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NORMAL,
        alarmLevel(7.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_low_alarm(void) {
    // 2.5 <= snd_alarm(3.0) and >= 0.1 → alarm
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_ALARM,
        alarmLevel(2.5f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_low_alarm_boundary(void) {
    // Exactly at snd_alarm threshold → alarm (<=)
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_ALARM,
        alarmLevel(3.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_low_warning(void) {
    // 3.5: above snd_alarm(3.0) but <= snd_warning(4.0) → warning
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_WARNING,
        alarmLevel(3.5f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_low_warning_boundary(void) {
    // Exactly at snd_warning threshold → warning (<=)
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_WARNING,
        alarmLevel(4.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_high_alarm(void) {
    // 15.0 >= snd_alarm_high(14.0) → alarm
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_HIGH_ALARM,
        alarmLevel(15.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_high_alarm_boundary(void) {
    // Exactly at snd_alarm_high → alarm (>=)
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_HIGH_ALARM,
        alarmLevel(14.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_high_warning(void) {
    // 12.0 >= snd_warning_high(10.0) but < snd_alarm_high(14.0) → warning
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_HIGH_WARNING,
        alarmLevel(12.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_high_warning_boundary(void) {
    // Exactly at snd_warning_high → warning (>=)
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_HIGH_WARNING,
        alarmLevel(10.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, false));
}

void test_alarmLevel_no_readings(void) {
    // Normal glucose but sensor is 25 minutes old (>= 20) → no readings warning
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NO_READINGS,
        alarmLevel(7.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   25, NO_READ_MIN, false));
}

void test_alarmLevel_no_readings_boundary(void) {
    // Exactly at snd_no_readings → warning (>=)
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NO_READINGS,
        alarmLevel(7.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   20, NO_READ_MIN, false));
}

void test_alarmLevel_no_readings_just_under(void) {
    // 19 minutes < 20 → normal
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NORMAL,
        alarmLevel(7.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   19, NO_READ_MIN, false));
}

void test_alarmLevel_loop_error(void) {
    // Normal glucose, fresh reading, but loop error → alarm
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOOP_ERROR,
        alarmLevel(7.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   5, NO_READ_MIN, true));
}

void test_alarmLevel_low_alarm_beats_no_readings(void) {
    // Low alarm takes priority over stale sensor — checked first in .ino
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_ALARM,
        alarmLevel(2.5f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   25, NO_READ_MIN, false));
}

void test_alarmLevel_zero_glucose_no_alarm(void) {
    // 0.0 glucose (no reading) should NOT trigger low alarm
    // The .ino checks sensSgv>=0.1
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NO_READINGS,
        alarmLevel(0.0f, ALARM_LO, WARN_LO, ALARM_HI, WARN_HI,
                   25, NO_READ_MIN, false));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_alarmLevel_normal_glucose);
    RUN_TEST(test_alarmLevel_low_alarm);
    RUN_TEST(test_alarmLevel_low_alarm_boundary);
    RUN_TEST(test_alarmLevel_low_warning);
    RUN_TEST(test_alarmLevel_low_warning_boundary);
    RUN_TEST(test_alarmLevel_high_alarm);
    RUN_TEST(test_alarmLevel_high_alarm_boundary);
    RUN_TEST(test_alarmLevel_high_warning);
    RUN_TEST(test_alarmLevel_high_warning_boundary);
    RUN_TEST(test_alarmLevel_no_readings);
    RUN_TEST(test_alarmLevel_no_readings_boundary);
    RUN_TEST(test_alarmLevel_no_readings_just_under);
    RUN_TEST(test_alarmLevel_loop_error);
    RUN_TEST(test_alarmLevel_low_alarm_beats_no_readings);
    RUN_TEST(test_alarmLevel_zero_glucose_no_alarm);
    return UNITY_END();
}
