#include <unity.h>
#include <string.h>
#include "ns_restart_schedule.h"

void setUp(void) {}
void tearDown(void) {}

static RestartSchedule s;

void test_nores_never_fires(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "NORES", 3, 0));
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "NORES", 0, 0));
}

void test_empty_never_fires(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "", 3, 0));
}

void test_malformed_never_fires(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "banana", 3, 0));
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "25:00", 25, 0));
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "03:99", 3, 99));
}

void test_fires_at_configured_minute(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "03:00", 2, 59));
    TEST_ASSERT_TRUE(restartScheduleDue(&s, "03:00", 3, 0));
}

void test_fires_only_once_per_day(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_TRUE(restartScheduleDue(&s, "03:00", 3, 0));
    TEST_ASSERT_FALSE_MESSAGE(restartScheduleDue(&s, "03:00", 3, 0),
        "the latch must prevent a reboot loop within the same minute");
}

void test_rearms_after_leaving_the_minute(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_TRUE(restartScheduleDue(&s, "03:00", 3, 0));
    TEST_ASSERT_FALSE(restartScheduleDue(&s, "03:00", 3, 1));
    TEST_ASSERT_TRUE(restartScheduleDue(&s, "03:00", 3, 0));
}

void test_single_digit_hour_accepted(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_TRUE(restartScheduleDue(&s, "3:05", 3, 5));
}

void test_midnight(void) {
    restartScheduleInit(&s);
    TEST_ASSERT_TRUE(restartScheduleDue(&s, "00:00", 0, 0));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_nores_never_fires);
    RUN_TEST(test_empty_never_fires);
    RUN_TEST(test_malformed_never_fires);
    RUN_TEST(test_fires_at_configured_minute);
    RUN_TEST(test_fires_only_once_per_day);
    RUN_TEST(test_rearms_after_leaving_the_minute);
    RUN_TEST(test_single_digit_hour_accepted);
    RUN_TEST(test_midnight);
    return UNITY_END();
}
