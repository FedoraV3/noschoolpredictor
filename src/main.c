#include <stdio.h>
#include <curl/curl.h>

#include "../include/data.h"
#include "../include/predictor.h"
#include "../include/news_checker.h"

char* scraper_api_key = NULL;

/**
 * @brief Runs the weather-based school suspension predictor.
 * @return Zero after normal execution, including handled startup failures.
 */
int main(void) {
	/* Initialize cURL here */
	if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
		return FALSE;
	}

	/* Get scraper api key */
	scraper_api_key = getenv("APIFY_API_KEY");

	if (scraper_api_key == NULL) {
		printf("Failed to get APIFY_API_KEY. Get an API Key from Apify to use this program\n");
	}

	/* Initialize functions for cURL */
	if (data_curl_init() != TRUE) {
		printf("data_curl_init has failed\n");
		return 0;
	}

	if (news_curl_init() != TRUE) {
		printf("news_curl_init has failed\n");
		return 0;
	}
	
	weather_data_t weather_data;
	if (get_weather_data(&weather_data) != TRUE) {
		printf("get_weather_data has failed\n");
		return 0;
	}

	/* now check the news if school is really suspended for real */
	fb_post_t *fb_posts = malloc(64 * sizeof(fb_post_t));
	size_t fb_posts_size = 0;
	if (get_fb_posts(&fb_posts, &fb_posts_size) != TRUE) {
		printf("get_fb_posts has failed\n");
		return 0;
	}

	if (no_school_tomorrow(&fb_posts, fb_posts_size) != TRUE) {
		printf("There is school tomorrow\n");
	} else {
		printf("No school tomorrow\n");
	}

	/* Predict the weather now */
	suspension_prediction_t result = predict_suspension(&weather_data);
	switch (result.risk) {
	    case SUSPENSION_RISK_VERY_LOW:
	        printf("Risk: Very Low\n");
	        break;

	    case SUSPENSION_RISK_LOW:
	        printf("Risk: Low\n");
	        break;

	    case SUSPENSION_RISK_MODERATE:
	        printf("Risk: Moderate\n");
	        break;

	    case SUSPENSION_RISK_HIGH:
	        printf("Risk: High\n");
	        break;

	    case SUSPENSION_RISK_EXTREME:
	        printf("Risk: Extreme\n");
	        break;

	    default:
	        printf("Risk: Unknown\n");
	        break;
	}
	
	printf("Chances of no school: %.2f%%\n", result.probability);

	char buffer[64];
	printf("Press enter to exit..");
	fgets(buffer, sizeof(buffer), stdin);
	return 0;
}
