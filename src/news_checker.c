#include "../include/news_checker.h"
#include "../include/re.h"
#include <ctype.h>


CURL *news_easy_handle;

static const char *API_ENDPOINT = "https://api.apify.com/v2/acts/";
static const char *API_ACTOR_ID = "apify~facebook-posts-scraper";

/* change these to what you want */
#define MAX_ITEMS 2
#define FB_PAGES_SIZE 2
#define FB_PAGES_NOT_OLDER_THAN "1 day"

/* In the final product, these will be set as options in the GUI*/
static const char *fb_pages_to_scrape[FB_PAGES_SIZE] = {
	"https://www.facebook.com/stacruz.lgu",
	"https://www.facebook.com/ThePGIS1818"
};

static const char *school_suspension_regexes[] = {
	/* English announcements. Stems also match suspended/suspension and
	 * canceled/cancelled/cancellation. */
	"class.*suspend",
	"suspend.*class",
	"class.*suspens",
	"suspens.*class",
	"class.*cancel",
	"cancel.*class",
	"no.*class",
	"no school",
	"school.*clos",
	"clos.*school",
	"face.to.face.*suspend",
	"suspend.*face.to.face",

	/* Common Filipino announcement wording. */
	"walang pasok",
	"walang klase",
	"suspendido.*klase",
	"suspindido.*klase",
	"suspendido.*pasok",
	"kanselado.*klase",
	"kanselado.*pasok",
	"kinansela.*klase",
	"klase.*suspendido",
	"klase.*kanselado"
};

static const char *school_continuation_regexes[] = {
	"class.*not.*suspend",
	"class.*not.*cancel",
	"classes.*continue",
	"classes.*resume",
	"school.*open",
	"may pasok",
	"may klase",
	"hindi.*suspendido",
	"hindi.*suspindido",
	"walang suspens"
};

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t real_size = size * nmemb;

	response_t *p_response = (response_t *)userdata;

	char *new_buffer = realloc(
		p_response->string,
		p_response->len + real_size + 1);

	if (new_buffer == NULL)
	{
		return 0; /* curl will report CURLE_WRITE_ERROR */
	}

	p_response->string = new_buffer;

	memcpy(
		p_response->string + p_response->len,
		ptr,
		real_size);

	p_response->len += real_size;
	p_response->capacity = p_response->len + 1;
	p_response->string[p_response->len] = '\0';

	return real_size;
}

