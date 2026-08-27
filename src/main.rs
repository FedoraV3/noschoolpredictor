mod ai;
mod data;

use crate::ai::tree::{load_ai, make_prediction};
use std::env;
use std::sync::OnceLock;

pub static APIFY_KEY: OnceLock<String> = OnceLock::new();

#[tokio::main]
async fn main() {
    /* it's a very good thing if this fails and panics the program */
    APIFY_KEY.set(env::var("APIFY_API_KEY").unwrap()).unwrap();
    load_ai("training.csv")
        .await
        .unwrap();
    let closure_probability = make_prediction().await.unwrap();
    println!("school closure probability: {closure_probability:.1}%");
}
