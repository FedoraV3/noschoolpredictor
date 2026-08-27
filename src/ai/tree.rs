use crate::data::weather::get_weather_data;
use anyhow::{Result, anyhow};
use linfa::prelude::*;
use linfa_trees::{DecisionTree, SplitQuality};
use ndarray::{Array1, Array2};
use openmeteo_rs::HourlyData;
use std::sync::OnceLock;

const FEATURE_COUNT: usize = 9;
const TREE_COUNT: usize = 101;
const FEATURES_PER_TREE: usize = 3;

struct ForestTree {
    model: DecisionTree<f64, usize>,
    features: [usize; FEATURES_PER_TREE],
}
static TRAINED_MODEL: OnceLock<Vec<ForestTree>> = OnceLock::new();

fn series(hourly: &HourlyData, name: &str) -> Result<Vec<f64>> {
    hourly
        .get(name)
        .ok_or_else(|| anyhow!("missing hourly variable: {name}"))?
        .values_f32()
        .ok_or_else(|| anyhow!("{name} is not numeric"))?
        .iter()
        .enumerate()
        .map(|(hour, value)| {
            value
                .map(f64::from)
                .ok_or_else(|| anyhow!("{name} is missing hour {hour}"))
        })
        .collect()
}

fn mean(values: &[f64], name: &str) -> Result<f64> {
    if values.is_empty() {
        return Err(anyhow!("{name} has no values"));
    }
    Ok(values.iter().sum::<f64>() / values.len() as f64)
}

fn maximum(values: &[f64], name: &str) -> Result<f64> {
    values
        .iter()
        .copied()
        .reduce(f64::max)
        .ok_or_else(|| anyhow!("{name} has no values"))
}

fn weather_features(response: openmeteo_rs::ForecastResponse) -> Result<[f64; FEATURE_COUNT]> {
    let hourly = response
        .hourly
        .as_ref()
        .ok_or_else(|| anyhow!("weather response has no hourly data"))?;
    let temperature = series(hourly, "temperature_2m")?;
    let humidity = series(hourly, "relative_humidity_2m")?;
    let precipitation = series(hourly, "precipitation")?;
    let precipitation_probability = series(hourly, "precipitation_probability")?;
    let weather_code = series(hourly, "weather_code")?;
    let pressure = series(hourly, "surface_pressure")?;
    let visibility = series(hourly, "visibility")?;
    let cloud_cover = series(hourly, "cloud_cover")?;
    let wind_speed = series(hourly, "wind_speed_10m")?;

    Ok([
        mean(&temperature, "temperature_2m")?,
        mean(&humidity, "relative_humidity_2m")?,
        precipitation.iter().sum(),
        maximum(&precipitation_probability, "precipitation_probability")?,
        maximum(&weather_code, "weather_code")?,
        mean(&pressure, "surface_pressure")?,
        mean(&visibility, "visibility")?,
        mean(&cloud_cover, "cloud_cover")?,
        maximum(&wind_speed, "wind_speed_10m")?,
    ])
}

#[derive(Debug, serde::Deserialize)]
pub struct TrainingRow {
    pub temperature_2m_mean: f64,
    pub relative_humidity_2m_mean: f64,
    pub precipitation_sum: f64,
    pub precipitation_probability_max: f64,
    pub weather_code: f64,
    pub surface_pressure_mean: f64,
    pub visibility_mean: f64,
    pub cloud_cover_mean: f64,
    pub wind_speed_10m_max: f64,
    pub suspended: usize,
}

impl TrainingRow {
    fn features(&self) -> [f64; FEATURE_COUNT] {
        [
            self.temperature_2m_mean,
            self.relative_humidity_2m_mean,
            self.precipitation_sum,
            self.precipitation_probability_max,
            self.weather_code,
            self.surface_pressure_mean,
            self.visibility_mean,
            self.cloud_cover_mean,
            self.wind_speed_10m_max,
        ]
    }
}

fn random(state: &mut u64, upper: usize) -> usize {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    (*state as usize) % upper
}

fn random_features(state: &mut u64) -> [usize; FEATURES_PER_TREE] {
    let mut available: Vec<usize> = (0..FEATURE_COUNT).collect();
    for index in 0..FEATURES_PER_TREE {
        let selected = index + random(state, FEATURE_COUNT - index);
        available.swap(index, selected);
    }
    [available[0], available[1], available[2]]
}

pub async fn load_ai(file_path: &str) -> Result<()> {
    let rows: Vec<TrainingRow> = csv::Reader::from_path(file_path)?
        .deserialize()
        .collect::<std::result::Result<_, _>>()?;
    if rows.is_empty() {
        return Err(anyhow!("training csv contains no rows"));
    }
    if rows.iter().any(|row| row.suspended > 1) {
        return Err(anyhow!("suspended labels must be 0 or 1"));
    }

    let mut state = 0x4d59_5df4_d0f3_3173_u64;
    let mut forest = Vec::with_capacity(TREE_COUNT);
    for i in 0..TREE_COUNT {
        let features = random_features(&mut state);
        let mut samples = Vec::with_capacity(rows.len() * FEATURES_PER_TREE);
        let mut labels = Vec::with_capacity(rows.len());
        for _ in 0..rows.len() {
            let row = &rows[random(&mut state, rows.len())];
            let values = row.features();
            samples.extend(features.map(|feature| values[feature]));
            labels.push(row.suspended);
        }
        let (training, dataset) = Dataset::new(
            Array2::from_shape_vec((rows.len(), FEATURES_PER_TREE), samples)?,
            Array1::from(labels),
        ).split_with_ratio(0.8);
        let model = DecisionTree::params()
            .split_quality(SplitQuality::Gini)
            .max_depth(Some(10))
            .min_weight_split(2.0)
            .min_weight_leaf(1.0)
            .fit(&training)?;

        // train your dragon
        if cfg!(debug_assertions) {
            let pred = model.predict(&dataset);
            let confusion_matrix = pred.confusion_matrix(&dataset)?;
            println!("tree no {i} dataset results: \nAccuracy: {:.2}%", confusion_matrix.accuracy() * 100.0);
        }

        forest.push(ForestTree { model, features });
    }

    // test prediction


    TRAINED_MODEL
        .set(forest)
        .map_err(|_| anyhow!("random forest has already been trained"))
}

/// Returns the percentage of trees voting for school closure (0.0..=100.0).
pub async fn make_prediction() -> Result<f64> {
    let weather = weather_features(get_weather_data().await?)?;
    let forest = TRAINED_MODEL
        .get()
        .ok_or_else(|| anyhow!("random forest has not been trained"))?;
    let mut closure_votes = 0;
    for tree in forest {
        let input = Array2::from_shape_vec(
            (1, FEATURES_PER_TREE),
            tree.features.map(|feature| weather[feature]).to_vec(),
        )?;
        if tree.model.predict(&input)[0] == 1 {
            closure_votes += 1;
        }
    }
    Ok(closure_votes as f64 * 100.0 / forest.len() as f64)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[tokio::test]
    async fn load_ai_fails_on_bad_input() {
        assert!(load_ai("this-file-does-not-exist.csv").await.is_err());
    }
}
