#include <unity.h>
#include <string.h>
#include <time.h>
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

static char buf[24];

void test_clock_24_hour(void) {
    formatClock(buf, sizeof(buf), 7, 5, TIME_FORMAT_24H);
    TEST_ASSERT_EQUAL_STRING("07:05", buf);
    formatClock(buf, sizeof(buf), 23, 59, TIME_FORMAT_24H);
    TEST_ASSERT_EQUAL_STRING("23:59", buf);
}

void test_clock_12_hour(void) {
    formatClock(buf, sizeof(buf), 7, 5, TIME_FORMAT_12H);
    TEST_ASSERT_EQUAL_STRING("7:05a", buf);
    formatClock(buf, sizeof(buf), 13, 7, TIME_FORMAT_12H);
    TEST_ASSERT_EQUAL_STRING("1:07p", buf);
}

void test_clock_12_hour_noon_and_midnight(void) {
    formatClock(buf, sizeof(buf), 0, 30, TIME_FORMAT_12H);
    TEST_ASSERT_EQUAL_STRING("12:30a", buf);
    formatClock(buf, sizeof(buf), 12, 0, TIME_FORMAT_12H);
    TEST_ASSERT_EQUAL_STRING("12:00p", buf);
}

static struct tm at(int mday, int mon, int hour, int min) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_mday = mday;
    t.tm_mon  = mon;
    t.tm_hour = hour;
    t.tm_min  = min;
    return t;
}

void test_log_date_day_first(void) {
    struct tm t = at(3, 3, 14, 5);
    formatLogDate(buf, sizeof(buf), &t, DATE_FORMAT_DAY_FIRST, TIME_FORMAT_24H);
    TEST_ASSERT_EQUAL_STRING("03.04 14:05", buf);
}

void test_log_date_month_first(void) {
    struct tm t = at(3, 3, 14, 5);
    formatLogDate(buf, sizeof(buf), &t, DATE_FORMAT_MONTH_FIRST, TIME_FORMAT_24H);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("04/03 14:05", buf,
        "a US reader must not see 3 April as 4 March");
}

void test_log_date_with_12_hour_time(void) {
    struct tm t = at(28, 11, 0, 7);
    formatLogDate(buf, sizeof(buf), &t, DATE_FORMAT_MONTH_FIRST, TIME_FORMAT_12H);
    TEST_ASSERT_EQUAL_STRING("12/28 12:07a", buf);
}

void test_log_date_without_clock(void) {
    formatLogDate(buf, sizeof(buf), NULL, DATE_FORMAT_DAY_FIRST, TIME_FORMAT_24H);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("no clock", buf,
        "an error logged before NTP must not read as 1970");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_clock_24_hour);
    RUN_TEST(test_clock_12_hour);
    RUN_TEST(test_clock_12_hour_noon_and_midnight);
    RUN_TEST(test_log_date_day_first);
    RUN_TEST(test_log_date_month_first);
    RUN_TEST(test_log_date_with_12_hour_time);
    RUN_TEST(test_log_date_without_clock);
    return UNITY_END();
}
