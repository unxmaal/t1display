#include <unity.h>
#include <string.h>
#include <stdint.h>
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
    uint32_t nearMax = UINT32_MAX - MIN_MS(1);
    alarmRecordFired(&s, nearMax);
    uint32_t afterWrap = nearMax + MIN_MS(6);
    TEST_ASSERT_TRUE_MESSAGE(afterWrap < nearMax, "the test must actually wrap");
    TEST_ASSERT_TRUE_MESSAGE(alarmShouldFire(&s, afterWrap, ALARM_LEVEL_LOW_ALARM, 5),
        "a millis() rollover must not suppress alarms for 49 days");
}

void test_repeat_interval_holds_across_rollover(void) {
    alarmScheduleInit(&s);
    uint32_t nearMax = UINT32_MAX - 1000;
    alarmRecordFired(&s, nearMax);
    uint32_t oneMinLater = nearMax + MIN_MS(1);
    TEST_ASSERT_FALSE_MESSAGE(alarmShouldFire(&s, oneMinLater, ALARM_LEVEL_LOW_ALARM, 5),
        "one minute after a wrap is one minute, not 49 days");
}

void test_snooze_counts_down_across_rollover(void) {
    alarmScheduleInit(&s);
    uint32_t nearMax = UINT32_MAX - 1000;
    alarmScheduleSnooze(&s, nearMax, ALARM_LEVEL_LOW_ALARM, 30);
    uint32_t tenMinLater = nearMax + MIN_MS(10);
    TEST_ASSERT_UINT32_WITHIN(5, 20UL * 60UL, alarmSnoozeRemainingSec(&s, tenMinLater));
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
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_LOW_WARNING, 5));
}

void test_snoozing_low_alarm_does_not_silence_high_alarm(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    TEST_ASSERT_TRUE_MESSAGE(alarmShouldFire(&s, MIN_MS(2), ALARM_LEVEL_HIGH_ALARM, 5),
        "a snoozed hypo must not hide a hyper");
}

void test_snoozing_high_alarm_does_not_silence_low_warning(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_HIGH_ALARM, 30);
    TEST_ASSERT_TRUE(alarmShouldFire(&s, MIN_MS(2), ALARM_LEVEL_LOW_WARNING, 5));
}

void test_snoozing_warning_does_not_silence_loop_error(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_HIGH_WARNING, 30);
    TEST_ASSERT_TRUE(alarmShouldFire(&s, MIN_MS(5), ALARM_LEVEL_LOOP_ERROR, 5));
}

void test_snoozing_low_alarm_does_not_silence_no_readings(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    TEST_ASSERT_TRUE_MESSAGE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_NO_READINGS, 5),
        "losing the feed while a hypo is snoozed is a new problem");
}

void test_snoozing_loop_error_silences_no_readings(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOOP_ERROR, 30);
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(10), ALARM_LEVEL_NO_READINGS, 5));
}

void test_double_tap_does_not_double_the_snooze(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 30);
    alarmScheduleSnooze(&s, 5000, ALARM_LEVEL_LOW_ALARM, 30);
    TEST_ASSERT_UINT32_WITHIN_MESSAGE(10, 30UL * 60UL, alarmSnoozeRemainingSec(&s, 5000),
        "a fumbled double tap must count as one press");
}

void test_deliberate_second_press_adds_one_step(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 20);
    alarmScheduleSnooze(&s, MIN_MS(1), ALARM_LEVEL_LOW_ALARM, 20);
    TEST_ASSERT_UINT32_WITHIN(10, 39UL * 60UL, alarmSnoozeRemainingSec(&s, MIN_MS(1)));
}

void test_press_for_a_new_level_resnoozes_at_that_level(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_WARNING, 30);
    alarmScheduleSnooze(&s, 3000, ALARM_LEVEL_LOW_ALARM, 30);
    TEST_ASSERT_FALSE(alarmShouldFire(&s, MIN_MS(5), ALARM_LEVEL_LOW_ALARM, 5));
}

void test_snooze_pressed_with_no_alarm_masks_nothing(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_NORMAL, 30);
    TEST_ASSERT_TRUE(alarmShouldFire(&s, MIN_MS(1), ALARM_LEVEL_LOW_WARNING, 5));
}

