/*
 * predictor.c
 *
 *  Created on: Aug 19, 2026
 *      Author: ir0n1c
 */

#include "predictor.h"

#include <math.h>
#include <stddef.h>

static float clamp_float(
    float value,
    float minimum,
    float maximum
)
{
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static float normalize(
    float value,
    float minimum,
    float maximum
)
{
    if (maximum <= minimum) {
        return 0.0f;
    }

    return clamp_float(
        (value - minimum) / (maximum - minimum),
        0.0f,
        1.0f
    );
}

static float sigmoid(float value)
{
    value = clamp_float(value, -20.0f, 20.0f);

    return 1.0f / (1.0f + expf(-value));
}

static float get_weather_code_score(uint8_t weather_code)
{
    switch (weather_code) {

        case 0:
        case 1:
            return 0.00f;

        case 2:
        case 3:
            return 0.05f;

        case 45:
        case 48:
            return 0.20f;

        case 51:
            return 0.10f;

        case 53:
            return 0.20f;

        case 55:
            return 0.30f;

        case 56:
        case 57:
            return 0.40f;

        case 61:
            return 0.25f;

        case 63:
            return 0.50f;

        case 65:
            return 0.80f;

        case 66:
        case 67:
            return 0.85f;

        case 71:
        case 73:
        case 75:
        case 77:
            return 0.40f;

        case 80:
            return 0.35f;

        case 81:
            return 0.60f;

        case 82:
            return 0.90f;

        case 85:
        case 86:
            return 0.50f;

        case 95:
            return 0.85f;

        case 96:
        case 99:
            return 1.00f;

        default:
            return 0.00f;
    }
}

static suspension_risk_t classify_probability(
    float probability
)
{
    if (probability >= 80.0f) {
        return SUSPENSION_RISK_EXTREME;
    }

    if (probability >= 60.0f) {
        return SUSPENSION_RISK_HIGH;
    }

    if (probability >= 35.0f) {
        return SUSPENSION_RISK_MODERATE;
    }

    if (probability >= 15.0f) {
        return SUSPENSION_RISK_LOW;
    }

    return SUSPENSION_RISK_VERY_LOW;
}

suspension_prediction_t predict_suspension(
    const weather_data_t *weather
)
{
    suspension_prediction_t prediction = {0};

    if (weather == NULL) {
        return prediction;
    }

    const float rain_probability_score = clamp_float(
        weather->precipitation_probability / 100.0f,
        0.0f,
        1.0f
    );

    const float precipitation_score = normalize(
        weather->precipitation,
        0.0f,
        15.0f
    );

    const float wind_speed_score = normalize(
        weather->wind_speed,
        10.0f,
        70.0f
    );

    const float wind_gust_score = normalize(
        weather->wind_gusts,
        20.0f,
        100.0f
    );

    const float humidity_score = normalize(
        weather->humidity,
        70.0f,
        100.0f
    );

    const float weather_code_score =
        get_weather_code_score(weather->weather_code);

    prediction.rain_score = clamp_float(
        rain_probability_score * 0.35f +
        precipitation_score * 0.65f,
        0.0f,
        1.0f
    );

    prediction.wind_score = clamp_float(
        wind_speed_score * 0.35f +
        wind_gust_score * 0.65f,
        0.0f,
        1.0f
    );

    prediction.storm_score = clamp_float(
        prediction.rain_score * 0.35f +
        prediction.wind_score * 0.25f +
        weather_code_score * 0.35f +
        humidity_score * 0.05f,
        0.0f,
        1.0f
    );

    const float rain_wind_interaction =
        prediction.rain_score *
        prediction.wind_score;

    const float thunderstorm_interaction =
        weather_code_score *
        rain_probability_score;

    const float raw_score =
        -4.10f +
        rain_probability_score * 1.20f +
        precipitation_score * 2.40f +
        wind_speed_score * 0.80f +
        wind_gust_score * 1.40f +
        humidity_score * 0.35f +
        weather_code_score * 1.90f +
        rain_wind_interaction * 1.25f +
        thunderstorm_interaction * 1.10f;

    prediction.probability =
        sigmoid(raw_score) * 100.0f;

    prediction.probability = clamp_float(
        prediction.probability,
        0.0f,
        100.0f
    );

    prediction.risk =
        classify_probability(prediction.probability);

    return prediction;
}

suspension_prediction_t predict_suspension_period(
    const weather_data_t *forecast,
    size_t forecast_count
)
{
    suspension_prediction_t final_prediction = {0};

    if (forecast == NULL || forecast_count == 0) {
        return final_prediction;
    }

    float highest_probability = 0.0f;
    float average_probability = 0.0f;
    size_t highest_index = 0;

    for (size_t i = 0; i < forecast_count; i++) {

        suspension_prediction_t hourly_prediction =
            predict_suspension(&forecast[i]);

        average_probability +=
            hourly_prediction.probability;

        if (hourly_prediction.probability >
            highest_probability) {

            highest_probability =
                hourly_prediction.probability;

            highest_index = i;
        }
    }

    average_probability /= (float)forecast_count;

    final_prediction =
        predict_suspension(&forecast[highest_index]);

    final_prediction.probability =
        highest_probability * 0.75f +
        average_probability * 0.25f;

    final_prediction.probability = clamp_float(
        final_prediction.probability,
        0.0f,
        100.0f
    );

    final_prediction.risk =
        classify_probability(
            final_prediction.probability
        );

    return final_prediction;
}

const char *suspension_risk_to_string(
    suspension_risk_t risk
)
{
    switch (risk) {

        case SUSPENSION_RISK_VERY_LOW:
            return "very low";

        case SUSPENSION_RISK_LOW:
            return "low";

        case SUSPENSION_RISK_MODERATE:
            return "moderate";

        case SUSPENSION_RISK_HIGH:
            return "high";

        case SUSPENSION_RISK_EXTREME:
            return "extreme";

        default:
            return "unknown";
    }
}