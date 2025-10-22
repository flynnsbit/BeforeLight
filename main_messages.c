#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt
#include <stdio.h>
#include <string.h>

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -t STR  Message text (default: 'OUT TO LUNCH')\n");
    fprintf(stderr, "  -r      Random quote from internet (requires curl)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

int main(int argc, char *argv[]) {
    int opt;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;
    char message_text[1024] = "OUT TO LUNCH";
    int random_mode = 0;
    const char *message = message_text;

    while ((opt = getopt(argc, argv, "s:f:t:rh")) != -1) {
        switch (opt) {
            case 's':
                speed_mult = atof(optarg);
                if (speed_mult <= 0.1f) speed_mult = 0.1f;
                if (speed_mult > 10.0f) speed_mult = 10.0f;
                break;
            case 'f':
                do_fullscreen = atoi(optarg);
                break;
            case 't':
                strcpy(message_text, optarg);
                break;
            case 'r':
                random_mode = 1;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Removed setenv for style testing
    srand(time(NULL));

    if (random_mode) {
        FILE *fp = popen("curl -s http://api.quotable.io/random | sed 's/.*\"content\":\"//' | sed 's/\",\"author.*//'", "r");
        if (fp) {
            if (fgets(message_text, sizeof(message_text), fp)) {
                char *newline = strchr(message_text, '\n');
                if (newline) *newline = '\0';
                if (strlen(message_text) > 0) {
                    message = message_text;
                }
            }
            int status = pclose(fp);
            if (status != 0) {
                // Command failed, keep default
                message = message_text;
            }
        } else {
            // popen failed, keep default
            message = message_text;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Messages", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (do_fullscreen) {
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
            SDL_Log("Warning: Failed to set fullscreen: %s", SDL_GetError());
        }
    }

    int W, H;
    SDL_GetRendererOutputSize(renderer, &W, &H);

    // Load font
    TTF_Font *font = NULL;
    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf"
    };
    for (size_t i = 0; i < sizeof(font_paths) / sizeof(char*); i++) {
        font = TTF_OpenFont(font_paths[i], 20);
        if (font) break;
    }
    if (!font) {
        SDL_Log("Error: Could not load a system font. Install SDL_ttf compatible fonts.");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture *text_texture = NULL;
    int text_w = 0, text_h = 0;

    // Function to update text display
    void update_text_texture(const char *text) {
        // Clear any existing texture
        if (text_texture) {
            SDL_DestroyTexture(text_texture);
            text_texture = NULL;
        }

        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *surface = TTF_RenderText_Blended(font, text, white);
        if (!surface) {
            SDL_Log("TTF_RenderText_Blended Error: %s", TTF_GetError());
            return;
        }

        text_texture = SDL_CreateTextureFromSurface(renderer, surface);
        text_w = surface->w;
        text_h = surface->h;
        SDL_FreeSurface(surface);
    }

    // Initial text render
    update_text_texture(message);

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float time_s = (current_time - start_time) / 1000.0f;

        // Update quote every 10 seconds (after each marquee cycle) if random mode
        if (random_mode && fmodf(time_s, 10.0f) < 0.016f) {
            FILE *fp = popen("curl -s http://api.quotable.io/random | sed 's/.*\"content\":\"//' | sed 's/\",\"author.*//'", "r");
            if (fp) {
                if (fgets(message_text, sizeof(message_text), fp)) {
                    char *newline = strchr(message_text, '\n');
                    if (newline) *newline = '\0';
                    if (strlen(message_text) > 0 && strcmp(message_text, "OUT TO LUNCH") != 0) {
                        update_text_texture(message_text);
                    }
                }
                pclose(fp);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Animate horizontal marquee (text scrolls completely off screen)
        float cycle = fmodf(time_s, 10.0f);
        float progress = cycle / 10.0f; // 0 to 1
        int dst_x = (int)(W - (W + text_w) * progress); // Complete off-screen exit at progress=1

        // Animate vertical steps
        float step_cycle = fmodf(time_s, 30.0f);
        int step = (int)floor(step_cycle / (30.0f / 3.0f)); // 0, 1, 2
        float y_pct = 0.2 + (0.8 / 3.0f) * step;

        int dst_y = (int)(y_pct * H) - text_h / 2;

        SDL_Rect dst_rect = {dst_x, dst_y, text_w, text_h};

        // Only render if text is not completely off-screen left (whole text visible when right edge > 0)
        if (dst_x + text_w > 0 && dst_x < W) {
            SDL_RenderCopy(renderer, text_texture, NULL, &dst_rect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyTexture(text_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
