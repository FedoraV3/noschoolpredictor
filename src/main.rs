mod ai;
mod data;

use crate::ai::tree::{load_ai, make_prediction};
use std::sync::OnceLock;
use anyhow::Result;
use pancurses::{curs_set, init_pair, initscr, newwin, noecho, start_color, ColorPair, Input, COLOR_BLACK, COLOR_BLUE, COLOR_PAIR, COLOR_WHITE};

pub static APIFY_KEY: OnceLock<String> = OnceLock::new();

#[tokio::main]
async fn main() -> Result<()> {
    /* it's a very good thing if this fails and panics the program */
    let mut ai_loaded = false;
    let screen = initscr();

    start_color();
    noecho();
    curs_set(1);
    screen.keypad(true);

    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLUE, COLOR_WHITE);

    let (max_y, max_x) = screen.get_max_yx();
    let win = newwin(max_y - 4, max_x - 4, 2, 2);

    win.bkgd(COLOR_PAIR(1));
    win.clear();
    win.border('|', '|', '-', '-', '+', '+', '+', '+');

    let mut text = String::new();
    let mut button_selected = false;

    loop {
        // textbox
        win.attron(ColorPair(1));
        win.mvprintw(2, 2, "dataset path:");
        win.mvprintw(3, 2, format!("{}", text));
        win.clrtoeol();

        // button
        if button_selected {
            win.attron(ColorPair(2));
        } else {
            win.attron(ColorPair(1));
        }

        win.mvprintw(5, 2, "[predict]");
        win.attroff(ColorPair(2));

        // place cursor inside textbox
        if !button_selected {
            win.mv(3, 2 + text.len() as i32);
        }

        win.refresh();

        match win.getch() {
            Some(Input::Character('\t')) => {
                button_selected = !button_selected;
            }

            Some(Input::Character('\n')) if button_selected => {
                win.mv(7, 2);
                win.clrtoeol();
                win.mvprintw(7, 2, "no school chance: ...");
                win.refresh();

                if !ai_loaded {
                    match load_ai(&text).await {
                        Ok(_) => {
                            ai_loaded = true;

                            match make_prediction().await {
                                Ok(r) => {
                                    win.mv(7, 2);
                                    win.clrtoeol();
                                    win.mvprintw(
                                        7,
                                        2,
                                        format!("no school chance: {:.2}%", r),
                                    );
                                }

                                Err(e) => {
                                    win.mv(7, 2);
                                    win.clrtoeol();
                                    win.mvprintw(
                                        7,
                                        2,
                                        format!("prediction failed: {e}"),
                                    );
                                }
                            }
                        }

                        Err(e) => {
                            ai_loaded = false;

                            let file_not_found = e.chain().any(|cause| {
                                cause
                                    .downcast_ref::<std::io::Error>()
                                    .is_some_and(|io_error| {
                                        io_error.kind() == std::io::ErrorKind::NotFound
                                    })
                            });

                            win.mv(7, 2);
                            win.clrtoeol();

                            if file_not_found {
                                win.mvprintw(7, 2, "dataset file not found");
                            } else {
                                win.mvprintw(
                                    7,
                                    2,
                                    format!("failed to load dataset: {e}"),
                                );
                            }
                        }
                    }
                } else {
                    match make_prediction().await {
                        Ok(r) => {
                            win.mvprintw(
                                7,
                                2,
                                format!("no school chance: {:.2}%", r),
                            );
                        }

                        Err(e) => {

                            win.mvprintw(
                                7,
                                2,
                                format!("prediction failed: {e}"),
                            );
                        }
                    }
                }

                win.refresh();
            }

            Some(Input::Character('\u{8}'))
            | Some(Input::Character('\u{7f}'))
            | Some(Input::KeyBackspace) if !button_selected => {
                text.pop();
            }

            Some(Input::Character(character))
            if !button_selected && !character.is_control() => {
                    text.push(character);
            }

            Some(Input::Character('\u{1b}')) => break,

            _ => {}
        }
    }

    Ok(())
}
