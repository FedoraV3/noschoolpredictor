/*
 * curl.c
 *
 *  Created on: Aug 18, 2026
 *      Author: ir0n1c
 */

#include "data.h"
#include "curl/curl.h"
#include "curl/easy.h"
#include <stdlib.h>

/* global variables for cURL */
/* this will be the handle used for the sessions with the weather api or it will be one shot soon */
CURL *easy_handle;

/* it must return in this format: x coords,y coords*/
static const char* IP_API_SERVICE = "https://ipconfig.io/coordinates";
static const char *WEATHER_API_SERVICE =
    "https://api.open-meteo.com/v1/forecast?"
    "latitude=%f&longitude=%f"
    "&hourly="
    "temperature_2m,"
    "precipitation_probability,"
    "precipitation,"
    "rain,"
    "showers,"
    "weather_code,"
    "wind_speed_10m,"
    "wind_gusts_10m,"
    "cloud_cover,"
    "visibility,"
    "surface_pressure,"
    "relative_humidity_2m";

static float usr_x, usr_y;

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	#ifndef NDEBUG
		printf("response size: %zu\n", size * nmemb);
	#endif
	
	size_t real_size = nmemb * size;

	response_t *p_response = (response_t*)userdata;
	p_response->capacity = real_size * sizeof(char);
	p_response->string = malloc(real_size * sizeof(char));
	if (p_response->string == NULL) {
		do {
			/* spam spam spam spam */
			p_response->string = malloc(real_size * sizeof(char));
		} while (p_response->string == NULL);
	}
	
	memcpy(p_response->string, ptr, real_size);
	
	p_response->len = real_size;
	
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
	
	#ifndef NDEBUG
		if (curl_easy_setopt(ip_handle, CURLOPT_VERBOSE, 1L) != CURLE_OK) {
			printf("Failed to set option for verbose! It might work but we will have no output\n");
		}
	#endif
	
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
	
	char *buffer = malloc(32 * sizeof(char));
	if (buffer == NULL) {
		do { 
			buffer = malloc(32 * sizeof(char)); 
		} while (buffer == NULL);
	}
	
	sprintf(buffer, WEATHER_API_SERVICE, usr_x, usr_y);
	
	easy_handle = curl_easy_init();
	
	if (curl_easy_setopt(easy_handle, CURLOPT_URL, &buffer) != CURLE_OK) {
		return FALSE;
	}
	
	if (curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK) {
		return FALSE;
	}
	

	
	return TRUE;
}
 
