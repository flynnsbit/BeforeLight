#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <time.h>
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

    SDL_Window *window = SDL_CreateWindow("Spotlight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    // Load background texture for the spotlight effect
    SDL_Surface *surf = IMG_Load("spotlight_bg.png");
    if (!surf) {
        SDL_Log("Spotlight background optional, using black");
        // No background, just use black
    }

    SDL_Texture *bg_tex = NULL;
    if (surf) {
        bg_tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

    // Spotlight properties
    float spotlight_x = W / 2.0f, spotlight_y = H / 2.0f;
    float spotlight_vx = 0, spotlight_vy = 0; // movement velocity
    int path_timer = 0; // timer for path changes
    int spotlight_radius = 120; // beam radius

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

        // Update spotlight movement (random path)
        if (path_timer <= 0) {
            // Choose new direction and speed
            float angle = (rand() % 360) * (3.14159f / 180.0f);
            float speed = 20.0f + (rand() % 100) / 10.0f;
            spotlight_vx = cosf(angle) * speed;
            spotlight_vy = sinf(angle) * speed;
            path_timer = 60 + rand() % 120; // Change direction every 1-3 seconds
        }
        path_timer--;

        spotlight_x += spotlight_vx * speed_mult * 0.016f;
        spotlight_y += spotlight_vy * speed_mult * 0.016f;

        // Bounce off walls
        if (spotlight_x < spotlight_radius) { spotlight_x = spotlight_radius; spotlight_vx = -spotlight_vx; }
        if (spotlight_x > W - spotlight_radius) { spotlight_x = W - spotlight_radius; spotlight_vx = -spotlight_vx; }
        if (spotlight_y < spotlight_radius) { spotlight_y = spotlight_radius; spotlight_vy = -spotlight_vy; }
        if (spotlight_y > H - spotlight_radius) { spotlight_y = H - spotlight_radius; spotlight_vy = -spotlight_vy; }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Draw background texture if available
        if (bg_tex) {
            SDL_RenderCopy(renderer, bg_tex, NULL, NULL);
        }

        // Draw spotlight effect (simple white circle overlay)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        for (int r = 0; r < spotlight_radius; r += 2) {
            int diameter = r * 2;
            SDL_Rect beam = {
                (int)(spotlight_x - r), (int)(spotlight_y - r),
                diameter, diameter
            };
            Uint8 alpha = 100 - (r * 100 / spotlight_radius);
            if (alpha > 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                SDL_RenderDrawRect(renderer, &beam);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
