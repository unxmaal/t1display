#include <unity.h>
#include <string.h>
#include "ns_url_build.h"

void setUp(void) {}
void tearDown(void) {}

static char url[320];

void test_bare_host_gets_https(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    TEST_ASSERT_EQUAL_STRING("https://ns.example.com", url);
}

void test_existing_scheme_preserved(void) {
    nsBuildBaseUrl(url, sizeof(url), "http://ns.example.com");
    TEST_ASSERT_EQUAL_STRING("http://ns.example.com", url);
}

void test_trailing_slash_stripped(void) {
    nsBuildBaseUrl(url, sizeof(url), "https://ns.example.com/");
    TEST_ASSERT_EQUAL_STRING("https://ns.example.com", url);
}

void test_empty_url_stays_empty(void) {
    nsBuildBaseUrl(url, sizeof(url), "");
    TEST_ASSERT_EQUAL_STRING("", url);
}

void test_entries_url_requests_one_entry(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    nsAppendEntriesPath(url, sizeof(url));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "https://ns.example.com/api/v1/entries.json?count=1&find[type][$eq]=sgv",
        url,
        "only the newest sgv entry is used, so only one should be fetched");
}

void test_entries_url_always_filters_to_sgv(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    nsAppendEntriesPath(url, sizeof(url));
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(url, "find[type][$eq]=sgv"),
        "with count=1 a non-sgv record would otherwise be returned");
}

void test_token_uses_ampersand_when_query_present(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    nsAppendEntriesPath(url, sizeof(url));
    nsAppendToken(url, sizeof(url), "abc123");
    TEST_ASSERT_NOT_NULL(strstr(url, "&token=abc123"));
}

void test_token_uses_question_mark_when_no_query(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    nsAppendToken(url, sizeof(url), "abc123");
    TEST_ASSERT_EQUAL_STRING("https://ns.example.com?token=abc123", url);
}

void test_empty_token_appends_nothing(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    nsAppendToken(url, sizeof(url), "");
    TEST_ASSERT_EQUAL_STRING("https://ns.example.com", url);
}

void test_properties_path(void) {
    nsBuildBaseUrl(url, sizeof(url), "ns.example.com");
    nsAppendPropertiesPath(url, sizeof(url));
    TEST_ASSERT_EQUAL_STRING("https://ns.example.com/api/v2/properties/delta", url);
}

void test_truncation_is_safe(void) {
    char small[24];
    nsBuildBaseUrl(small, sizeof(small), "averyveryverylonghostname.example.com");
    TEST_ASSERT_TRUE(strlen(small) < sizeof(small));
    nsAppendEntriesPath(small, sizeof(small));
    TEST_ASSERT_TRUE(strlen(small) < sizeof(small));
    nsAppendToken(small, sizeof(small), "token");
    TEST_ASSERT_TRUE(strlen(small) < sizeof(small));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_bare_host_gets_https);
    RUN_TEST(test_existing_scheme_preserved);
    RUN_TEST(test_trailing_slash_stripped);
    RUN_TEST(test_empty_url_stays_empty);
    RUN_TEST(test_entries_url_requests_one_entry);
    RUN_TEST(test_entries_url_always_filters_to_sgv);
    RUN_TEST(test_token_uses_ampersand_when_query_present);
    RUN_TEST(test_token_uses_question_mark_when_no_query);
    RUN_TEST(test_empty_token_appends_nothing);
    RUN_TEST(test_properties_path);
    RUN_TEST(test_truncation_is_safe);
    return UNITY_END();
}
