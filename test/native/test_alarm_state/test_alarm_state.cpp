#include <unity.h>
#include <string.h>
#include "ns_alarm_state.h"
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

#define MIN_MS(m) ((unsigned long)(m) * 60UL * 1000UL)

static AlarmSchedule s;

void test_first_alarm_fires_immediately(void) {
    alarmScheduleInit(&s);
    TEST_ASSERT_TRUE(alarmShouldFire(&s, 0, ALARM_LEVEL_LOW_ALARM, 5));
}

void test_repeat_suppressed_until_interval_elapses(void) {
    alarmScheduleInit(&s);
    alarmRecordFired(&s, MIN_MS(0));
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(4), ALARM_LEVEL_LOW_ALARM, 5));
    TEST_ASSERT_TRUE(alarmShouldFire(&s, MIN_MS(6), ALARM_LEVEL_LOW_ALARM, 5));
}

void test_timing_does_not_need_a_wall_clock(void) {
    alarmScheduleInit(&s);
    TEST_ASSERT_TRUE_MESSAGE(alarmShouldFire(&s, 0, ALARM_LEVEL_LOW_ALARM, 5),
        "alarms must fire on a device that never synced NTP");
}

void test_millis_rollover_does_not_block_alarms(void) {
    alarmScheduleInit(&s);
    unsigned long nearMax = 0xFFFFFFFFUL - MIN_MS(1);
    alarmRecordFired(&s, nearMax);
    unsigned long afterWrap = nearMax + MIN_MS(6);
    TEST_ASSERT_TRUE_MESSAGE(alarmShouldFire(&s, afterWrap, ALARM_LEVEL_LOW_ALARM, 5),
        "a millis() rollover must not suppress alarms for 49 days");
}

void test_snooze_suppresses_same_level(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_WARNING, 30);
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_LOW_WARNING, 5));
}

void test_snooze_expires(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_WARNING, 30);
    TEST_ASSERT_TRUE(alarmShouldFire(&s, MIN_MS(31), ALARM_LEVEL_LOW_WARNING, 5));
}

void test_snoozing_no_readings_does_not_silence_hypo(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_NO_READINGS, 30);
    TEST_ASSERT_TRUE_MESSAGE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_LOW_ALARM, 5),
        "a more severe condition must break through an existing snooze");
}

void test_snoozing_warning_does_not_silence_alarm(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_WARNING, 30);
    TEST_ASSERT_TRUE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_LOW_ALARM, 5));
}

void test_snoozing_alarm_silences_lesser_warning(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_NO_READINGS, 5));
}

void test_repeated_snooze_is_capped(void) {
    alarmScheduleInit(&s);
    for (int i = 0; i < 20; i++)
        alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    unsigned long rem = alarmSnoozeRemainingSec(&s, 0);
    TEST_ASSERT_TRUE_MESSAGE(rem <= ALARM_SNOOZE_MAX_SEC,
        "repeated snooze presses must not accumulate without bound");
}

void test_held_press_does_not_run_away(void) {
    alarmScheduleInit(&s);
    for (int i = 0; i < 10; i++)
        alarmScheduleSnooze(&s, (unsigned long)i * 100UL, ALARM_LEVEL_LOW_ALARM, 30);
    unsigned long rem = alarmSnoozeRemainingSec(&s, 1000);
    TEST_ASSERT_TRUE_MESSAGE(rem <= ALARM_SNOOZE_MAX_SEC,
        "a one-second finger hold must not silence alarms for hours");
}

void test_snooze_multiplier_resets_after_expiry(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    unsigned long stacked = alarmSnoozeRemainingSec(&s, 0);

    unsigned long later = MIN_MS(200);
    alarmScheduleSnooze(&s, later, ALARM_LEVEL_LOW_ALARM, 30);
    unsigned long fresh = alarmSnoozeRemainingSec(&s, later);

    TEST_ASSERT_TRUE(stacked > fresh);
    TEST_ASSERT_UINT32_WITHIN(60, 30UL * 60UL, fresh);
}

void test_snooze_remaining_zero_when_not_snoozed(void) {
    alarmScheduleInit(&s);
    TEST_ASSERT_EQUAL_UINT32(0, alarmSnoozeRemainingSec(&s, MIN_MS(5)));
}

void test_snooze_remaining_counts_down(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    unsigned long at0 = alarmSnoozeRemainingSec(&s, 0);
    unsigned long at10 = alarmSnoozeRemainingSec(&s, MIN_MS(10));
    TEST_ASSERT_TRUE(at10 < at0);
    TEST_ASSERT_UINT32_WITHIN(60, 20UL * 60UL, at10);
}

void test_severity_rank_ordering(void) {
    TEST_ASSERT_TRUE(alarmSeverityRank(ALARM_LEVEL_LOW_ALARM) >
                     alarmSeverityRank(ALARM_LEVEL_LOW_WARNING));
    TEST_ASSERT_TRUE(alarmSeverityRank(ALARM_LEVEL_HIGH_ALARM) >
                     alarmSeverityRank(ALARM_LEVEL_HIGH_WARNING));
    TEST_ASSERT_TRUE(alarmSeverityRank(ALARM_LEVEL_LOW_WARNING) >
                     alarmSeverityRank(ALARM_LEVEL_NO_READINGS));
    TEST_ASSERT_EQUAL_INT(0, alarmSeverityRank(ALARM_LEVEL_NORMAL));
}

void test_normal_level_never_fires(void) {
    alarmScheduleInit(&s);
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(100), ALARM_LEVEL_NORMAL, 5));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_first_alarm_fires_immediately);
    RUN_TEST(test_repeat_suppressed_until_interval_elapses);
    RUN_TEST(test_timing_does_not_need_a_wall_clock);
    RUN_TEST(test_millis_rollover_does_not_block_alarms);
    RUN_TEST(test_snooze_suppresses_same_level);
    RUN_TEST(test_snooze_expires);
    RUN_TEST(test_snoozing_no_readings_does_not_silence_hypo);
    RUN_TEST(test_snoozing_warning_does_not_silence_alarm);
    RUN_TEST(test_snoozing_alarm_silences_lesser_warning);
    RUN_TEST(test_repeated_snooze_is_capped);
    RUN_TEST(test_held_press_does_not_run_away);
    RUN_TEST(test_snooze_multiplier_resets_after_expiry);
    RUN_TEST(test_snooze_remaining_zero_when_not_snoozed);
    RUN_TEST(test_snooze_remaining_counts_down);
    RUN_TEST(test_severity_rank_ordering);
    RUN_TEST(test_normal_level_never_fires);
    return UNITY_END();
}
