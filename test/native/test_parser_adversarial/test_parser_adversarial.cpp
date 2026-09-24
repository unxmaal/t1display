#include <unity.h>
#include <string.h>
#include <stdio.h>
#include "ns_json_parse.h"
#include "ns_display_model.h"
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

static SGVEntry entry;
static DeltaInfo delta;

static int parseTrend(int trend) {
    char json[160];
    snprintf(json, sizeof(json),
             "[{\"sgv\":120,\"date\":1700000000000,\"trend\":%d,\"device\":\"x\"}]", trend);
    memset(&entry, 0, sizeof(entry));
    return parseSGVResponse(json, strlen(json), &entry);
}

void test_numeric_trend_1_double_up(void) {
    TEST_ASSERT_EQUAL_INT(PARSE_OK, parseTrend(1));
    TEST_ASSERT_EQUAL_STRING("DoubleUp", entry.direction);
}
void test_numeric_trend_2_single_up(void) {
    parseTrend(2); TEST_ASSERT_EQUAL_STRING("SingleUp", entry.direction);
}
void test_numeric_trend_3_fortyfive_up(void) {
    parseTrend(3); TEST_ASSERT_EQUAL_STRING("FortyFiveUp", entry.direction);
}
void test_numeric_trend_4_flat(void) {
    parseTrend(4); TEST_ASSERT_EQUAL_STRING("Flat", entry.direction);
}
void test_numeric_trend_5_fortyfive_down(void) {
    parseTrend(5); TEST_ASSERT_EQUAL_STRING("FortyFiveDown", entry.direction);
}
void test_numeric_trend_6_single_down(void) {
    parseTrend(6); TEST_ASSERT_EQUAL_STRING("SingleDown", entry.direction);
}
void test_numeric_trend_7_double_down(void) {
    parseTrend(7); TEST_ASSERT_EQUAL_STRING("DoubleDown", entry.direction);
}
void test_numeric_trend_out_of_range_is_none(void) {
    parseTrend(99); TEST_ASSERT_EQUAL_STRING("NONE", entry.direction);
    parseTrend(0);  TEST_ASSERT_EQUAL_STRING("NONE", entry.direction);
    parseTrend(-3); TEST_ASSERT_EQUAL_STRING("NONE", entry.direction);
}

void test_string_trend_fallback(void) {
    const char *j = "[{\"sgv\":120,\"date\":1700000000000,\"trend\":\"SingleUp\"}]";
    memset(&entry, 0, sizeof(entry));
    TEST_ASSERT_EQUAL_INT(PARSE_OK, parseSGVResponse(j, strlen(j), &entry));
    TEST_ASSERT_EQUAL_STRING("SingleUp", entry.direction);
}

void test_empty_string_trend_is_none(void) {
    const char *j = "[{\"sgv\":120,\"date\":1700000000000,\"trend\":\"\"}]";
    memset(&entry, 0, sizeof(entry));
    parseSGVResponse(j, strlen(j), &entry);
    TEST_ASSERT_EQUAL_STRING("NONE", entry.direction);
}

void test_direction_na_falls_through_to_trend(void) {
    const char *j = "[{\"sgv\":120,\"date\":1700000000000,\"direction\":\"N/A\",\"trend\":2}]";
    memset(&entry, 0, sizeof(entry));
    parseSGVResponse(j, strlen(j), &entry);
    TEST_ASSERT_EQUAL_STRING("SingleUp", entry.direction);
}

void test_array_without_sgv_field_is_rejected(void) {
    const char *j = "[{\"mbg\":120,\"date\":1700000000000}]";
    memset(&entry, 0, sizeof(entry));
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_NO_SGV, parseSGVResponse(j, strlen(j), &entry));
}

void test_html_error_page_is_rejected(void) {
    const char *j = "<html><body>502 Bad Gateway</body></html>";
    memset(&entry, 0, sizeof(entry));
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, parseSGVResponse(j, strlen(j), &entry));
}

void test_truncated_json_is_rejected(void) {
    const char *j = "[{\"sgv\":120,\"date\":170000";
    memset(&entry, 0, sizeof(entry));
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, parseSGVResponse(j, strlen(j), &entry));
}

void test_null_args_rejected(void) {
    TEST_ASSERT_NOT_EQUAL(PARSE_OK, parseSGVResponse(NULL, 0, &entry));
    TEST_ASSERT_NOT_EQUAL(PARSE_OK, parseDeltaResponse(NULL, 0, &delta));
}