BOOL news_curl_init(void) {
	news_easy_handle = curl_easy_init();
	/* since we just need to run the actor and get the data from it we will just use this */
	if (curl_easy_setopt(news_easy_handle, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK) return FALSE;
	if (curl_easy_setopt(news_easy_handle, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) return FALSE;
	if (curl_easy_setopt(news_easy_handle, CURLOPT_DEFAULT_PROTOCOL, "https") != CURLE_OK) return FALSE;

	/* headers */
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "Accept: application/json");

	char small_buffer[512];
	snprintf(small_buffer, sizeof(small_buffer), "Authorization: Bearer %s", scraper_api_key);
	headers = curl_slist_append(headers, small_buffer);

	if (curl_easy_setopt(news_easy_handle, CURLOPT_HTTPHEADER, headers) != CURLE_OK) return FALSE;

	return TRUE;
}

BOOL no_school_tomorrow(fb_post_t **fb_posts, size_t fb_posts_size) {
	if (fb_posts == NULL || *fb_posts == NULL || fb_posts_size == 0) return FALSE;

	for (size_t i = 0; i < fb_posts_size; i++) {
		const char *post = (*fb_posts)[i].post_str;
		if (post == NULL) continue;

		const size_t post_len = strlen(post);
		char *lowercase_post = malloc(post_len + 1);
		if (lowercase_post == NULL) return FALSE;

		for (size_t j = 0; j < post_len; j++) {
			lowercase_post[j] = (char)tolower((unsigned char)post[j]);
		}
		lowercase_post[post_len] = '\0';

		for (size_t j = 0; j < sizeof(school_continuation_regexes) / sizeof(school_continuation_regexes[0]); j++) {
			int match_length = 0;
			if (re_match(school_continuation_regexes[j], lowercase_post, &match_length) >= 0) {
				free(lowercase_post);
				lowercase_post = NULL;
				break;
			}
		}

		if (lowercase_post == NULL) continue;

		for (size_t j = 0; j < sizeof(school_suspension_regexes) / sizeof(school_suspension_regexes[0]); j++) {
			int match_length = 0;
			if (re_match(school_suspension_regexes[j], lowercase_post, &match_length) >= 0) {
				free(lowercase_post);
				return TRUE;
			}
		}

		free(lowercase_post);
	}

	return FALSE;
}

BOOL get_fb_posts(fb_post_t **fb_posts, size_t *fb_posts_size) {
	response_t response = {0};

	if (fb_posts == NULL || news_easy_handle == NULL) return FALSE;

	char endpoint[256];
	snprintf(endpoint, sizeof(endpoint), "%s%s/run-sync-get-dataset-items", API_ENDPOINT, API_ACTOR_ID);

	cJSON *json_data = cJSON_CreateObject();
	if (json_data == NULL) return FALSE;
	cJSON *urls = cJSON_AddArrayToObject(json_data, "startUrls");
	if (urls == NULL) {
		cJSON_Delete(json_data);
		return FALSE;
	}

	for (size_t i = 0; i < FB_PAGES_SIZE; i++) {
		cJSON *item = cJSON_CreateObject();
		if (item == NULL) {
			cJSON_Delete(json_data);
			return FALSE;
		}
		cJSON_AddStringToObject(item, "url", fb_pages_to_scrape[i]);
		cJSON_AddItemToArray(urls, item);
	}

	cJSON_AddNumberToObject(json_data, "resultsLimit", MAX_ITEMS);
	cJSON_AddStringToObject(json_data, "onlyPostsNewerThan", FB_PAGES_NOT_OLDER_THAN);
	char *request = cJSON_PrintUnformatted(json_data);

	cJSON_Delete(json_data);

	if (request == NULL) return FALSE;

	if (curl_easy_setopt(news_easy_handle, CURLOPT_POST, 1L) != CURLE_OK) {
		cJSON_free(request);
		return FALSE;
	}
	if (curl_easy_setopt(news_easy_handle, CURLOPT_URL, endpoint) != CURLE_OK) {
		cJSON_free(request);
		return FALSE;
	}
	if (curl_easy_setopt(news_easy_handle, CURLOPT_WRITEDATA, &response) != CURLE_OK) {
		cJSON_free(request);
		return FALSE;
	}
	if (curl_easy_setopt(news_easy_handle, CURLOPT_POSTFIELDS, request) != CURLE_OK) {
		cJSON_free(request);
		return FALSE;
	}

	if (curl_easy_perform(news_easy_handle) != CURLE_OK) {
		cJSON_free(request);
		free(response.string);
		return FALSE;
	}

	cJSON_free(request);

	/* i dont wanna wast ememory so i will just load it here since i have no choice */
	cJSON *response_json = cJSON_Parse(response.string);
	if (response_json == NULL) {
		free(response.string);
		return FALSE;
	}

	/* parse the string here */
	for (int i = 0; i < FB_PAGES_SIZE; i++) {
		fb_post_t fb_post = {0};
		cJSON *json_data = cJSON_GetArrayItem(response_json, i);

		/* get the string and the date */
		cJSON *post_data = cJSON_GetObjectItemCaseSensitive(json_data, "text");
		/* we will use this to get the date, time */
		cJSON *post_timestamp  = cJSON_GetObjectItemCaseSensitive(json_data, "timestamp");

		if (!cJSON_IsNumber(post_timestamp)) {
			cJSON_Delete(response_json);
			free(response.string);
			return FALSE;
		}

		struct tm tm;
		const time_t timestamp = post_timestamp->valueint;

		localtime_s(&tm, &timestamp);

		fb_post.year = tm.tm_year;
		fb_post.month = tm.tm_mon;
		fb_post.day = tm.tm_mday;

		fb_post.hour = tm.tm_hour;
		fb_post.minute = tm.tm_min;
		fb_post.second = tm.tm_sec;

		size_t post_data_len = strlen(post_data->valuestring);
		fb_post.post_str = malloc(post_data_len + 1 * sizeof(char));
		if (fb_post.post_str == NULL) {
			cJSON_Delete(response_json);
			free(fb_post.post_str);
			fb_post.post_str = NULL;
			free(response.string);
			return FALSE;
		}
		strncpy(fb_post.post_str, post_data->valuestring, post_data_len);

		fb_post.post_str_len = post_data_len;

		/* insert into the **fb_posts array */
		(*fb_posts)[i] = fb_post;
	}

	*fb_posts_size = FB_PAGES_SIZE;

	cJSON_free(response_json);
	free(response.string);

	return TRUE;
}
