#include "curl_m.h"
#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "include/news_checker.h"
}

char *scraper_api_key = const_cast<char *>("test-key");

class NewsCheckerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        curl_mock::reset();
        news_easy_handle = nullptr;
    }
};

TEST_F(NewsCheckerTest, DetectsSuspensionCaseInsensitively)
{
    fb_post_t posts[] = {{0, 0, 0, 0, 0, 0, const_cast<char *>("CLASSES are SUSPENDED tomorrow"), 31}};
    fb_post_t *post_ptr = posts;
    EXPECT_TRUE(no_school_tomorrow(&post_ptr, 1));
}

TEST_F(NewsCheckerTest, DetectsCommonEnglishAnnouncements)
{
    const char *announcements[] = {
        "Suspension of classes in all levels",
        "There will be no classes tomorrow",
        "All schools are closed due to flooding",
        "Face-to-face learning is suspended",
        "Cancellation of afternoon classes"
    };

    for (const char *announcement : announcements) {
        fb_post_t post = {0, 0, 0, 0, 0, 0, const_cast<char *>(announcement), std::strlen(announcement)};
        fb_post_t *post_ptr = &post;
        EXPECT_TRUE(no_school_tomorrow(&post_ptr, 1)) << announcement;
    }
}

TEST_F(NewsCheckerTest, DetectsCommonFilipinoAnnouncements)
{
    const char *announcements[] = {
        "Walang klase bukas dahil sa bagyo",
        "Suspendido ang klase sa lahat ng antas",
        "Kanselado ang pasok ngayong hapon",
        "Kinansela ang klase dahil sa baha"
    };

    for (const char *announcement : announcements) {
        fb_post_t post = {0, 0, 0, 0, 0, 0, const_cast<char *>(announcement), std::strlen(announcement)};
        fb_post_t *post_ptr = &post;
        EXPECT_TRUE(no_school_tomorrow(&post_ptr, 1)) << announcement;
    }
}

TEST_F(NewsCheckerTest, DoesNotTreatContinuationAsSuspension)
{
    const char *announcements[] = {
        "Classes are not suspended tomorrow",
        "Classes continue as scheduled",
        "May pasok bukas",
        "Walang suspension ng klase"
    };

    for (const char *announcement : announcements) {
        fb_post_t post = {0, 0, 0, 0, 0, 0, const_cast<char *>(announcement), std::strlen(announcement)};
        fb_post_t *post_ptr = &post;
        EXPECT_FALSE(no_school_tomorrow(&post_ptr, 1)) << announcement;
    }
}

TEST_F(NewsCheckerTest, RejectsUnrelatedAndInvalidPosts)
{
    fb_post_t posts[] = {{0, 0, 0, 0, 0, 0, const_cast<char *>("classes continue normally"), 25}};
    fb_post_t *post_ptr = posts;
    EXPECT_FALSE(no_school_tomorrow(&post_ptr, 1));
    EXPECT_FALSE(no_school_tomorrow(nullptr, 1));
    EXPECT_FALSE(no_school_tomorrow(&post_ptr, 0));
}

TEST_F(NewsCheckerTest, InitializationConfiguresCurl)
{
    EXPECT_TRUE(news_curl_init());
    EXPECT_NE(news_easy_handle, nullptr);
}

TEST_F(NewsCheckerTest, InitializationReportsSetoptFailure)
{
    curl_mock::fail_next_setopt(CURLOPT_FOLLOWLOCATION);
    EXPECT_FALSE(news_curl_init());
}

TEST_F(NewsCheckerTest, FetchesAndParsesPosts)
{
    ASSERT_TRUE(news_curl_init());
    curl_mock::set_response(
        "[{\"text\":\"no school tomorrow\",\"timestamp\":1700000000},"
        "{\"text\":\"classes suspended\",\"timestamp\":1700003600}]");

    auto *posts = static_cast<fb_post_t *>(std::calloc(2, sizeof(fb_post_t)));
    size_t count = 0;
    ASSERT_TRUE(get_fb_posts(&posts, &count));
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(curl_mock::perform_count(), 1u);
    EXPECT_TRUE(curl_mock::post_enabled());
    EXPECT_NE(std::strstr(curl_mock::url(), "run-sync-get-dataset-items"), nullptr);
    EXPECT_NE(std::strstr(curl_mock::post_fields(), "resultsLimit"), nullptr);
    EXPECT_EQ(std::string(posts[0].post_str, posts[0].post_str_len), "no school tomorrow");

    for (size_t i = 0; i < count; ++i) std::free(posts[i].post_str);
    std::free(posts);
}

TEST_F(NewsCheckerTest, FetchFailsWhenCurlFails)
{
    ASSERT_TRUE(news_curl_init());
    curl_mock::set_perform_result(CURLE_COULDNT_CONNECT);
    auto *posts = static_cast<fb_post_t *>(std::calloc(2, sizeof(fb_post_t)));
    size_t count = 0;
    EXPECT_FALSE(get_fb_posts(&posts, &count));
    std::free(posts);
}
