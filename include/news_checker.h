#ifndef NEWS_CHECKER_H
#define NEWS_CHECKER_H

#include <curl/curl.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <string.h>
#include "include/data.h"

extern CURL *news_easy_handle;
extern char* scraper_api_key;

typedef struct facebook_post {
	/* day related */
	int month;
	int day;
	long long year;

	/* hour related */
	int hour;
	int minute;
	int second;

	/* post data */
	char *post_str;
	size_t post_str_len;
} fb_post_t;

/**
 * @brief Initializes the HTTP client used to retrieve Facebook posts.
 *
 * Configures the CURL handle and request headers required by the scraper API.
 *
 * @retval TRUE The client was initialized successfully.
 * @retval FALSE CURL initialization or configuration failed.
 */
BOOL news_curl_init(void);

/**
 * @brief Uses regular expressions to determine whether Facebook posts announce
 * no school for tomorrow.
 *
 * Inspects each post using regular-expression matching for a school-suspension
 * announcement that applies to tomorrow.
 *
 * @param[in] fb_posts Array of Facebook posts to inspect with regular expressions.
 * @param[in] fb_posts_size Number of posts in @p fb_posts.
 *
 * @retval TRUE The posts indicate that there is no school tomorrow.
 * @retval FALSE The posts do not indicate that there is no school tomorrow.
 */
BOOL no_school_tomorrow(fb_post_t **fb_posts, size_t fb_posts_size);

/**
 * @brief Retrieves recent posts from the configured Facebook pages.
 *
 * @param[out] fb_posts Receives the dynamically allocated array of posts.
 * @param[out] post_count Receives the number of elements in @p fb_posts.
 *
 * @retval TRUE The posts were retrieved and parsed successfully.
 * @retval FALSE An argument was invalid or the request or parsing failed.
 * @pre news_curl_init() must have completed successfully.
 * @warning Apify is required to scrape the pages.
 * @warning The caller owns the returned posts and their post_str members.
 *
 * @attention you can ignore post_count if you want since get_fb_posts is fixed to the define
 * MAX_ITEMS. View news_checker.c for more information
 */
BOOL get_fb_posts(fb_post_t **fb_posts, size_t *post_count);

#endif