void test_zero_timeout_still_snoozes(void) {
    alarmScheduleInit(&s);
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 0);
    TEST_ASSERT_TRUE_MESSAGE(alarmSnoozeRemainingSec(&s, 0) > 0,
        "the snooze button must never silently do nothing");
}

void test_loop_error_has_its_own_sound(void) {
    TEST_ASSERT_NOT_EQUAL(alarmSound(ALARM_LEVEL_HIGH_ALARM), alarmSound(ALARM_LEVEL_LOOP_ERROR));
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_NO_READINGS, alarmSound(ALARM_LEVEL_LOOP_ERROR));
}

void test_each_glucose_level_maps_to_its_sound(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_LOW_ALARM,    alarmSound(ALARM_LEVEL_LOW_ALARM));
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_HIGH_ALARM,   alarmSound(ALARM_LEVEL_HIGH_ALARM));
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_LOW_WARNING,  alarmSound(ALARM_LEVEL_LOW_WARNING));
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_HIGH_WARNING, alarmSound(ALARM_LEVEL_HIGH_WARNING));
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_NO_READINGS,  alarmSound(ALARM_LEVEL_NO_READINGS));
    TEST_ASSERT_EQUAL_INT(ALARM_SOUND_NONE,         alarmSound(ALARM_LEVEL_NORMAL));
}

void test_repeated_snooze_is_capped(void) {
    alarmScheduleInit(&s);
    for (int i = 0; i < 20; i++)
        alarmScheduleSnooze(&s, MIN_MS(i), ALARM_LEVEL_LOW_ALARM, 30);
    unsigned long rem = alarmSnoozeRemainingSec(&s, MIN_MS(19));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(ALARM_SNOOZE_MAX_SEC, rem,
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
    alarmScheduleSnooze(&s, 0, ALARM_LEVEL_LOW_ALARM, 20);
    alarmScheduleSnooze(&s, MIN_MS(1), ALARM_LEVEL_LOW_ALARM, 20);
    unsigned long stacked = alarmSnoozeRemainingSec(&s, MIN_MS(1));

    unsigned long later = MIN_MS(200);
    alarmScheduleSnooze(&s, later, ALARM_LEVEL_LOW_ALARM, 20);
    unsigned long fresh = alarmSnoozeRemainingSec(&s, later);

    TEST_ASSERT_TRUE(stacked > fresh);
    TEST_ASSERT_UINT32_WITHIN(60, 20UL * 60UL, fresh);
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
    RUN_TEST(test_repeat_interval_holds_across_rollover);
    RUN_TEST(test_snooze_counts_down_across_rollover);
    RUN_TEST(test_snooze_suppresses_same_level);
    RUN_TEST(test_snooze_expires);
    RUN_TEST(test_snoozing_no_readings_does_not_silence_hypo);
    RUN_TEST(test_snoozing_warning_does_not_silence_alarm);
    RUN_TEST(test_snoozing_alarm_silences_lesser_warning);
    RUN_TEST(test_snoozing_low_alarm_does_not_silence_high_alarm);
    RUN_TEST(test_snoozing_high_alarm_does_not_silence_low_warning);
    RUN_TEST(test_snoozing_warning_does_not_silence_loop_error);
    RUN_TEST(test_snoozing_low_alarm_does_not_silence_no_readings);
    RUN_TEST(test_snoozing_loop_error_silences_no_readings);
    RUN_TEST(test_double_tap_does_not_double_the_snooze);
    RUN_TEST(test_deliberate_second_press_adds_one_step);
    RUN_TEST(test_press_for_a_new_level_resnoozes_at_that_level);
    RUN_TEST(test_snooze_pressed_with_no_alarm_masks_nothing);
    RUN_TEST(test_zero_timeout_still_snoozes);
    RUN_TEST(test_loop_error_has_its_own_sound);
    RUN_TEST(test_each_glucose_level_maps_to_its_sound);
    RUN_TEST(test_repeated_snooze_is_capped);
    RUN_TEST(test_held_press_does_not_run_away);
    RUN_TEST(test_snooze_multiplier_resets_after_expiry);
    RUN_TEST(test_snooze_remaining_zero_when_not_snoozed);
    RUN_TEST(test_snooze_remaining_counts_down);
    RUN_TEST(test_severity_rank_ordering);
    RUN_TEST(test_normal_level_never_fires);
    return UNITY_END();
}
