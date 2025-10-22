#include <SDL.h>
#include <SDL_image.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt

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

    if (!(IMG_Init(IMG_INIT_PNG))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Change to img directory
    chdir("..");
    chdir("img");

    SDL_Window *window = SDL_CreateWindow("Rainstorm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    // Load rain layer textures
    SDL_Surface *surf_near = IMG_Load("rain-tile-near.png");
    SDL_Surface *surf_mid = IMG_Load("rain-tile-mid.png");
    SDL_Surface *surf_far = IMG_Load("rain-tile-distant.png");

    int tile_w = surf_near->w, tile_h = surf_near->h;
    SDL_Texture *tex_near = SDL_CreateTextureFromSurface(renderer, surf_near);
    SDL_Texture *tex_mid = SDL_CreateTextureFromSurface(renderer, surf_mid);
    SDL_Texture *tex_far = SDL_CreateTextureFromSurface(renderer, surf_far);
    SDL_FreeSurface(surf_near);
    SDL_FreeSurface(surf_mid);
    SDL_FreeSurface(surf_far);

    // Rain droplet data
    #define MAX_DROPS 300
    struct drop {
        float x, vy;
        int z_layer; // 0=near, 1=mid, 2=far
    };
    struct drop drops[MAX_DROPS];

    // Initialize drops
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = rand() % W;
        drops[i].vy = 200 + rand() % 200;
        drops[i].z_layer = rand() % 3;
    }

    // Scrolling background positions
    float bg_near_x = 0, bg_mid_x = 0, bg_far_x = 0;
    float wind_speed_near = 60, wind_speed_mid = 40, wind_speed_far = 20;

    // Lightning flash
    int lightning_flash = 0, lightning_timer = 0;

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

        // Update background scroll positions
        bg_near_x += wind_speed_near * speed_mult * 0.016f;
        bg_mid_x += wind_speed_mid * speed_mult * 0.016f;
        bg_far_x += wind_speed_far * speed_mult * 0.016f;

        // Wrap backgrounds
        if (bg_near_x >= W) bg_near_x -= W;
        if (bg_mid_x >= W) bg_mid_x -= W;
        if (bg_far_x >= W) bg_far_x -= W;

        // Update drops
        for (int i = 0; i < MAX_DROPS; i++) {
            drops[i].vy += 20 * speed_mult; // gravity
            drops[i].x += (drops[i].z_layer == 0 ? wind_speed_near * 0.5f :
                          drops[i].z_layer == 1 ? wind_speed_mid * 0.3f : wind_speed_far * 0.2f) * speed_mult * 0.016f;

            // Respawn at top if off screen
            if (drops[i].x > W + 10 || drops[i].x < -10) {
                drops[i].x = rand() % W;
                drops[i].vy = 200 + rand() % 200;
            }
            if (drops[i].vy > H * 2) {
                drops[i].x = rand() % W;
                drops[i].vy = 200 + rand() % 200;
            }
        }

        // Lightning occasionally
        if (rand() % 2000 == 0 && !lightning_flash) {
            lightning_flash = rand() % 100 + 50; // Flash frames
        }
        if (lightning_flash > 0) {
            lightning_flash--;
        }

        // Render
        SDL_SetRenderDrawColor(renderer,
            lightning_flash > 0 ? 200 : 50,
            lightning_flash > 0 ? 220 : 60,
            lightning_flash > 0 ? 255 : 80, 255);
        SDL_RenderClear(renderer);

        // Draw background layers (tiled scrolling)
        SDL_Rect bg_rect;
        bg_rect.w = tile_w;
        bg_rect.h = tile_h;
        bg_rect.y = H - tile_h;
        bg_rect.x = -W + (int)bg_far_x; SDL_RenderCopy(renderer, tex_far, NULL, &bg_rect);
        bg_rect.x = (int)bg_far_x; SDL_RenderCopy(renderer, tex_far, NULL, &bg_rect);
        bg_rect.x = W + (int)bg_far_x; SDL_RenderCopy(renderer, tex_far, NULL, &bg_rect);

        bg_rect.y = H - tile_h * 2;
        bg_rect.x = -W + (int)bg_mid_x; SDL_RenderCopy(renderer, tex_mid, NULL, &bg_rect);
        bg_rect.x = (int)bg_mid_x; SDL_RenderCopy(renderer, tex_mid, NULL, &bg_rect);
        bg_rect.x = W + (int)bg_mid_x; SDL_RenderCopy(renderer, tex_mid, NULL, &bg_rect);

        bg_rect.y = H - tile_h * 3;
        bg_rect.x = -W + (int)bg_near_x; SDL_RenderCopy(renderer, tex_near, NULL, &bg_rect);
        bg_rect.x = (int)bg_near_x; SDL_RenderCopy(renderer, tex_near, NULL, &bg_rect);
        bg_rect.x = W + (int)bg_near_x; SDL_RenderCopy(renderer, tex_near, NULL, &bg_rect);

        // Draw drops as simple lines
        SDL_SetRenderDrawColor(renderer, 180, 220, 255, 150);
        for (int i = 0; i < MAX_DROPS; i++) {
            if (drops[i].vy < H) {
                int alpha = 255 - (drops[i].vy / H) * 128; // Fade near bottom
                SDL_SetRenderDrawColor(renderer, 150, 200, 255, alpha < 0 ? 0 : alpha);
                SDL_RenderDrawLine(renderer, (int)drops[i].x, (int)(drops[i].vy) % H,
                                  (int)drops[i].x + (drops[i].z_layer == 0 ? 2 : 1),
                                  (int)(drops[i].vy + (drops[i].z_layer == 0 ? 10 : 5)) % H);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyTexture(tex_near);
    SDL_DestroyTexture(tex_mid);
    SDL_DestroyTexture(tex_far);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
