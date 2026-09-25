#include <unity.h>
#include "ns_runtime.h"

void setUp(void) {}
void tearDown(void) {}

void test_fetches_between_watchdog_resets_fit_the_timeout(void) {
    TEST_ASSERT_TRUE_MESSAGE(NS_FETCH_WINDOW_MS < NS_WDT_TIMEOUT_SEC * 1000UL / 2,
        "the slowest fetch window must leave the watchdog half its budget");
}

void test_services_start_once_wifi_is_up(void) {
    ServicesLatch l;
    servicesLatchInit(&l);
    TEST_ASSERT_FALSE(servicesDue(&l, false));
    TEST_ASSERT_TRUE_MESSAGE(servicesDue(&l, true),
        "Wi-Fi arriving after boot must still start the web config and OTA");
    TEST_ASSERT_FALSE(servicesDue(&l, true));
}

void test_services_do_not_restart_after_a_drop(void) {
    ServicesLatch l;
    servicesLatchInit(&l);
    TEST_ASSERT_TRUE(servicesDue(&l, true));
    TEST_ASSERT_FALSE(servicesDue(&l, false));
    TEST_ASSERT_FALSE(servicesDue(&l, true));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_fetches_between_watchdog_resets_fit_the_timeout);
    RUN_TEST(test_services_start_once_wifi_is_up);
    RUN_TEST(test_services_do_not_restart_after_a_drop);
    return UNITY_END();
}
