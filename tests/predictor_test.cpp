#include <gtest/gtest.h>

extern "C" {
#include "include/predictor.h"
}

TEST(Predictor, NullWeatherProducesZeroPrediction)
{
    const suspension_prediction_t result = predict_suspension(nullptr);
    EXPECT_FLOAT_EQ(result.probability, 0.0f);
    EXPECT_EQ(result.risk, SUSPENSION_RISK_VERY_LOW);
}

TEST(Predictor, CalmWeatherHasVeryLowRisk)
{
    const weather_data_t weather = {};
    const suspension_prediction_t result = predict_suspension(&weather);
    EXPECT_LT(result.probability, 15.0f);
    EXPECT_EQ(result.risk, SUSPENSION_RISK_VERY_LOW);
}

TEST(Predictor, SevereStormHasExtremeRisk)
{
    weather_data_t weather = {};
    weather.humidity = 100.0f;
    weather.wind_speed = 70.0f;
    weather.wind_gusts = 100.0f;
    weather.precipitation = 15.0f;
    weather.precipitation_probability = 100.0f;
    weather.weather_code = 99;

    const suspension_prediction_t result = predict_suspension(&weather);
    EXPECT_GT(result.probability, 80.0f);
    EXPECT_EQ(result.risk, SUSPENSION_RISK_EXTREME);
    EXPECT_FLOAT_EQ(result.rain_score, 1.0f);
    EXPECT_FLOAT_EQ(result.wind_score, 1.0f);
}

TEST(Predictor, PeriodWeightsTheWorstHour)
{
    weather_data_t forecast[2] = {};
    forecast[1].humidity = 100.0f;
    forecast[1].wind_speed = 70.0f;
    forecast[1].wind_gusts = 100.0f;
    forecast[1].precipitation = 15.0f;
    forecast[1].precipitation_probability = 100.0f;
    forecast[1].weather_code = 99;

    const float calm = predict_suspension(&forecast[0]).probability;
    const float severe = predict_suspension(&forecast[1]).probability;
    const suspension_prediction_t result = predict_suspension_period(forecast, 2);

    EXPECT_FLOAT_EQ(result.probability, severe * 0.875f + calm * 0.125f);
    EXPECT_GT(result.probability, calm);
    EXPECT_LT(result.probability, severe);
}

TEST(Predictor, RiskNamesIncludeUnknownValues)
{
    EXPECT_STREQ(suspension_risk_to_string(SUSPENSION_RISK_HIGH), "high");
    EXPECT_STREQ(suspension_risk_to_string(static_cast<suspension_risk_t>(99)), "unknown");
}
