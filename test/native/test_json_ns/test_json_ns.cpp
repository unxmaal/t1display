#include <unity.h>
#include <string.h>
#include <math.h>
#include "ns_json_parse.h"
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* ── Realistic Nightscout JSON fixtures ────────────────────────── */

// Standard Nightscout /api/v1/entries.json response (array of SGV entries)
static const char *NS_RESPONSE_NORMAL = R"([
  {"_id":"abc123","device":"xDrip-DexcomG6","date":1709312400000,
   "dateString":"2024-03-01T18:00:00.000Z","sgv":152,"delta":3.2,
   "direction":"Flat","type":"sgv","filtered":0,"unfiltered":0,"rssi":100},
  {"_id":"abc122","device":"xDrip-DexcomG6","date":1709312100000,
   "dateString":"2024-03-01T17:55:00.000Z","sgv":149,"delta":2.1,
   "direction":"Flat","type":"sgv"},
  {"_id":"abc121","device":"xDrip-DexcomG6","date":1709311800000,
   "sgv":147,"direction":"FortyFiveUp","type":"sgv"}
])";

// Response where first entry lacks "sgv" (e.g., calibration entry)
static const char *NS_RESPONSE_CAL_FIRST = R"([
  {"_id":"cal001","device":"xDrip","date":1709312400000,
   "type":"cal","slope":1000,"intercept":30000},
  {"_id":"sgv001","device":"xDrip","date":1709312100000,
   "sgv":180,"direction":"SingleUp","type":"sgv"}
])";

// Response with numeric "trend" instead of string "direction" (Railway/Dexcom)
static const char *NS_RESPONSE_NUMERIC_TREND = R"([
  {"_id":"r001","device":"share2","date":1709312400000,
   "sgv":95,"trend":4,"type":"sgv"}
])";

// Empty array
static const char *NS_RESPONSE_EMPTY = "[]";


// Nightscout /api/v2/properties/delta response
static const char *DELTA_RESPONSE = R"({
  "delta":{"absolute":5,"elapsedMins":4.99,"interpolated":false,
   "mean5MinsAgo":148,"mgdl":5,"scaled":5,"display":"+5"}
})";

// High glucose response
static const char *NS_RESPONSE_HIGH = R"([
  {"device":"Dexcom","date":1709312400000,
   "sgv":310,"direction":"DoubleUp","type":"sgv"}
])";

// Low glucose response
static const char *NS_RESPONSE_LOW = R"([
  {"device":"Dexcom","date":1709312400000,
   "sgv":55,"direction":"DoubleDown","type":"sgv"}
])";

/* ── parseSGVResponse tests ────────────────────────────────────── */

void test_parse_sgv_normal(void) {
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    int rc = parseSGVResponse(NS_RESPONSE_NORMAL, strlen(NS_RESPONSE_NORMAL), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, rc);
    TEST_ASSERT_EQUAL_STRING("xDrip-DexcomG6", entry.device);
    TEST_ASSERT_EQUAL_UINT64(1709312400000ULL, entry.date_ms);
    TEST_ASSERT_EQUAL(1709312400, entry.date_sec);
    TEST_ASSERT_EQUAL_FLOAT(152.0f, entry.sgv_mgdl);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 152.0f / MGDL_PER_MMOL, entry.sgv_mmol);
    TEST_ASSERT_EQUAL_STRING("Flat", entry.direction);
    TEST_ASSERT_EQUAL_INT(0, entry.arrow_angle);  // Flat = 0 degrees
}

void test_parse_sgv_skips_cal_entry(void) {
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    int rc = parseSGVResponse(NS_RESPONSE_CAL_FIRST, strlen(NS_RESPONSE_CAL_FIRST), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, rc);
    // Should skip cal entry and find the SGV entry
    TEST_ASSERT_EQUAL_FLOAT(180.0f, entry.sgv_mgdl);
    TEST_ASSERT_EQUAL_STRING("SingleUp", entry.direction);
    TEST_ASSERT_EQUAL_INT(-75, entry.arrow_angle);  // SingleUp = -75
}

void test_parse_sgv_numeric_trend(void) {
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    int rc = parseSGVResponse(NS_RESPONSE_NUMERIC_TREND, strlen(NS_RESPONSE_NUMERIC_TREND), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, rc);
    TEST_ASSERT_EQUAL_FLOAT(95.0f, entry.sgv_mgdl);
    TEST_ASSERT_EQUAL_STRING("Flat", entry.direction);  // trend=4 → Flat
    TEST_ASSERT_EQUAL_INT(0, entry.arrow_angle);
}

void test_parse_sgv_empty_array(void) {
    SGVEntry entry;
    int rc = parseSGVResponse(NS_RESPONSE_EMPTY, strlen(NS_RESPONSE_EMPTY), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_EMPTY, rc);
}

