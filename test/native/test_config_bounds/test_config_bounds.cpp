#include <unity.h>
#include <string.h>
#include <stdlib.h>
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

static void parseExactAlloc(const char *src, ParsedConfig *cfg) {
    size_t len = strlen(src);
    char *heap = (char *)malloc(len);
    memcpy(heap, src, len);
    configDefaults(cfg);
    parseConfigBuffer(heap, len, cfg);
    free(heap);
}

void test_last_line_without_newline_is_parsed(void) {
    ParsedConfig cfg;
    parseExactAlloc("[config]\nname = Eric\nred_low = 3.9", &cfg);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.9f, cfg.red_low);
}

void test_single_line_without_newline(void) {
    ParsedConfig cfg;
    parseExactAlloc("[config]", &cfg);
    TEST_ASSERT_EQUAL_INT(0, cfg.unknownKeys);
}

void test_bare_key_without_newline(void) {
    ParsedConfig cfg;
    parseExactAlloc("[config]\nname = Eric", &cfg);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
}

void test_trailing_whitespace_without_newline(void) {
    ParsedConfig cfg;
    parseExactAlloc("[config]\nname = Eric   ", &cfg);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
}

void test_wlan_without_trailing_newline(void) {
    ParsedConfig cfg;
    parseExactAlloc("[wlan1]\nssid = My Net\npass = hunter2", &cfg);
    TEST_ASSERT_EQUAL_STRING("My Net", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("hunter2", cfg.wlanpass[0]);
}

void test_empty_buffer(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    char *heap = (char *)malloc(1);
    int parsed = parseConfigBuffer(heap, 0, &cfg);
    free(heap);
    TEST_ASSERT_EQUAL_INT(0, parsed);
}

void test_over_long_line_does_not_overflow(void) {
    ParsedConfig cfg;
    char big[2048];
    strcpy(big, "[config]\nnightscout = ");
    size_t at = strlen(big);
    memset(big + at, 'x', 1900);
    big[at + 1900] = '\0';
    parseExactAlloc(big, &cfg);
    TEST_ASSERT_TRUE(strlen(cfg.url) < sizeof(cfg.url));
}

void test_crlf_without_final_newline(void) {
    ParsedConfig cfg;
    parseExactAlloc("[config]\r\nname = Eric\r\ndst = 0", &cfg);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
    TEST_ASSERT_EQUAL_INT(0, cfg.dst);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_last_line_without_newline_is_parsed);
    RUN_TEST(test_single_line_without_newline);
    RUN_TEST(test_bare_key_without_newline);
    RUN_TEST(test_trailing_whitespace_without_newline);
    RUN_TEST(test_wlan_without_trailing_newline);
    RUN_TEST(test_empty_buffer);
    RUN_TEST(test_over_long_line_does_not_overflow);
    RUN_TEST(test_crlf_without_final_newline);
    return UNITY_END();
}
