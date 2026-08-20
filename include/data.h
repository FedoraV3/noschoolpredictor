#ifndef DATA_H_
#define DATA_H_

#include <stddef.h>
#include <stdint.h>

typedef void CURL;

#ifdef _WIN32
#include <windows.h>
#else
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif

/** @brief Weather measurements used by the suspension predictor. */
typedef struct WEATHER_DATA {

    /* Open-Meteo data */
    int64_t timestamp;
    float temperature;
    float humidity;
    float wind_speed;
    float wind_gusts;
    float precipitation;
    float precipitation_probability;
    uint8_t weather_code;

    /* Derived values */
    float rain_intensity;
    float storm_score;

} weather_data_t;

/** @brief Growable response buffer used by libcurl callbacks. */
typedef struct str {
	/* on the heap */
	char* string;
	size_t len;
	size_t capacity;
} response_t;

extern CURL *data_easy_handle;

/* function prototypes */

/**
 * @brief Initializes the weather-data HTTP client.
 *
 * Determines the user's coordinates, builds the weather API request URL, and
 * configures the CURL handle used by get_weather_data().
 *
 * @retval TRUE The client was initialized successfully.
 * @retval FALSE Memory allocation or CURL configuration failed.
 */
BOOL data_curl_init(void);

/**
 * @brief Fetches and deserializes the current weather data.
 *
 * @param[out] data Destination for the weather measurements returned by the
 *                  configured API.
 * @retval TRUE Weather data was fetched and parsed successfully.
 * @retval FALSE The request, response parsing, or input validation failed.
 * @pre data_curl_init() must have completed successfully.
 */
BOOL get_weather_data(weather_data_t *data);


#endif /* DATA_H_ */
