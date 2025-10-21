#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

#define PI 3.14159f

#define WINDOW_WIDTH 0  // fullscreen
#define WINDOW_HEIGHT 0
#define SPRITE_SIZE 145
#define FISH_FRAME_COUNT 4  // not used

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

// Single globe texture

SDL_Texture *globe_tex;

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

    // Removed setenv for style testing
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Globe", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
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

    // Load globe texture
    SDL_Surface *surf = IMG_Load("img/globe_240.jpg");
    if (!surf) {
        SDL_Log("Error loading globe_240.jpg: %s", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    globe_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    // Initialize globe physics
    float x = 100, y = 100, vx = 200, vy = 150;
    int ball_size = 240;

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Update physics
        const float dt = 0.016f;
        x += vx * dt * speed_mult;
        y += vy * dt * speed_mult;

        // Wall collisions
        if (x < 0) { x = 0; vx = -vx; }
        else if (x > W - ball_size) { x = W - ball_size; vx = -vx; }
        if (y < 0) { y = 0; vy = -vy; }
        else if (y > H - ball_size) { y = H - ball_size; vy = -vy; }

        // Render static globe
        SDL_Rect src_rect = {0, 0, 240, 240};
        SDL_Rect dst_rect = {(int)x, (int)y, 240, 240};
        SDL_RenderCopy(renderer, globe_tex, &src_rect, &dst_rect);



        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyTexture(globe_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