void test_parse_sgv_invalid_json(void) {
    SGVEntry entry;
    const char *bad = "{not valid json at all";
    int rc = parseSGVResponse(bad, strlen(bad), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, rc);
}

void test_parse_sgv_null_input(void) {
    SGVEntry entry;
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, parseSGVResponse(NULL, 0, &entry));
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, parseSGVResponse("[]", 2, NULL));
}

void test_parse_sgv_high_glucose(void) {
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    int rc = parseSGVResponse(NS_RESPONSE_HIGH, strlen(NS_RESPONSE_HIGH), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, rc);
    TEST_ASSERT_EQUAL_FLOAT(310.0f, entry.sgv_mgdl);
    TEST_ASSERT_EQUAL_STRING("DoubleUp", entry.direction);
    TEST_ASSERT_EQUAL_INT(-90, entry.arrow_angle);  // DoubleUp = -90
}

void test_parse_sgv_low_glucose(void) {
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    int rc = parseSGVResponse(NS_RESPONSE_LOW, strlen(NS_RESPONSE_LOW), &entry);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, rc);
    TEST_ASSERT_EQUAL_FLOAT(55.0f, entry.sgv_mgdl);
    TEST_ASSERT_EQUAL_INT(90, entry.arrow_angle);  // DoubleDown = 90
}

void test_parse_sgv_mmol_conversion(void) {
    SGVEntry entry;
    memset(&entry, 0, sizeof(entry));
    parseSGVResponse(NS_RESPONSE_NORMAL, strlen(NS_RESPONSE_NORMAL), &entry);
    TEST_ASSERT_EQUAL_FLOAT(152.0f / MGDL_PER_MMOL, entry.sgv_mmol);
}

/* ── parseDeltaResponse tests ──────────────────────────────────── */

void test_parse_delta(void) {
    DeltaInfo delta;
    memset(&delta, 0, sizeof(delta));
    int rc = parseDeltaResponse(DELTA_RESPONSE, strlen(DELTA_RESPONSE), &delta);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, rc);
    TEST_ASSERT_EQUAL_INT(5, delta.mgdl);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f / MGDL_PER_MMOL, delta.mmol);
}

void test_parse_delta_invalid(void) {
    DeltaInfo delta;
    int rc = parseDeltaResponse("not json", 8, &delta);
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_JSON, rc);
}

/* ── formatDelta tests ─────────────────────────────────────────── */

void test_format_delta_mgdl_positive(void) {
    DeltaInfo d = {5, 5.0f / MGDL_PER_MMOL};
    char buf[16];
    formatDelta(buf, sizeof(buf), &d, true);
    TEST_ASSERT_EQUAL_STRING("+5", buf);
}

void test_format_delta_mgdl_negative(void) {
    DeltaInfo d = {-3, -3.0f / 18.0f};
    char buf[16];
    formatDelta(buf, sizeof(buf), &d, true);
    TEST_ASSERT_EQUAL_STRING("-3", buf);
}

void test_format_delta_mgdl_zero(void) {
    DeltaInfo d = {0, 0.0f};
    char buf[16];
    formatDelta(buf, sizeof(buf), &d, true);
    TEST_ASSERT_EQUAL_STRING("+0", buf);
}

void test_format_delta_mmol_positive(void) {
    DeltaInfo d = {5, 5.0f / MGDL_PER_MMOL};
    char buf[16];
    formatDelta(buf, sizeof(buf), &d, false);
    TEST_ASSERT_EQUAL_STRING("+0.3", buf);
}

void test_format_delta_mmol_negative(void) {
    DeltaInfo d = {-18, -1.0f};
    char buf[16];
    formatDelta(buf, sizeof(buf), &d, false);
    TEST_ASSERT_EQUAL_STRING("-1.0", buf);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    // SGV parsing
    RUN_TEST(test_parse_sgv_normal);
    RUN_TEST(test_parse_sgv_skips_cal_entry);
    RUN_TEST(test_parse_sgv_numeric_trend);
    RUN_TEST(test_parse_sgv_empty_array);
    RUN_TEST(test_parse_sgv_invalid_json);
    RUN_TEST(test_parse_sgv_null_input);
    RUN_TEST(test_parse_sgv_high_glucose);
    RUN_TEST(test_parse_sgv_low_glucose);
    RUN_TEST(test_parse_sgv_mmol_conversion);


    // Delta
    RUN_TEST(test_parse_delta);
    RUN_TEST(test_parse_delta_invalid);

    // Formatting
    RUN_TEST(test_format_delta_mgdl_positive);
    RUN_TEST(test_format_delta_mgdl_negative);
    RUN_TEST(test_format_delta_mgdl_zero);
    RUN_TEST(test_format_delta_mmol_positive);
    RUN_TEST(test_format_delta_mmol_negative);

    return UNITY_END();
}
