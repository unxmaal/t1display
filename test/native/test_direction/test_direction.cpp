#include <unity.h>
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* ── camelCase direction strings ────────────────────────────────── */

void test_direction_DoubleDown(void) {
    TEST_ASSERT_EQUAL_INT(90, directionToAngle("DoubleDown"));
}

void test_direction_SingleDown(void) {
    TEST_ASSERT_EQUAL_INT(75, directionToAngle("SingleDown"));
}

void test_direction_FortyFiveDown(void) {
    TEST_ASSERT_EQUAL_INT(45, directionToAngle("FortyFiveDown"));
}

void test_direction_Flat(void) {
    TEST_ASSERT_EQUAL_INT(0, directionToAngle("Flat"));
}

void test_direction_FortyFiveUp(void) {
    TEST_ASSERT_EQUAL_INT(-45, directionToAngle("FortyFiveUp"));
}

void test_direction_SingleUp(void) {
    TEST_ASSERT_EQUAL_INT(-75, directionToAngle("SingleUp"));
}

void test_direction_DoubleUp(void) {
    TEST_ASSERT_EQUAL_INT(-90, directionToAngle("DoubleUp"));
}

/* ── UPPER_CASE direction strings ───────────────────────────────── */

void test_direction_DOUBLE_DOWN(void) {
    TEST_ASSERT_EQUAL_INT(90, directionToAngle("DOUBLE_DOWN"));
}

void test_direction_SINGLE_DOWN(void) {
    TEST_ASSERT_EQUAL_INT(75, directionToAngle("SINGLE_DOWN"));
}

void test_direction_FORTY_FIVE_DOWN(void) {
    TEST_ASSERT_EQUAL_INT(45, directionToAngle("FORTY_FIVE_DOWN"));
}

void test_direction_FLAT(void) {
    TEST_ASSERT_EQUAL_INT(0, directionToAngle("FLAT"));
}

void test_direction_FORTY_FIVE_UP(void) {
    TEST_ASSERT_EQUAL_INT(-45, directionToAngle("FORTY_FIVE_UP"));
}

void test_direction_SINGLE_UP(void) {
    TEST_ASSERT_EQUAL_INT(-75, directionToAngle("SINGLE_UP"));
}

void test_direction_DOUBLE_UP(void) {
    TEST_ASSERT_EQUAL_INT(-90, directionToAngle("DOUBLE_UP"));
}

/* ── Edge cases ─────────────────────────────────────────────────── */

void test_direction_NONE(void) {
    TEST_ASSERT_EQUAL_INT(180, directionToAngle("NONE"));
}

void test_direction_NOT_COMPUTABLE(void) {
    TEST_ASSERT_EQUAL_INT(180, directionToAngle("NOT COMPUTABLE"));
}

void test_direction_unknown(void) {
    TEST_ASSERT_EQUAL_INT(180, directionToAngle("garbage"));
}

void test_direction_empty(void) {
    TEST_ASSERT_EQUAL_INT(180, directionToAngle(""));
}

void test_direction_null(void) {
    TEST_ASSERT_EQUAL_INT(180, directionToAngle(NULL));
}

void test_direction_TripleUp(void) {
    TEST_ASSERT_EQUAL_INT(-90, directionToAngle("TripleUp"));
    TEST_ASSERT_EQUAL_INT(-90, directionToAngle("TRIPLE_UP"));
}

void test_direction_TripleDown(void) {
    TEST_ASSERT_EQUAL_INT(90, directionToAngle("TripleDown"));
    TEST_ASSERT_EQUAL_INT(90, directionToAngle("TRIPLE_DOWN"));
}

void test_style_single_for_ordinary_trends(void) {
    TEST_ASSERT_EQUAL_INT(ARROW_SINGLE, directionArrowStyle("Flat"));
    TEST_ASSERT_EQUAL_INT(ARROW_SINGLE, directionArrowStyle("DoubleUp"));
    TEST_ASSERT_EQUAL_INT(ARROW_SINGLE, directionArrowStyle("FORTY_FIVE_DOWN"));
}

void test_style_double_for_triple_trends(void) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(ARROW_DOUBLE, directionArrowStyle("TripleUp"),
        "the fastest rise must not look like no trend");
    TEST_ASSERT_EQUAL_INT(ARROW_DOUBLE, directionArrowStyle("TripleDown"));
    TEST_ASSERT_EQUAL_INT(ARROW_DOUBLE, directionArrowStyle("TRIPLE_DOWN"));
}

void test_style_rate_out_of_range(void) {
    TEST_ASSERT_EQUAL_INT(ARROW_RATE_OUT_OF_RANGE, directionArrowStyle("RATE OUT OF RANGE"));
    TEST_ASSERT_EQUAL_INT(ARROW_RATE_OUT_OF_RANGE, directionArrowStyle("RATE_OUT_OF_RANGE"));
}

void test_style_none_for_unknown(void) {
    TEST_ASSERT_EQUAL_INT(ARROW_NONE, directionArrowStyle("NONE"));
    TEST_ASSERT_EQUAL_INT(ARROW_NONE, directionArrowStyle("NOT COMPUTABLE"));
    TEST_ASSERT_EQUAL_INT(ARROW_NONE, directionArrowStyle("garbage"));
    TEST_ASSERT_EQUAL_INT(ARROW_NONE, directionArrowStyle(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_direction_DoubleDown);
    RUN_TEST(test_direction_SingleDown);
    RUN_TEST(test_direction_FortyFiveDown);
    RUN_TEST(test_direction_Flat);
    RUN_TEST(test_direction_FortyFiveUp);
    RUN_TEST(test_direction_SingleUp);
    RUN_TEST(test_direction_DoubleUp);
    RUN_TEST(test_direction_DOUBLE_DOWN);
    RUN_TEST(test_direction_SINGLE_DOWN);
    RUN_TEST(test_direction_FORTY_FIVE_DOWN);
    RUN_TEST(test_direction_FLAT);
    RUN_TEST(test_direction_FORTY_FIVE_UP);
    RUN_TEST(test_direction_SINGLE_UP);
    RUN_TEST(test_direction_DOUBLE_UP);
    RUN_TEST(test_direction_NONE);
    RUN_TEST(test_direction_NOT_COMPUTABLE);
    RUN_TEST(test_direction_unknown);
    RUN_TEST(test_direction_empty);
    RUN_TEST(test_direction_null);
    RUN_TEST(test_direction_TripleUp);
    RUN_TEST(test_direction_TripleDown);
    RUN_TEST(test_style_single_for_ordinary_trends);
    RUN_TEST(test_style_double_for_triple_trends);
    RUN_TEST(test_style_rate_out_of_range);
    RUN_TEST(test_style_none_for_unknown);
    return UNITY_END();
}
