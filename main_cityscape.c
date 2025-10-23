#include <SDL.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt

extern char *optarg;

#define PI 3.14159f
#define MAX_STARS 200
#define MAX_BUILDINGS 12  // Solid buildings with clear silhouettes
#define MAX_METEORS 5
#define SKY_HEIGHT_PERCENT 0.8f

typedef struct {
    float x, y;
    float brightness;
    float twinkle_speed;
    float base_brightness;
} Star;

typedef struct {
    float x, height, width;
    int num_windows;
} Building;

typedef struct {
    float x, y;
    float vx, vy;
    float life;
} Meteor;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

// Generate night sky color gradient
SDL_Color get_sky_color(int y, int total_height) {
    float normalized_y = (float)y / total_height;
    SDL_Color color;

    // Deep night sky gradient from horizon to zenith
    if (normalized_y < 0.3f) {
        // Horizon area - slightly lighter blue
        color.r = (Uint8)(30 + normalized_y * 50);
        color.g = (Uint8)(50 + normalized_y * 70);
        color.b = (Uint8)(80 + normalized_y * 100);
    } else {
        // Upper sky - deep navy to black
        float upper_factor = (normalized_y - 0.3f) / 0.7f;
        color.r = (Uint8)(15 * (1.0f - upper_factor));
        color.g = (Uint8)(30 * (1.0f - upper_factor));
        color.b = (Uint8)(80 * (1.0f - upper_factor));
    }

    return color;
}

