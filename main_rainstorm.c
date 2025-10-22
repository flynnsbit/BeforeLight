#include <SDL.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt
#include <math.h>

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

int main(int argc, char *argv[]) {
    int opt;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "s:f:h")) != -1) {
        switch (opt) {
            case 's':
                speed_mult = atof(optarg);
                if (speed_mult <= 0.1f) speed_mult = 0.1f;
                if (speed_mult > 10.0f) speed_mult = 10.0f;
                break;
            case 'f':
                do_fullscreen = atoi(optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Rainstorm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
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

    // Rain droplet data - single layer, full screen
    #define MAX_DROPS 150
    struct drop {
        float x, y;
    };
    struct drop drops[MAX_DROPS];

    // Initialize drops from top
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = rand() % W;
        drops[i].y = (float)(rand() % H);  // Start at random Y from top
    }

    // Main loop
    SDL_Event e;
    int quit = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        // Update drops - continuous fall with 15 deg slant
        for (int i = 0; i < MAX_DROPS; i++) {
            drops[i].y += 8.0f * speed_mult;  // Fall speed
            drops[i].x += (int)(8.0f * speed_mult * 0.268f);  // tan(15°) ≈ 0.268 for slant

            // Respawn at top when off bottom
            if (drops[i].y > H + 20) {
                drops[i].x = rand() % W;
                drops[i].y = -10;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Draw angled rain drops (white lines from top to bottom)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
        for (int i = 0; i < MAX_DROPS; i++) {
            int length = 15;
            float tan15 = 0.268f; // tan(15°)
            int dx = (int)(length * tan15);
            SDL_RenderDrawLine(renderer, (int)drops[i].x, (int)drops[i].y,
                              (int)drops[i].x + dx, (int)drops[i].y + length);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
