#include <unity.h>
#include <string.h>
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* ── Control character replacement ──────────────────────────────── */

void test_sanitize_replaces_control_chars(void) {
    char buf[] = "hello\x01world\x1F!";
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("hello world !", buf);
    TEST_ASSERT_EQUAL(13, len);
}

void test_sanitize_replaces_newline_tab(void) {
    char buf[] = "line1\nline2\ttab";
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("line1 line2 tab", buf);
    TEST_ASSERT_EQUAL(15, len);
}

/* ── Unicode escape replacement ─────────────────────────────────── */

void test_sanitize_replaces_u0000(void) {
    char buf[64];
    strcpy(buf, "before\\u0000after");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("before after", buf);
    TEST_ASSERT_EQUAL(12, len);
}

void test_sanitize_replaces_u000b(void) {
    char buf[64];
    strcpy(buf, "x\\u000by");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("x y", buf);
    TEST_ASSERT_EQUAL(3, len);
}

void test_sanitize_replaces_u0002(void) {
    char buf[64];
    strcpy(buf, "test\\u0002val");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("test val", buf);
    TEST_ASSERT_EQUAL(8, len);
}

void test_sanitize_keeps_escaped_digit_two(void) {
    char buf[64];
    strcpy(buf, "test\\u0032val");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("test\\u0032val", buf,
        "\\u0032 is the digit 2, not a control character");
    TEST_ASSERT_EQUAL(13, len);
}

void test_sanitize_multiple_unicode_escapes(void) {
    char buf[128];
    strcpy(buf, "a\\u0000b\\u000bc\\u0002d");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("a b c d", buf);
    TEST_ASSERT_EQUAL(7, len);
}

/* ── Date fractional millisecond stripping ──────────────────────── */

void test_sanitize_strips_date_fractional_ms(void) {
    char buf[128];
    strcpy(buf, "{\"date\":1234567890.123,\"value\":100}");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("{\"date\":1234567890,\"value\":100}", buf);
    TEST_ASSERT_EQUAL(strlen(buf), len);
}

void test_sanitize_strips_multiple_date_fields(void) {
    char buf[256];
    strcpy(buf, "[{\"date\":111.22},{\"date\":333.4444}]");
    size_t len = sanitizeJson(buf, strlen(buf));
    TEST_ASSERT_EQUAL_STRING("[{\"date\":111},{\"date\":333}]", buf);
    TEST_ASSERT_EQUAL(strlen(buf), len);
}

void test_sanitize_date_without_fraction_unchanged(void) {
    char buf[128];
    strcpy(buf, "{\"date\":1234567890,\"value\":100}");
    size_t origLen = strlen(buf);
    size_t len = sanitizeJson(buf, origLen);
    TEST_ASSERT_EQUAL_STRING("{\"date\":1234567890,\"value\":100}", buf);
    TEST_ASSERT_EQUAL(origLen, len);
}

/* ── Clean passthrough ──────────────────────────────────────────── */

void test_sanitize_clean_json_unchanged(void) {
    char buf[128];
    strcpy(buf, "{\"sgv\":120,\"direction\":\"Flat\"}");
    size_t origLen = strlen(buf);
    size_t len = sanitizeJson(buf, origLen);
    TEST_ASSERT_EQUAL_STRING("{\"sgv\":120,\"direction\":\"Flat\"}", buf);
    TEST_ASSERT_EQUAL(origLen, len);
}

void test_sanitize_empty_string(void) {
    char buf[4] = "";
    size_t len = sanitizeJson(buf, 0);
    TEST_ASSERT_EQUAL_STRING("", buf);
    TEST_ASSERT_EQUAL(0, len);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sanitize_replaces_control_chars);
    RUN_TEST(test_sanitize_replaces_newline_tab);
    RUN_TEST(test_sanitize_replaces_u0000);
    RUN_TEST(test_sanitize_replaces_u000b);
    RUN_TEST(test_sanitize_replaces_u0002);
    RUN_TEST(test_sanitize_keeps_escaped_digit_two);
    RUN_TEST(test_sanitize_multiple_unicode_escapes);
    RUN_TEST(test_sanitize_strips_date_fractional_ms);
    RUN_TEST(test_sanitize_strips_multiple_date_fields);
    RUN_TEST(test_sanitize_date_without_fraction_unchanged);
    RUN_TEST(test_sanitize_clean_json_unchanged);
    RUN_TEST(test_sanitize_empty_string);
    return UNITY_END();
}
