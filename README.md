# No school predictor
This is my project. I will work on it through out the entire
rainy season and for now, this is a simple CLI program. Soon
I wanna make it into a GTK program with configurable settings
as most of the stuff in this code (strings to regex for no school, etc. (or i just ran out of things to say HAHAHA))

# Warning
This is only supported for windows and
linux and other os support will be added soon.

# Building
To build this project for use, first clone this repo
> git clone https://github.com/FedoraV3/noschoolpredictor.git

> cd /path/to/noschoolpredictor

After that you can build it into either an executable or install it if you want. <strong>Beware that there is no uninstaller</strong>

First configure:
> meson setup build --buildtype=release

Then finally, build
> meson compile -C build

# Setup (assuming you have the .exe compiled already)
To set this project up you must have an API key to the API service named "Apify". 
This is the vital part for news_checker.c to check if there is no school from facebook posts,
as in my area we use facebook posts to show that there is no school

You can get your API key here:
https://apify.com/apify/facebook-posts-scraper

After you have obtained your API key, create an environment
variable in Windows that is named "APIFY_API_KEY"

Next, you have to get a dataset for your place
to train the ai with. You will need to extract the weather
and then if school was suspended in your place, to find out how to make
a dataset refer to the folder "example_dataset" in the repo.

After that you should be good to go and be able to use the program. Launch it with:
> NoSchoolPredictor.exe <path/to/dataset>