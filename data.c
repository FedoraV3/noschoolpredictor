/*
 * data.c
 *
 *  Created on: Aug 18, 2026
 *      Author: ir0n1c
 */

#include "data.h"
#include "cJSON.h"
#include "curl/curl.h"
#include "curl/easy.h"
#include <stdlib.h>

/* global variables for cURL */
/* this will be the handle used for the sessions with the weather api or it will be one shot soon */
CURL *easy_handle;

/* it must return in this format: x coords,y coords*/
static const char *IP_API_SERVICE = "https://ipconfig.io/coordinates";
static const char *WEATHER_API_SERVICE = "https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&hourly=temperature_2m,precipitation_probability,precipitation,rain,showers,weather_code,wind_speed_10m,wind_gusts_10m,cloud_cover,visibility,surface_pressure,relative_humidity_2m&timezone=auto";
static const size_t WEATHER_API_SIZE = 257 * sizeof(char);

static float usr_x, usr_y;

/**
	@brief This is the write callback function, all the data gets passed into a struct named
	response_t. View definition of response_t for more details.
	
	@param[out] userdata = pointer to a response_t. This will receive the response string and the size and capacity
	is included in the response_t struct
	
	@retval The result of size * nmemb.
*/
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
#ifndef NDEBUG
    printf("response size: %zu\n", size * nmemb);
#endif

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

/**
	@brief Grab the user's public IP, then lookup it for the longitude and latitude so we have data for the weather API
	@retval A value that is equal to truthy if the function has succeeded, whilst falsy if not.
*/
static BOOL get_coordinates_from_ip(float *x_buf, float *y_buf) {
	/* create a handle for one session */
	CURL *ip_handle = curl_easy_init();
	response_t url_response = {0}; 

	if (curl_easy_setopt(ip_handle, CURLOPT_URL, IP_API_SERVICE) != CURLE_OK) {
		return FALSE;
	}
	
	if (curl_easy_setopt(ip_handle, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK) {
		return FALSE;
	} 
	
	if (curl_easy_setopt(ip_handle, CURLOPT_WRITEDATA, &url_response) != CURLE_OK) {
		return FALSE;
	}
	
	if (curl_easy_perform(ip_handle) != CURLE_OK) {
		return FALSE;
	}
	
	curl_easy_cleanup(ip_handle);
	
	/* extract the x, y coordinates so that we can return it to the caller */
	char *x = strtok(url_response.string, ",");
	char *y = strtok(NULL, ",");
	
	*x_buf = atof(x);
	*y_buf = atof(y);
		
	/* free everything */	
	free(url_response.string);
	
	return TRUE;
}

/**
	@brief Initialize curl and do other things to prepare for the program.
	@retval A value that is equal to truthy if curl has successfully initialized, whilst falsy if not.
 */
BOOL curl_init() {
	if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
		return FALSE;
	}
	
	/* get the coords of the user for accurate weather data from their area */
	get_coordinates_from_ip(&usr_x, &usr_y);
	
	char *buffer = malloc(WEATHER_API_SIZE + 32);
	if (buffer == NULL) {
		do { 
			buffer = malloc(32 * sizeof(char)); 
		} while (buffer == NULL);
	}
	
	snprintf(buffer, WEATHER_API_SIZE + 32, WEATHER_API_SERVICE, usr_x, usr_y);
	
	#ifndef NDEBUG
		printf("%s\n", buffer);
	#endif
	
	easy_handle = curl_easy_init();
	
	if (curl_easy_setopt(easy_handle, CURLOPT_URL, buffer) != CURLE_OK) {
		return FALSE;
	}
	
	if (curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK) {
		return FALSE;
	}
	
	return TRUE;
}
 
/**
	@brief This function grabs weather data from an API, and then it deserializes and places the data into a buffer
	provided by the user.
	
	@param[out] data = This is the buffer that will receive the weather data.
	@retval A value that is equal to truthy if the function has succeeded, whilst falsy if not.
*/
BOOL get_weather_data(weather_data_t *data) {
	response_t url_response = {0};
	weather_data_t buffer = {0};
	if (curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &url_response) != CURLE_OK) {
		return FALSE;
	}
	
	/* Execute */
	curl_easy_perform(easy_handle);
	
	/* Deserialize */

	cJSON *deserialized_response = cJSON_Parse(url_response.string);

	if (deserialized_response == NULL) {

	    return FALSE;
	}

	cJSON *hourly = cJSON_GetObjectItemCaseSensitive(
	    deserialized_response,
	    "hourly"
	);

	if (hourly == NULL) {

	    cJSON_Delete(deserialized_response);

	    return FALSE;
	}

	buffer.timestamp =
	    (int64_t)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "time"),
	        0
	    )->valuedouble;

	buffer.temperature =
	    (float)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "temperature_2m"),
	        0
	    )->valuedouble;

	buffer.humidity =
	    (float)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "relative_humidity_2m"),
	        0
	    )->valuedouble;

	buffer.wind_speed =
	    (float)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "wind_speed_10m"),
	        0
	    )->valuedouble;

	buffer.wind_gusts =
	    (float)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "wind_gusts_10m"),
	        0
	    )->valuedouble;

	buffer.precipitation =
	    (float)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "precipitation"),
	        0
	    )->valuedouble;

	buffer.precipitation_probability =
	    (float)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "precipitation_probability"),
	        0
	    )->valuedouble;

	buffer.weather_code =
	    (uint8_t)cJSON_GetArrayItem(
	        cJSON_GetObjectItem(hourly, "weather_code"),
	        0
	    )->valueint;

	buffer.rain_intensity =
	    buffer.precipitation *
	    (buffer.precipitation_probability / 100.0f);

	/* storm_score is calculated later by your predictor */
	#ifndef NDEBUG
		char* debug = cJSON_Print(deserialized_response);
		printf("%s\n", debug);
	#endif
	
	cJSON_Delete(deserialized_response);
	*data = buffer;
	
	return TRUE;
}