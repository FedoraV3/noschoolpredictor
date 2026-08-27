mod ai;
mod data;

use crate::ai::tree::{load_ai, make_prediction};
use std::env::{args, var};
use std::sync::OnceLock;
use anyhow::Result;

pub static APIFY_KEY: OnceLock<String> = OnceLock::new();

#[tokio::main]
async fn main() -> Result<()> {
    /* it's a very good thing if this fails and panics the program */
    APIFY_KEY.set(var("APIFY_API_KEY").unwrap()).unwrap();

    let prog_args = args().collect::<Vec<String>>();
    if prog_args.len() < 2 {
        panic!("Please specify a path into the dataset file you want to train the AI with.")
    }

    // just need the first so lets do that
    load_ai(prog_args.get(1).unwrap()).await?;
    println!("Chance of no school tomorrow: {:.2}%", make_prediction().await?);

    Ok(())
}
