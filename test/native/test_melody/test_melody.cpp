#include <unity.h>
#include "ns_melody.h"

void setUp(void) {}
void tearDown(void) {}

static const int NOTES[] = { 988, 698, 659 };
static const int DURS[]  = { 120, 200, 100 };

static MelodySequencer m;

void test_idle_before_start(void) {
    melodyInit(&m);
    TEST_ASSERT_FALSE(melodyActive(&m));
    TEST_ASSERT_EQUAL_INT(-1, melodyNextNote(&m, 0));
}

void test_first_note_is_due_immediately(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 1000);
    TEST_ASSERT_TRUE(melodyActive(&m));
    TEST_ASSERT_EQUAL_INT(0, melodyNextNote(&m, 1000));
}

void test_second_note_waits_for_the_first_to_finish(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 1000);
    TEST_ASSERT_EQUAL_INT(0, melodyNextNote(&m, 1000));
    TEST_ASSERT_EQUAL_INT(-1, melodyNextNote(&m, 1000 + 100));
    TEST_ASSERT_EQUAL_INT(1, melodyNextNote(&m, 1000 + 180));
}

void test_plays_every_note_in_order(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 0);
    int seen[3];
    int n = 0;
    for (unsigned long t = 0; t < 2000 && n < 3; t += 10) {
        int idx = melodyNextNote(&m, t);
        if (idx >= 0) seen[n++] = idx;
    }
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_INT(0, seen[0]);
    TEST_ASSERT_EQUAL_INT(1, seen[1]);
    TEST_ASSERT_EQUAL_INT(2, seen[2]);
}

void test_becomes_inactive_when_finished(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 0);
    for (unsigned long t = 0; t < 3000; t += 10)
        melodyNextNote(&m, t);
    TEST_ASSERT_FALSE(melodyActive(&m));
}

void test_never_blocks_more_than_one_note_ahead(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 0);
    TEST_ASSERT_EQUAL_INT(0, melodyNextNote(&m, 0));
    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, melodyNextNote(&m, 0),
        "the sequencer must not emit the whole melody in one tick");
}

void test_restart_replaces_current_melody(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 0);
    melodyNextNote(&m, 0);
    melodyStart(&m, NOTES, DURS, 3, 5000);
    TEST_ASSERT_EQUAL_INT(0, melodyNextNote(&m, 5000));
}

void test_rollover_safe(void) {
    melodyInit(&m);
    unsigned long nearMax = 0xFFFFFFFFUL - 50;
    melodyStart(&m, NOTES, DURS, 3, nearMax);
    TEST_ASSERT_EQUAL_INT(0, melodyNextNote(&m, nearMax));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, melodyNextNote(&m, nearMax + 200),
        "a millis() wrap mid-melody must not stall playback");
}

void test_zero_count_is_inactive(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 0, 0);
    TEST_ASSERT_FALSE(melodyActive(&m));
}

void test_note_frequency_and_duration_readable(void) {
    melodyInit(&m);
    melodyStart(&m, NOTES, DURS, 3, 0);
    int idx = melodyNextNote(&m, 0);
    TEST_ASSERT_EQUAL_INT(988, melodyFreqAt(&m, idx));
    TEST_ASSERT_EQUAL_INT(120, melodyDurationAt(&m, idx));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_idle_before_start);
    RUN_TEST(test_first_note_is_due_immediately);
    RUN_TEST(test_second_note_waits_for_the_first_to_finish);
    RUN_TEST(test_plays_every_note_in_order);
    RUN_TEST(test_becomes_inactive_when_finished);
    RUN_TEST(test_never_blocks_more_than_one_note_ahead);
    RUN_TEST(test_restart_replaces_current_melody);
    RUN_TEST(test_rollover_safe);
    RUN_TEST(test_zero_count_is_inactive);
    RUN_TEST(test_note_frequency_and_duration_readable);
    return UNITY_END();
}