void test_oversized_device_name_truncates_safely(void) {
    char j[512];
    snprintf(j, sizeof(j),
             "[{\"sgv\":120,\"date\":1700000000000,\"device\":\"%.*s\"}]",
             200, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    memset(&entry, 0, sizeof(entry));
    parseSGVResponse(j, strlen(j), &entry);
    TEST_ASSERT_TRUE(strlen(entry.device) < sizeof(entry.device));
}

void test_delta_missing_field_is_rejected(void) {
    const char *j = "{}";
    memset(&delta, 0, sizeof(delta));
    TEST_ASSERT_EQUAL_INT_MESSAGE(PARSE_ERR_NO_SGV, parseDeltaResponse(j, strlen(j), &delta),
        "an empty object must not report a successful delta of zero");
}

void test_delta_malformed_is_rejected(void) {
    const char *j = "{\"delta\":";
    memset(&delta, 0, sizeof(delta));
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, parseDeltaResponse(j, strlen(j), &delta));
}

void test_delta_valid_still_parses(void) {
    const char *j = "{\"delta\":{\"mgdl\":-5}}";
    memset(&delta, 0, sizeof(delta));
    TEST_ASSERT_EQUAL_INT(PARSE_OK, parseDeltaResponse(j, strlen(j), &delta));
    TEST_ASSERT_EQUAL_INT(-5, delta.mgdl);
}

void test_error_descriptions_cover_known_codes(void) {
    StatusPageModel m;
    int codes[] = {1001, 1002, 1003, -5, 404};
    char dates[5][16] = {"a","b","c","d","e"};
    buildStatusModel(&m, codes, dates, 5, 5, 1000, 2000, "1.2.3.4", "v1", 50);
    TEST_ASSERT_EQUAL_INT(5, m.display_count);
    TEST_ASSERT_NOT_NULL(strstr(m.errors[0].desc_str, "JSON"));
    TEST_ASSERT_NOT_NULL(strstr(m.errors[1].desc_str, "No data"));
    TEST_ASSERT_NOT_NULL(strstr(m.errors[2].desc_str, "JSON2"));
    TEST_ASSERT_NOT_NULL(strstr(m.errors[3].desc_str, "HTTP"));
}

void test_status_model_clamps_overflow(void) {
    StatusPageModel m;
    int codes[32];
    char dates[32][16];
    for (int i = 0; i < 32; i++) { codes[i] = 1001; strcpy(dates[i], "x"); }
    buildStatusModel(&m, codes, dates, 32, 32, 0, 0, "ip", "v", 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(STATUS_MAX_ERRORS, m.display_count,
        "the display count must be clamped to the array it indexes");
}

void test_status_model_null_arrays(void) {
    StatusPageModel m;
    buildStatusModel(&m, NULL, NULL, 5, 5, 0, 0, "ip", "v", 0);
    TEST_ASSERT_EQUAL_INT(0, m.display_count);
}

void test_serialize_truncation_reported_at_every_field(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    for (size_t sz = 8; sz < 900; sz += 37) {
        char buf[1024];
        int n = serializeConfigINI(&cfg, buf, sz);
        TEST_ASSERT_TRUE_MESSAGE(n == 0 || (size_t)n < sz,
            "a truncated serialize must report failure, never overrun");
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_numeric_trend_1_double_up);
    RUN_TEST(test_numeric_trend_2_single_up);
    RUN_TEST(test_numeric_trend_3_fortyfive_up);
    RUN_TEST(test_numeric_trend_4_flat);
    RUN_TEST(test_numeric_trend_5_fortyfive_down);
    RUN_TEST(test_numeric_trend_6_single_down);
    RUN_TEST(test_numeric_trend_7_double_down);
    RUN_TEST(test_numeric_trend_out_of_range_is_none);
    RUN_TEST(test_string_trend_fallback);
    RUN_TEST(test_empty_string_trend_is_none);
    RUN_TEST(test_direction_na_falls_through_to_trend);
    RUN_TEST(test_array_without_sgv_field_is_rejected);
    RUN_TEST(test_html_error_page_is_rejected);
    RUN_TEST(test_truncated_json_is_rejected);
    RUN_TEST(test_null_args_rejected);
    RUN_TEST(test_oversized_device_name_truncates_safely);
    RUN_TEST(test_delta_missing_field_is_rejected);
    RUN_TEST(test_delta_malformed_is_rejected);
    RUN_TEST(test_delta_valid_still_parses);
    RUN_TEST(test_error_descriptions_cover_known_codes);
    RUN_TEST(test_status_model_clamps_overflow);
    RUN_TEST(test_status_model_null_arrays);
    RUN_TEST(test_serialize_truncation_reported_at_every_field);
    return UNITY_END();
}
