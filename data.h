/*
 * curl.h
 *
 *  Created on: Aug 18, 2026
 *      Author: ir0n1c
 */

#ifndef DATA_H_
#define DATA_H_

#include <curl/curl.h>
#include <cjson/cJSON.h>

#include <Windows.h>



typedef struct WEATHER_DATA {
    /* Open-Meteo data */
    int64_t timestamp;
    float temperature;
    float humidity;
    float pressure_msl;
    float wind_speed;
    float wind_gusts;
    float precipitation;
    float precipitation_probability;
    uint8_t weather_code;
    BOOL is_day;

    /* Derived values */
    float feels_like;
    float rain_intensity;
    float storm_score;
} weather_data_t;

typedef struct str {
	/* on the heap */
	char* string;
	size_t len;
	size_t capacity;
} response_t;

extern CURL *easy_handle;

/* function prototypes */

/**
	@brief Initialize curl and do other things to prepare for the program.
	@retval A value that is equal to truthy if curl has successfully initialized, whilst falsy if not.
 */
BOOL curl_init();

/**
	@brief This function grabs weather data from an API, and then it deserializes and places the data into a buffer
	provided by the user.
	
	@param[out] data = This is the buffer that will receive the weather data.
	@retval A value that is equal to truthy if the function has succeeded, whilst falsy if not.
*/
BOOL get_weather_data(weather_data_t *data);


#endif /* DATA_H_ */
