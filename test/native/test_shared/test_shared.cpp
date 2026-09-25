#include <unity.h>
#include <string.h>
#include <mutex>
#include <thread>
#include <atomic>
#include "ns_shared.h"

void setUp(void) {}
void tearDown(void) {}

static std::mutex mtx;
static int acquires = 0;
static void lockMtx(void *m)   { static_cast<std::mutex *>(m)->lock(); acquires++; }
static void unlockMtx(void *m) { static_cast<std::mutex *>(m)->unlock(); }
static NsLock realLock() { return NsLock{lockMtx, unlockMtx, &mtx}; }

struct Big {
    char text[256];
    int  seq;
};

static Big make(int seq) {
    Big b;
    memset(b.text, 'a' + (seq % 26), sizeof(b.text) - 1);
    b.text[sizeof(b.text) - 1] = '\0';
    b.seq = seq;
    return b;
}

static bool consistent(const Big &b) {
    char want = (char)('a' + (b.seq % 26));
    for (size_t i = 0; i + 1 < sizeof(b.text); i++)
        if (b.text[i] != want) return false;
    return true;
}

void test_guarded_round_trips_a_value(void) {
    Guarded<Big> g;
    g.attach(realLock());
    g.publish(make(7));
    TEST_ASSERT_EQUAL_INT(7, g.snapshot().seq);
}

void test_guarded_takes_the_lock_for_every_access(void) {
    Guarded<int> g;
    g.attach(realLock());
    acquires = 0;
    g.publish(3);
    (void)g.snapshot();
    TEST_ASSERT_EQUAL_INT(2, acquires);
}

void test_guarded_works_before_a_lock_is_attached(void) {
    Guarded<int> g;
    g.publish(5);
    TEST_ASSERT_EQUAL_INT(5, g.snapshot());
}

void test_guarded_snapshots_are_never_torn(void) {
    Guarded<Big> g;
    g.attach(realLock());
    g.publish(make(0));
    std::atomic<bool> stop(false);
    std::thread writer([&]() {
        for (int i = 1; i < 20000; i++) g.publish(make(i));
        stop = true;
    });
    int torn = 0;
    while (!stop) {
        if (!consistent(g.snapshot())) torn++;
    }
    writer.join();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, torn, "a reader on another core must never see a half-written config");
}

void test_exchange_request_reply_cycle(void) {
    Exchange<int, int> x;
    x.attach(realLock());
    int req = 0, resp = 0;
    TEST_ASSERT_FALSE(x.take(&req));
    TEST_ASSERT_TRUE(x.submit(41));
    TEST_ASSERT_FALSE(x.collect(&resp));
    TEST_ASSERT_TRUE(x.take(&req));
    TEST_ASSERT_EQUAL_INT(41, req);
    TEST_ASSERT_FALSE(x.take(&req));
    x.reply(req + 1);
    TEST_ASSERT_TRUE(x.collect(&resp));
    TEST_ASSERT_EQUAL_INT(42, resp);
    TEST_ASSERT_FALSE(x.collect(&resp));
}

void test_exchange_rejects_a_second_request_while_busy(void) {
    Exchange<int, int> x;
    TEST_ASSERT_TRUE(x.submit(1));
    TEST_ASSERT_FALSE(x.submit(2));
    int req;
    x.take(&req);
    TEST_ASSERT_FALSE_MESSAGE(x.submit(3), "a request being worked on must not be overwritten");
    TEST_ASSERT_EQUAL_INT(1, req);
}

void test_exchange_accepts_new_request_after_an_uncollected_reply(void) {
    Exchange<int, int> x;
    int req, resp;
    x.submit(1);
    x.take(&req);
    x.reply(10);
    TEST_ASSERT_TRUE_MESSAGE(x.submit(2), "a caller that timed out must not wedge the exchange");
    TEST_ASSERT_FALSE(x.collect(&resp));
    TEST_ASSERT_TRUE(x.take(&req));
    TEST_ASSERT_EQUAL_INT(2, req);
}

void test_exchange_reply_without_take_is_ignored(void) {
    Exchange<int, int> x;
    int resp;
    x.reply(5);
    TEST_ASSERT_FALSE(x.collect(&resp));
}

void test_exchange_across_threads(void) {
    Exchange<Big, int> x;
    x.attach(realLock());
    std::atomic<bool> done(false);
    std::thread server([&]() {
        Big req;
        int served = 0;
        while (served < 200) {
            if (x.take(&req)) {
                x.reply(consistent(req) ? req.seq : -1);
                served++;
            }
        }
        done = true;
    });
    for (int i = 0; i < 200; i++) {
        while (!x.submit(make(i))) {}
        int resp = -2;
        while (!x.collect(&resp)) {}
        TEST_ASSERT_EQUAL_INT(i, resp);
    }
    server.join();
    TEST_ASSERT_TRUE(done);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_guarded_round_trips_a_value);
    RUN_TEST(test_guarded_takes_the_lock_for_every_access);
    RUN_TEST(test_guarded_works_before_a_lock_is_attached);
    RUN_TEST(test_guarded_snapshots_are_never_torn);
    RUN_TEST(test_exchange_request_reply_cycle);
    RUN_TEST(test_exchange_rejects_a_second_request_while_busy);
    RUN_TEST(test_exchange_accepts_new_request_after_an_uncollected_reply);
    RUN_TEST(test_exchange_reply_without_take_is_ignored);
    RUN_TEST(test_exchange_across_threads);
    return UNITY_END();
}