void generate_stars(Star stars[], int count, int screen_width, int sky_height) {
    for (int i = 0; i < count; i++) {
        stars[i].x = rand() % screen_width;
        stars[i].y = rand() % sky_height;
        stars[i].base_brightness = 150 + rand() % 105;  // 150-255
        stars[i].brightness = stars[i].base_brightness;
        stars[i].twinkle_speed = 0.01f + (rand() % 100) / 1000.0f;  // Slow twinkle
    }
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
                if (speed_mult > 3.0f) speed_mult = 3.0f;
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

    SDL_Window *window = SDL_CreateWindow("Starry Night Cityscape", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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
    int sky_height = (int)(H * SKY_HEIGHT_PERCENT);

    // Initialize stars
    Star stars[MAX_STARS];
    generate_stars(stars, MAX_STARS, W, sky_height);

    // Initialize buildings
    Building buildings[MAX_BUILDINGS];
    int building_width = W / MAX_BUILDINGS * 0.8f;
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        buildings[i].x = i * (W / MAX_BUILDINGS) + (rand() % (W / MAX_BUILDINGS / 4));
        buildings[i].width = building_width + (rand() % building_width / 2); // Variable width
        buildings[i].height = H * 0.4f + (rand() % (int)(H * 0.3f)); // Random height
        buildings[i].num_windows = 3 + rand() % 4; // 3-6 windows
    }

    // Initialize meteors
    Meteor meteors[MAX_METEORS];
    for (int i = 0; i < MAX_METEORS; i++) {
        meteors[i].life = 0;  // Start inactive
    }

    // Main animation loop
    SDL_Event e;
    int quit = 0;
    Uint32 last_time = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        // Render background sky gradient (single scanlines for performance)
        for (int y = 0; y < sky_height; y += 2) {  // Render every other line
            SDL_Color sky_color = get_sky_color(y, H);
            SDL_SetRenderDrawColor(renderer, sky_color.r, sky_color.g, sky_color.b, 255);

            SDL_Rect scanline = {0, y, W, 2};
            SDL_RenderFillRect(renderer, &scanline);
        }

        // Render stars with twinkling
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < MAX_STARS; i++) {
            // Update twinkle
            stars[i].brightness = stars[i].base_brightness +
                                 sinf(current_time * 0.001f * stars[i].twinkle_speed) * 50;

            if (stars[i].brightness > 255) stars[i].brightness = 255;
            if (stars[i].brightness < 100) stars[i].brightness = 100;

            int size = (stars[i].brightness > 200) ? 2 : 1;
            int alpha = (int)stars[i].brightness;

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
            SDL_Rect star_rect = {(int)stars[i].x - size/2, (int)stars[i].y - size/2, size, size};
            SDL_RenderFillRect(renderer, &star_rect);

            // Brighter stars get a glow effect
            if (stars[i].brightness > 220) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(renderer, 255, 255, 200, alpha * 0.3f);
                SDL_Rect glow_rect = {(int)stars[i].x - 3, (int)stars[i].y - 3, 6, 6};
                SDL_RenderFillRect(renderer, &glow_rect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }

        // Occasionally spawn a meteor
        if (rand() % 800 < 2) {  // Low probability
            for (int i = 0; i < MAX_METEORS; i++) {
                if (meteors[i].life <= 0) {
                    meteors[i].x = rand() % W;
                    meteors[i].y = -10;
                    meteors[i].vx = -(3.0f + rand() % 10) * speed_mult;
                    meteors[i].vy = (1.0f + rand() % 5) * speed_mult;
                    meteors[i].life = 1.0f;
                    break;
                }
            }
        }

        // Update and render meteors
        for (int i = 0; i < MAX_METEORS; i++) {
            if (meteors[i].life > 0) {
                meteors[i].x += meteors[i].vx;
                meteors[i].y += meteors[i].vy;
                meteors[i].life -= dt * 1.5f;

                if (meteors[i].life > 0 && meteors[i].y < sky_height) {
                    // Draw meteor trail
                    int trail_length = 20;
                    for (int t = 0; t < trail_length; t++) {
                        int trail_x = (int)(meteors[i].x + meteors[i].vx * t * 0.5f);
                        int trail_y = (int)(meteors[i].y + meteors[i].vy * t * 0.5f);
                        float trail_alpha = meteors[i].life * (trail_length - t) / trail_length;

                        if (trail_x >= 0 && trail_x < W && trail_y >= 0 && trail_y < sky_height) {
                            SDL_SetRenderDrawColor(renderer, 255, 255, 150, (Uint8)(trail_alpha * 255));
                            SDL_RenderDrawPoint(renderer, trail_x, trail_y);
                        }
                    }
                }
            }
        }

        // Render buildings (solid silhouettes with windows lit)
        for (int i = 0; i < MAX_BUILDINGS; i++) {
            Building *b = &buildings[i];

            // Building silhouette
            SDL_SetRenderDrawColor(renderer, 35, 35, 40, 255);  // Dark building color
            int building_x = (int)b->x;
            int building_y = H - (int)b->height - 50;  // Ground level
            int building_w = (int)b->width;
            int building_h = (int)b->height;

            SDL_Rect building_rect = {building_x, building_y, building_w, building_h};
            SDL_RenderFillRect(renderer, &building_rect);

            // Window lights
            SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);  // Warm yellow windows

            // Vertical windows stacked
            int window_spacing = building_h / (b->num_windows + 1);
            int window_width = building_w * 0.15f + rand() % building_w / 10;  // Variable window width
            int window_margin = 8;

            for (int w = 0; w < b->num_windows; w++) {
                int window_y = building_y + window_spacing * (w + 1) - window_width/2;
                int window_x = building_x + window_margin;

                // Only render windows if they're within building bounds
                if (window_x + window_width > building_x + building_w - window_margin) {
                    window_width = building_x + building_w - window_x - window_margin;
                }

                if (window_x + window_width <= building_x + building_w &&
                    window_y + window_width <= building_y + building_h) {

                    SDL_Rect window_rect = {window_x, window_y, window_width, window_width};
                    SDL_RenderFillRect(renderer, &window_rect);

                    // Light rays extending upward from windows
                    int ray_height = sky_height - window_y;
                    if (ray_height > 0) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
                        SDL_SetRenderDrawColor(renderer, 255, 255, 200, 40);  // Subtle rays
                        SDL_Rect light_ray = {window_x + window_width/2 - 10, window_y - ray_height, 20, ray_height};
                        SDL_RenderFillRect(renderer, &light_ray);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    }
                }
            }
        }

        // Add subtle city glow near horizon
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        for (int y = sky_height; y < H; y++) {
            float glow_factor = 1.0f - ((float)y - sky_height) / (H - sky_height);
            if (glow_factor > 0) {
                Uint8 glow_alpha = (Uint8)(glow_factor * 80 * speed_mult);
                SDL_SetRenderDrawColor(renderer, 100, 120, 180, glow_alpha);
                SDL_Rect glow_line = {0, y, W, 1};
                SDL_RenderFillRect(renderer, &glow_line);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
