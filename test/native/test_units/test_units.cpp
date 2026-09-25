#include <unity.h>
#include <string.h>
#include <stdio.h>
#include "ns_pure_logic.h"
#include "ns_json_parse.h"
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

void test_conversion_matches_nightscout(void) {
    TEST_ASSERT_EQUAL_FLOAT(18.01559f, MGDL_PER_MMOL);
}

static float readingMmol(int mgdl) {
    char json[128];
    snprintf(json, sizeof(json), "[{\"sgv\":%d,\"date\":1700000000000,\"direction\":\"Flat\"}]", mgdl);
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    TEST_ASSERT_EQUAL_INT(PARSE_OK, parseSGVResponse(json, strlen(json), &entry));
    return entry.sgv_mmol;
}

static float thresholdMmol(const char *mgdl) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    cfg.show_mgdl = 1;
    const ConfigKV kv[] = {{"red_high", mgdl}};
    applyConfigForm(&cfg, kv, 1);
    return cfg.red_high;
}

void test_reading_and_threshold_in_mgdl_compare_equal(void) {
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(thresholdMmol("180"), readingMmol(180),
        "a reading exactly at a threshold must compare as at the threshold");
}

void test_reading_at_threshold_is_not_above_it(void) {
    float t = thresholdMmol("180");
    TEST_ASSERT_EQUAL_INT(GLUCOSE_COLOR_YELLOW,
        glucoseColor(readingMmol(180), 4.5f, 9.0f, 3.9f, t));
}

void test_delta_uses_the_same_factor(void) {
    const char *json = "{\"delta\":{\"mgdl\":18}}";
    DeltaInfo delta;
    memset(&delta, 0, sizeof(delta));
    TEST_ASSERT_EQUAL_INT(PARSE_OK, parseDeltaResponse(json, strlen(json), &delta));
    TEST_ASSERT_EQUAL_FLOAT(18.0f / MGDL_PER_MMOL, delta.mmol);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_conversion_matches_nightscout);
    RUN_TEST(test_reading_and_threshold_in_mgdl_compare_equal);
    RUN_TEST(test_reading_at_threshold_is_not_above_it);
    RUN_TEST(test_delta_uses_the_same_factor);
    return UNITY_END();
}
