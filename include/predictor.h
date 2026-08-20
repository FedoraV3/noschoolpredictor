#ifndef PREDICTOR_H_
#define PREDICTOR_H_

#include "data.h"

/** @brief Qualitative risk levels for a school suspension. */
typedef enum SUSPENSION_RISK {

    SUSPENSION_RISK_VERY_LOW,
    SUSPENSION_RISK_LOW,
    SUSPENSION_RISK_MODERATE,
    SUSPENSION_RISK_HIGH,
    SUSPENSION_RISK_EXTREME

} suspension_risk_t;

/** @brief Probability and component scores produced by the predictor. */
typedef struct SUSPENSION_PREDICTION {

    float probability;

    float rain_score;
    float wind_score;
    float storm_score;

    suspension_risk_t risk;

} suspension_prediction_t;

/**
 * @brief Predicts suspension risk from one weather sample.
 *
 * @param weather Weather sample to evaluate; may be NULL.
 * @return The calculated prediction, or a zeroed prediction for NULL input.
 */
suspension_prediction_t predict_suspension(
    const weather_data_t *weather
);

/**
 * @brief Predicts suspension risk over a forecast period.
 *
 * @param forecast Array of weather samples; may be NULL.
 * @param forecast_count Number of samples in @p forecast.
 * @return A prediction weighted toward the highest-risk sample.
 */
suspension_prediction_t predict_suspension_period(
    const weather_data_t *forecast,
    size_t forecast_count
);

/**
 * @brief Converts a suspension risk value to readable text.
 *
 * @param risk Risk value to convert.
 * @return A static lowercase string describing the risk.
 */
const char *suspension_risk_to_string(
    suspension_risk_t risk
);

#endif // PREDICTOR_H_
