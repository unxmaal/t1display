#include <unity.h>
#include "ns_error_log.h"

void setUp(void) {}
void tearDown(void) {}

static NsErrorLog elog;

void test_starts_empty(void) {
    nsErrorLogInit(&elog);
    TEST_ASSERT_EQUAL_INT(0, nsErrorLogHeld(&elog));
    TEST_ASSERT_EQUAL_INT(0, elog.consecutive);
    TEST_ASSERT_EQUAL_UINT32(0, elog.total);
}

void test_add_records_code_and_time(void) {
    nsErrorLogInit(&elog);
    nsErrorLogAdd(&elog, 1001, 5000);
    TEST_ASSERT_EQUAL_INT(1, nsErrorLogHeld(&elog));
    TEST_ASSERT_EQUAL_INT(1001, nsErrorLogCodeAt(&elog, 0));
    TEST_ASSERT_EQUAL_INT(5000, nsErrorLogTimeAt(&elog, 0));
}

void test_ring_buffer_wraps_and_drops_oldest(void) {
    nsErrorLogInit(&elog);
    for (int i = 0; i < NS_ERR_LOG_SIZE + 3; i++)
        nsErrorLogAdd(&elog, 2000 + i, 100 + i);
    TEST_ASSERT_EQUAL_INT(NS_ERR_LOG_SIZE, nsErrorLogHeld(&elog));
    TEST_ASSERT_EQUAL_INT(2003, nsErrorLogCodeAt(&elog, 0));
    TEST_ASSERT_EQUAL_INT(2000 + NS_ERR_LOG_SIZE + 2,
                          nsErrorLogCodeAt(&elog, NS_ERR_LOG_SIZE - 1));
}

void test_unknown_time_does_not_inherit_previous(void) {
    nsErrorLogInit(&elog);
    nsErrorLogAdd(&elog, 1001, 5000);
    nsErrorLogAdd(&elog, 1002, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nsErrorLogTimeAt(&elog, 1),
        "an entry logged without a clock must not carry a stale timestamp");
}

void test_consecutive_increments(void) {
    nsErrorLogInit(&elog);
    nsErrorLogAdd(&elog, 1001, 1);
    nsErrorLogAdd(&elog, 1001, 2);
    TEST_ASSERT_EQUAL_INT(2, elog.consecutive);
}

void test_success_resets_consecutive_but_keeps_history(void) {
    nsErrorLogInit(&elog);
    nsErrorLogAdd(&elog, 1001, 1);
    nsErrorLogAdd(&elog, 1001, 2);
    nsErrorLogSuccess(&elog);
    TEST_ASSERT_EQUAL_INT(0, elog.consecutive);
    TEST_ASSERT_EQUAL_INT(2, nsErrorLogHeld(&elog));
    TEST_ASSERT_EQUAL_UINT32(2, elog.total);
}

void test_restart_uses_consecutive_not_cumulative(void) {
    nsErrorLogInit(&elog);
    for (int i = 0; i < 20; i++) {
        nsErrorLogAdd(&elog, 1001, i);
        nsErrorLogSuccess(&elog);
    }
    TEST_ASSERT_FALSE_MESSAGE(nsErrorLogShouldRestart(&elog, 5),
        "twenty errors spread across successful fetches must not reboot");
}

void test_restart_fires_on_sustained_failure(void) {
    nsErrorLogInit(&elog);
    for (int i = 0; i < 5; i++)
        nsErrorLogAdd(&elog, 1001, i);
    TEST_ASSERT_TRUE(nsErrorLogShouldRestart(&elog, 5));
}

void test_restart_disabled_when_threshold_zero(void) {
    nsErrorLogInit(&elog);
    for (int i = 0; i < 50; i++)
        nsErrorLogAdd(&elog, 1001, i);
    TEST_ASSERT_FALSE(nsErrorLogShouldRestart(&elog, 0));
}

void test_badge_clears_after_success(void) {
    nsErrorLogInit(&elog);
    nsErrorLogAdd(&elog, 1001, 1);
    TEST_ASSERT_TRUE(nsErrorLogHasActiveFault(&elog));
    nsErrorLogSuccess(&elog);
    TEST_ASSERT_FALSE_MESSAGE(nsErrorLogHasActiveFault(&elog),
        "the fault badge must clear once a fetch succeeds");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_starts_empty);
    RUN_TEST(test_add_records_code_and_time);
    RUN_TEST(test_ring_buffer_wraps_and_drops_oldest);
    RUN_TEST(test_unknown_time_does_not_inherit_previous);
    RUN_TEST(test_consecutive_increments);
    RUN_TEST(test_success_resets_consecutive_but_keeps_history);
    RUN_TEST(test_restart_uses_consecutive_not_cumulative);
    RUN_TEST(test_restart_fires_on_sustained_failure);
    RUN_TEST(test_restart_disabled_when_threshold_zero);
    RUN_TEST(test_badge_clears_after_success);
    return UNITY_END();
}
