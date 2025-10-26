/**
 * Cityscape Screensaver
 * Replicates the yellow cityscape buildings with blinking windows from starry-n1ght.netlify.app
 *
 * Program: Portable C screensaver using SDL2
 * Compile: gcc -o cityscape main_cityscape.c `sdl2-config --cflags --libs` -lm
 * Run: ./cityscape
 */

#include <SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <unistd.h> // for getopt

extern char *optarg;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60
#define FRAME_DELAY_MS (1000 / TARGET_FPS)

#define NUM_BUILDINGS 15
#define MIN_BUILDING_WIDTH 30
#define MAX_BUILDING_WIDTH 90
#define MIN_BUILDING_HEIGHT 100
#define MAX_BUILDING_HEIGHT 250

// Colors: Mustard yellow buildings (#FFC107), White windows (#FFFFFF), Black background
#define BUILDING_R 255
#define BUILDING_G 193
#define BUILDING_B 7
#define WINDOW_R 255
#define WINDOW_G 255
#define WINDOW_B 255
#define WINDOW_SIZE 4

#define WINDOW_SPACING 10

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -f 0|1  Windowed (0) or fullscreen (1) mode (default: fullscreen)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

typedef struct {
    int x, y;          // Bottom-left corner position
    int width, height; // Building dimensions
    int window_rows;   // Number of window rows
    int window_cols;   // Number of window columns
    uint8_t *window_states;      // 1 = on, 0 = off
    float *window_timers;        // Seconds until next toggle check
} Building;

void initialize_buildings(Building *buildings);
void update_windows(Building *buildings, float dt);
void render(SDL_Renderer *renderer, Building *buildings);

/**
 * Main program entry point
 */
int main(int argc, char *argv[]) {
    int opt;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "f:h")) != -1) {
        switch (opt) {
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
    srand((unsigned int)time(NULL));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "Cityscape Screensaver",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (do_fullscreen) {
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
            fprintf(stderr, "Warning: Failed to set fullscreen: %s\n", SDL_GetError());
        }
    }

    // Initialize buildings
    Building buildings[NUM_BUILDINGS];
    initialize_buildings(buildings);

    // Main loop
    uint32_t last_time = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    while (running) {
        uint32_t current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        // Handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
            }
        }

        // Update window states
        update_windows(buildings, dt);

        // Clear screen to black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render buildings and windows
        render(renderer, buildings);

        // Present frame
        SDL_RenderPresent(renderer);

        // Maintain target FPS
        SDL_Delay(FRAME_DELAY_MS);
    }

    // Cleanup
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        free(buildings[i].window_states);
        free(buildings[i].window_timers);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

/**
 * Initialize building positions, sizes, and window grids
 */
void initialize_buildings(Building *buildings) {
    int current_x = 0; // Start at left edge

    for (int i = 0; i < NUM_BUILDINGS; i++) {
        Building *building = &buildings[i];

        // Random width and height within specified ranges
        building->width = MIN_BUILDING_WIDTH +
                         (rand() % (MAX_BUILDING_WIDTH - MIN_BUILDING_WIDTH + 1));
        building->height = MIN_BUILDING_HEIGHT +
                          (rand() % (MAX_BUILDING_HEIGHT - MIN_BUILDING_HEIGHT + 1));

        // Position building (side by side, no gaps, bottom-aligned)
        building->x = current_x;
        building->y = WINDOW_HEIGHT - building->height;

        // Calculate window grid (columns and rows based on spacing)
        building->window_cols = building->width / WINDOW_SPACING;
        building->window_rows = building->height / WINDOW_SPACING;

        // Ensure minimum of 1 row/col
        if (building->window_cols < 1) building->window_cols = 1;
        if (building->window_rows < 1) building->window_rows = 1;

        int total_windows = building->window_cols * building->window_rows;

        // Allocate window state and timer arrays
        building->window_states = malloc(total_windows * sizeof(uint8_t));
        building->window_timers = malloc(total_windows * sizeof(float));

        // Initialize window states (30% chance of starting on)
        for (int j = 0; j < total_windows; j++) {
            building->window_states[j] = (rand() % 100 < 30) ? 1 : 0;
            building->window_timers[j] = 0.5f + ((float)rand() / RAND_MAX) * 1.5f; // 0.5-2 sec initial
        }

        // Move to next building position (no gaps)
        current_x += building->width;
    }

    // Handle case where buildings exceed window width (wrap or scale, but keep simple)
    if (current_x > WINDOW_WIDTH) {
        // For simplicity, just place buildings within bounds
        current_x = 0;
        for (int i = 0; i < NUM_BUILDINGS; i++) {
            buildings[i].x = i * (WINDOW_WIDTH / NUM_BUILDINGS);
            buildings[i].width = WINDOW_WIDTH / NUM_BUILDINGS;
        }
    }
}

