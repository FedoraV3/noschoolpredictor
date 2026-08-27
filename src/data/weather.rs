use anyhow::Result;
use openmeteo_rs::{Client, ForecastResponse, HourlyVar};
use reqwest;
static IP_ADDR_API_URL: &str = "https://ipapi.co/latlong/";

struct Coordinates {
    latitude: f64,
    longitude: f64,
}

async fn get_user_coords() -> Result<Coordinates> {
    let response = reqwest::get(IP_ADDR_API_URL)
        .await?
        .text()
        .await?;

    let (lat, lon) = response
        .trim()
        .split_once(",")
        .expect("expected latitude,longitude");

    let coords_latitude = lat.parse::<f64>()?;
    let coords_longitude = lon.parse::<f64>()?;

    Ok(Coordinates {
        latitude: coords_latitude,
        longitude: coords_longitude,
    })
}

pub async fn get_weather_data() -> Result<ForecastResponse> {
    let coords = get_user_coords().await?;

    /* sent the request */
    let weather_client = Client::new()
        .forecast(coords.latitude, coords.longitude)
        .hourly([HourlyVar::Temperature2m, HourlyVar::RelativeHumidity2m,
                         HourlyVar::Precipitation, HourlyVar::PrecipitationProbability,
                         HourlyVar::WeatherCode, HourlyVar::SurfacePressure,
                         HourlyVar::Visibility, HourlyVar::CloudCover,
                         HourlyVar::WindSpeed10m])
        .forecast_days(1)
        .send()
        .await?;

    Ok(weather_client)
}
