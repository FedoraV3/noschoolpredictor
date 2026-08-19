// Simple Hello World program
#include <stdio.h>
#include <curl/curl.h>

#include "data.h"
#include "predictor.h"

int main(int argc, char **argv) {
	/* Initialize libcurl */
	if (curl_init() != TRUE) {
		printf("curl_init has failed\n");
		return 0;
	}
	
	weather_data_t weather_data;
	if (get_weather_data(&weather_data) != TRUE) {
		printf("get_weather_data has failed\n");
		return 0;
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
