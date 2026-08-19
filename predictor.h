/*
 * predictor.h
 *
 *  Created on: Aug 19, 2026
 *      Author: ir0n1c
 */

#ifndef PREDICTOR_H_
#define PREDICTOR_H_

#include "data.h"

typedef enum SUSPENSION_RISK {

    SUSPENSION_RISK_VERY_LOW,
    SUSPENSION_RISK_LOW,
    SUSPENSION_RISK_MODERATE,
    SUSPENSION_RISK_HIGH,
    SUSPENSION_RISK_EXTREME

} suspension_risk_t;

typedef struct SUSPENSION_PREDICTION {

    float probability;

    float rain_score;
    float wind_score;
    float storm_score;

    suspension_risk_t risk;

} suspension_prediction_t;

suspension_prediction_t predict_suspension(
    const weather_data_t *weather
);

suspension_prediction_t predict_suspension_period(
    const weather_data_t *forecast,
    size_t forecast_count
);

const char *suspension_risk_to_string(
    suspension_risk_t risk
);

#endif // PREDICTOR_H_