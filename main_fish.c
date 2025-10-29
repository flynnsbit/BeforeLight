#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "assets/fish_angel.h"
#include "assets/fish_butterfly.h"
#include "assets/fish_flounder.h"
#include "assets/fish_guppy.h"
#include "assets/fish_jelly.h"
#include "assets/fish_minnow.h"
#include "assets/fish_red.h"
#include "assets/fish_seahorse.h"
#include "assets/fish_sprite.h"
#include "assets/fish_striped.h"
#include "assets/seafloor.h"
#include "assets/bubbles_50.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

#define WINDOW_WIDTH 0  // fullscreen
#define WINDOW_HEIGHT 0
#define SPRITE_SIZE 145
#define FISH_FRAME_COUNT 4  // not used

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t N    Number of fish (default: all)\n");
    fprintf(stderr, "  -m N    Number of bubbles (default: all)\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

struct AnimParam {
    float fly_duration;
    float delay;
    int flap_direction; // 1 or -1 or 0 for toast
};

const struct AnimParam anim_params[11] = {
    {18.2, 0.0, 0}, // ltr slowed
    {18.2, 0.0, 1}, // rtl slowed
    {9.1f, 0.0, 0}, // ltr-fast slowed
    {9.1f, 0.0, 1}, // rtl-fast slowed
    {18.2, 4.25f, 1}, // rtl-delay1 slowed
    {18.2, 8.5f, 1}, // rtl-delay-2 slowed
    {18.2, 4.25f, 1}, // rtl-delay1 or similar slowed
    {18.2, 6.0f, 1}, // rtl-delay2 slowed
    {18.2, 0.0, 0}, // bubble rise slowed
    {18.2, 4.0, 0}, // bubble rise delay slowed
    {18.2, 8.0, 0}, // bubble rise delay2 slowed
};

struct Pos {
    float top_pct;
};

const struct Pos poses[9] = {
    {-15}, // 0 row1
    {5},   // 1 row2
    {25},  // 2 row3
    {45},  // 3 row4
    {65},  // 4 row5
    {85},  // 5 row6
    {10},  // 6 bubble left
    {50},  // 7 bubble middle
    {85}   // 8 bubble right
};

typedef struct Entity {
    int is_toaster;
    int anim_type;
    int pos_index;
    int toast_type;
} Entity;

const Entity entities[] = {
    {0, 0, 0, 1}, // butterfly ltr row1 fish-butterfly.png
    {0, 3, 0, 4}, // jelly rtl-fast row1 fish-jelly.png
    {0, 1, 1, 3}, // guppy rtl row2 fish-guppy.png
    {0, 4, 1, 0}, // angel rtl-delay1 row2 fish-angel.png
    {0, 5, 2, 7}, // seahorse rtl-delay-2 row3 fish-seahorse.png
    {0, 7, 3, 6}, // red rtl-delay2 row4 fish-red.png
    {0, 0, 3, 4}, // jelly ltr row4 fish-jelly.png
    {0, 1, 4, 5}, // minnow rtl row5 fish-minnow.png
    {0, 2, 4, 7}, // seahorse ltr-fast row5 fish-seahorse.png
    {0, 3, 5, 0}, // angel rtl-fast row6 fish-angel.png
    {1, 8, 6, 4}, // bubble left
    {1, 9, 7, 4}, // bubble middle
    {1, 10, 8, 4}, // bubble right
};

int main(int argc, char *argv[]) {
    int opt;
    int fish_count = 30;
    int bubble_count = 15;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "t:m:s:f:h")) != -1) {
        switch (opt) {
            case 't':
                fish_count = atoi(optarg);
                break;
            case 'm':
                bubble_count = atoi(optarg);
                break;
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

    setenv("SDL_VIDEODRIVER", "wayland", 1); // Force Wayland for Hyprland
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

    SDL_Window *window = SDL_CreateWindow("Fish Aquarium", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    // Load textures
    SDL_Texture *fish_texs[10], *bg_tex, *bubble_tex;
    SDL_Surface *surf;
    int bg_w = 0, bg_h = 0;
    for (int i = 0; i < 10; i++) {
        const unsigned char *data = NULL;
        unsigned int len = 0;
        switch (i) {
            case 0: data = fish_angel; len = fish_angel_len; break;
            case 1: data = fish_butterfly; len = fish_butterfly_len; break;
            case 2: data = fish_flounder; len = fish_flounder_len; break;
            case 3: data = fish_guppy; len = fish_guppy_len; break;
            case 4: data = fish_jelly; len = fish_jelly_len; break;
            case 5: data = fish_minnow; len = fish_minnow_len; break;
            case 6: data = fish_red; len = fish_red_len; break;
            case 7: data = fish_seahorse; len = fish_seahorse_len; break;
            case 8: data = fish_sprite; len = fish_sprite_len; break;
            case 9: data = fish_striped; len = fish_striped_len; break;
        }
        SDL_RWops *rw = SDL_RWFromConstMem(data, len);
        if (!rw) {
            SDL_Log("Error creating RWops for embedded fish texture %d: %s", i, SDL_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        surf = IMG_Load_RW(rw, 1); // 1 to autoclose
        if (!surf) {
            SDL_Log("Error loading embedded fish texture %d: %s", i, IMG_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        fish_texs[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

    // Background seabed
    SDL_RWops *rw_bg = SDL_RWFromConstMem(seafloor, seafloor_len);
    if (!rw_bg) {
        SDL_Log("Error creating RWops for embedded seafloor: %s", SDL_GetError());
        // Skip background
        bg_tex = NULL;
    } else {
        surf = IMG_Load_RW(rw_bg, 1); // 1 to autoclose
        if (!surf) {
            SDL_Log("Error loading embedded seafloor: %s", IMG_GetError());
            // Skip background
            bg_tex = NULL;
        } else {
            bg_tex = SDL_CreateTextureFromSurface(renderer, surf);
            bg_w = surf->w;
            bg_h = surf->h;
            SDL_FreeSurface(surf);
        }
    }

    // Bubbles
    SDL_RWops *rw_bubble = SDL_RWFromConstMem(bubbles_50, bubbles_50_len);
    if (!rw_bubble) {
        SDL_Log("Error creating RWops for embedded bubbles: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    surf = IMG_Load_RW(rw_bubble, 1); // 1 to autoclose
    if (!surf) {
        SDL_Log("Error loading embedded bubbles: %s", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    bubble_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    // Hide cursor during screensaver
    system("hyprctl keyword cursor:invisible true &>/dev/null");

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Log("Screensaver quit triggered: event type %d", e.type);
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                // Only quit on mouse motion after 2 seconds to prevent immediate quit
                Uint32 current_time = SDL_GetTicks();
                if ((current_time - start_time) > 2000) { // 2 second grace period
                    SDL_Log("Screensaver quit triggered: mouse motion after grace period");
                    quit = 1;
                }
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float time_s = (current_time - start_time) / 1000.0f;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Background seabed
        if (bg_tex) {
            for (int x = 0; x < W; x += bg_w) {
                SDL_Rect bgrect = {x, H - bg_h, bg_w, bg_h};
                SDL_RenderCopy(renderer, bg_tex, NULL, &bgrect);
            }
        } else {
            SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // brown color
            SDL_RenderFillRect(renderer, &(SDL_Rect){0, H - 100, W, 100}); // sea floor bottom
        }

        size_t entity_count = sizeof(entities) / sizeof(entities[0]);

        // Render bubbles (is_toaster==1)
        int drawn_bubbles = 0;
        for (size_t i = 0; i < entity_count; i++) {
            if (drawn_bubbles >= bubble_count) break;
            const Entity ent = entities[i];
            if (ent.is_toaster != 1) continue;
            const struct AnimParam ap = anim_params[ent.anim_type];
            const struct Pos pos = poses[ent.pos_index];

            float local_time = time_s - ap.delay;
            if (local_time < 0) continue;

            float current_x = pos.top_pct * W / 100.0f - 25.0f; // center bubble
            float current_y = (H + 56.0f) - local_time * ((H + 56.0f) / ap.fly_duration);
            if (current_y < -56) continue;

            SDL_Rect dstrect = {(int)current_x, (int)current_y, 50, 56};

            // Bubble animation (2 frames)
            float bubble_cycle = fmodf(local_time, 0.4f);
            int bubble_frame = (int)(bubble_cycle / 0.2f) % 2;
            SDL_Rect bubble_srcrect = {bubble_frame * 50, 0, 50, 56};

            SDL_RenderCopy(renderer, bubble_tex, &bubble_srcrect, &dstrect);
            drawn_bubbles++;
        }

        // Render fish (is_toaster==0)
        int drawn_fish = 0;
        for (size_t i = 0; i < entity_count; i++) {
            const Entity ent = entities[i];
            if (ent.is_toaster != 0) continue;
            if (drawn_fish >= fish_count) continue;
            const struct AnimParam ap = anim_params[ent.anim_type];
            const struct Pos pos = poses[ent.pos_index];

            // Animation timing
            float local_time = time_s - ap.delay;
            if (local_time < 0) continue;

            // Vertical toggle for row1
            float current_top_pct = pos.top_pct;
            if (ent.pos_index == 0) {
                float toggle_cycle = fmodf(local_time, 13.0f);
                if (toggle_cycle >= 6.5f) {
                    current_top_pct = pos.top_pct + 50.0f;
                }
            }

            float cycle_time = fmodf(local_time, ap.fly_duration);
            float fly_f = cycle_time / ap.fly_duration;

            // Calculate fish size (fixed 50% smaller)
            int fish_size = SPRITE_SIZE / 2;

            // Calculate start position
            float start_y = (current_top_pct / 100.0f * H) - fish_size / 2.0f;

            // Fish swim animation
            int direction = ap.flap_direction;
            float start_left_pct = (direction == 0) ? -1.0f : 1.4f;
            float end_left_pct = (direction == 0) ? 1.4f : -1.0f;
            float delta_left_pct = end_left_pct - start_left_pct;
            float current_left_pct = start_left_pct + delta_left_pct * fly_f;

            float current_x = current_left_pct * W - fish_size / 2.0f;
            float current_y = start_y;
            SDL_Rect dstrect = {(int)current_x, (int)current_y, fish_size, fish_size};

            // Fish sprite animation (2 frames)
            float flap_cycle = fmodf(local_time, 0.6f); // 2 frames, 0.3s each
            int flap_frame = (int)(flap_cycle / 0.3f) % 2;
            SDL_Rect srcrect = {flap_frame * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE};

            SDL_RendererFlip flip = (direction == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer, fish_texs[ent.toast_type], &srcrect, &dstrect, 0.0, NULL, flip);
            drawn_fish++;
        }



        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup - restore cursor visibility
    system("hyprctl keyword cursor:invisible false 2>/dev/null");

    // Cleanup
    for (int i = 0; i < 10; i++) {
        SDL_DestroyTexture(fish_texs[i]);
    }
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyTexture(bubble_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
