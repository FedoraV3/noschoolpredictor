#include "curl_m.h"
#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "include/data.h"
}

class WeatherDataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        curl_mock::reset();
        data_easy_handle = nullptr;
    }

    void initialize()
    {
        curl_mock::set_response("14.5995,120.9842");
        ASSERT_TRUE(data_curl_init());
    }
};

TEST_F(WeatherDataTest, InitializationBuildsWeatherUrlFromCoordinates)
{
    initialize();
    EXPECT_NE(data_easy_handle, nullptr);
    EXPECT_NE(std::strstr(curl_mock::url(), "latitude=14.599500"), nullptr);
    EXPECT_NE(std::strstr(curl_mock::url(), "longitude=120.984200"), nullptr);
}

TEST_F(WeatherDataTest, ParsesWeatherResponseAndDerivesRainIntensity)
{
    initialize();
    curl_mock::set_response(
        "{\"hourly\":{\"time\":[1700000000],\"temperature_2m\":[29.5],"
        "\"relative_humidity_2m\":[88],\"wind_speed_10m\":[31.5],"
        "\"wind_gusts_10m\":[52],\"precipitation\":[8],"
        "\"precipitation_probability\":[75],\"weather_code\":[95]}}");

    weather_data_t weather = {};
    ASSERT_TRUE(get_weather_data(&weather));
    EXPECT_EQ(weather.timestamp, 1700000000);
    EXPECT_FLOAT_EQ(weather.temperature, 29.5f);
    EXPECT_FLOAT_EQ(weather.humidity, 88.0f);
    EXPECT_FLOAT_EQ(weather.rain_intensity, 6.0f);
    EXPECT_EQ(weather.weather_code, 95);
}

TEST_F(WeatherDataTest, RejectsMalformedWeatherResponse)
{
    initialize();
    curl_mock::set_response("not-json");
    weather_data_t weather = {};
    EXPECT_FALSE(get_weather_data(&weather));
}
