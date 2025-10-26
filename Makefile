CC = gcc
CFLAGS = -Wall -Wextra -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lm

fishsaver: main_fish.c
	$(CC) $(CFLAGS) -o build/fishsaver main_fish.c $(LDFLAGS)

hardrain: main_hard_rain.c
	$(CC) $(CFLAGS) -o build/hardrain main_hard_rain.c $(LDFLAGS)

bouncingball: main_bouncing_ball.c
	$(CC) $(CFLAGS) -o build/bouncingball main_bouncing_ball.c $(LDFLAGS)

globe: main_globe.c
	$(CC) $(CFLAGS) -o build/globe main_globe.c $(LDFLAGS)

warp: main_warp.c
	$(CC) $(CFLAGS) -o build/warp main_warp.c $(LDFLAGS)

toastersaver: main_toaster.c
	$(CC) $(CFLAGS) -o build/toastersaver main_toaster.c $(LDFLAGS)

messages: main_messages.c
	$(CC) $(CFLAGS) -o build/messages main_messages.c $(LDFLAGS) -lSDL2_ttf

messages2: main_messages2.c
	$(CC) $(CFLAGS) -o build/messages2 main_messages2.c $(LDFLAGS) -lSDL2_ttf

logo: main_logo.c
	$(CC) $(CFLAGS) -o build/logo main_logo.c $(LDFLAGS)

rainstorm: main_rainstorm.c
	$(CC) $(CFLAGS) -o build/rainstorm main_rainstorm.c $(LDFLAGS)

spotlight: main_spotlight.c
	$(CC) $(CFLAGS) -o build/spotlight main_spotlight.c $(LDFLAGS)

lifeforms: main_lifeforms.c
	$(CC) $(CFLAGS) -o build/lifeforms main_lifeforms.c $(LDFLAGS)

fadeout: main_fadeout.c
	$(CC) $(CFLAGS) -o build/fadeout main_fadeout.c $(LDFLAGS) -lX11

matrix: main_matrix.c
	$(CC) $(CFLAGS) -o build/matrix main_matrix.c $(LDFLAGS) -lSDL2_ttf

randomizer: main_randomizer.c
	$(CC) $(CFLAGS) -o build/randomizer main_randomizer.c $(LDFLAGS) -lSDL2_ttf

paperfire: main_paperfire.c
	$(CC) $(CFLAGS) -o build/paperfire main_paperfire.c $(LDFLAGS)

cityscape: main_cityscape.c
	$(CC) $(CFLAGS) -o build/cityscape main_cityscape.c $(LDFLAGS)

screensaver_config: screensaver_config.c
	$(CC) -Wall -Wextra -O2 -o build/screensaver_config screensaver_config.c -lncurses -lm

all: fishsaver hardrain bouncingball globe warp toastersaver messages messages2 logo rainstorm spotlight lifeforms fadeout matrix randomizer paperfire cityscape screensaver_config

clean:
	rm -f build/*

.PHONY: clean all