/**
 * Update window timers and toggle states as needed
 */
void update_windows(Building *buildings, float dt) {
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        Building *building = &buildings[i];
        int total_windows = building->window_cols * building->window_rows;

        // Count currently lit windows for population control
        int lit_count = 0;
        for (int j = 0; j < total_windows; j++) {
            if (building->window_states[j]) lit_count++;
        }

        for (int j = 0; j < total_windows; j++) {
            // Decrement timer
            building->window_timers[j] -= dt;

            // Check for state toggle when timer expires
            if (building->window_timers[j] <= 0.0f) {
                // 50% chance to toggle state, but only if it helps maintain 20-40% lit windows
                int target_min_lit = (int)(total_windows * 0.20f);
                int target_max_lit = (int)(total_windows * 0.40f);

                bool can_toggle = (rand() % 2 == 0); // Base 50% chance

                if (can_toggle) {
                    // Check if toggle would keep lit count in desired range
                    bool would_be_lit = !building->window_states[j];
                    int new_lit_count = would_be_lit ? lit_count + 1 : lit_count - 1;

                    if (new_lit_count >= target_min_lit && new_lit_count <= target_max_lit) {
                        // Perform the toggle
                        building->window_states[j] = 1 - building->window_states[j];
                        lit_count = new_lit_count;
                    }
                }

                // Reset timer (0.5-2 seconds)
                building->window_timers[j] = 0.5f + ((float)rand() / RAND_MAX) * 1.5f;
            }
        }
    }
}

/**
 * Render all buildings and their windows
 */
void render(SDL_Renderer *renderer, Building *buildings) {
    // Set building color (mustard yellow)
    SDL_SetRenderDrawColor(renderer, BUILDING_R, BUILDING_G, BUILDING_B, 255);

    for (int i = 0; i < NUM_BUILDINGS; i++) {
        Building *building = &buildings[i];

        // Create SDL_Rect for building
        SDL_Rect building_rect = {
            building->x,
            building->y,
            building->width,
            building->height
        };

        // Draw building as filled rectangle
        SDL_RenderFillRect(renderer, &building_rect);

        // Draw lit windows
        SDL_SetRenderDrawColor(renderer, WINDOW_R, WINDOW_G, WINDOW_B, 255);

        for (int row = 0; row < building->window_rows; row++) {
            for (int col = 0; col < building->window_cols; col++) {
                int window_index = row * building->window_cols + col;

                if (building->window_states[window_index]) {
                    // Calculate window position (centered in grid cells, offset from edges)
                    int window_x = building->x + col * WINDOW_SPACING + 3; // +3 for offset from edge
                    int window_y = building->y + row * WINDOW_SPACING + 3;

                    SDL_Rect window_rect = {
                        window_x,
                        window_y,
                        WINDOW_SIZE,
                        WINDOW_SIZE
                    };

                    SDL_RenderFillRect(renderer, &window_rect);
                }
            }
        }
    }
}
