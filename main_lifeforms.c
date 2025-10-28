#include <SDL.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

extern char *optarg;

#define PI 3.14159f

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

typedef struct {
    float x, y;
} Point;

typedef struct {
    int v1, v2; // vertex indices that connect
} Edge;

// Bear constellation
const Point bear_vertices[] = {
    {0, 0},      // 0: head bottom
    {-30, -40},  // 1: head top
    {30, -40},   // 2: head right
    {-60, 20},   // 3: left ear
    {60, 20},    // 4: right ear
    {-80, 60},   // 5: left paw front
    {80, 60},    // 6: right paw front
    {-70, 120},  // 7: left paw back
    {70, 120},   // 8: right paw back
    {-40, 80},   // 9: left front leg
    {40, 80},    // 10: right front leg
};

const Edge bear_edges[] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4},  // head connections
    {0, 5}, {0, 6}, {0, 7}, {0, 8},  // body connections
    {5, 9}, {6, 10}, {7, 9}, {8, 10}, // leg connections
    {3, 5}, {4, 6}, // ear to shoulder connections
};

// Fish constellation
const Point fish_vertices[] = {
    {0, 0},      // 0: head
    {40, -30},   // 1: top fin
    {40, 30},    // 2: bottom fin
    {80, 0},     // 3: tail base
    {120, -20},  // 4: tail top
    {120, 20},   // 5: tail bottom
    {50, -10},   // 6: dorsal fin top
    {50, 10},    // 7: ventral fin
    {-20, -15},  // 8: eye
};

const Edge fish_edges[] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 6}, {0, 7}, // head fins
    {3, 4}, {3, 5}, {1, 6}, {2, 7}, // body fins
    {0, 8}, // eye
};

// Bird constellation
const Point bird_vertices[] = {
    {0, 0},      // 0: head
    {20, -20},   // 1: beak
    {-30, -40},  // 2: left wing top
    {-10, 10},   // 3: left wing bottom
    {30, -40},   // 4: right wing top
    {10, 10},    // 5: right wing bottom
    {-20, 20},   // 6: left tail
    {20, 20},    // 7: right tail
    {0, 30},     // 8: tail center
};

const Edge bird_edges[] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, // head and wings
    {0, 6}, {0, 7}, {0, 8}, // tail
    {2, 3}, {4, 5}, {6, 7}, {6, 8}, {7, 8}, // wing and tail connections
};

// Human figure constellation
const Point human_vertices[] = {
    {0, -60},   // 0: head
    {0, 0},     // 1: torso
    {30, -30},  // 2: left arm
    {-30, -30}, // 3: right arm
    {20, 60},   // 4: left leg
    {-20, 60},  // 5: right leg
};

const Edge human_edges[] = {
    {0, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, // body connections
};

typedef struct {
    const Point *vertices;
    const Edge *edges;
    int num_vertices;
    int num_edges;
} Constellation;

const Constellation constellations[] = {
    {bear_vertices, bear_edges, sizeof(bear_vertices)/sizeof(Point), sizeof(bear_edges)/sizeof(Edge)},
    {fish_vertices, fish_edges, sizeof(fish_vertices)/sizeof(Point), sizeof(fish_edges)/sizeof(Edge)},
    {bird_vertices, bird_edges, sizeof(bird_vertices)/sizeof(Point), sizeof(bird_edges)/sizeof(Edge)},
    {human_vertices, human_edges, sizeof(human_vertices)/sizeof(Point), sizeof(human_edges)/sizeof(Edge)},
};

typedef struct {
    Point pos;         // current position
    Point target;      // target position
    float connect_progress; // 0-1 how connected line is
    int is_active;     // star is visible
} Star;

const int NUM_CONSTELLATIONS = sizeof(constellations) / sizeof(Constellation);

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

    SDL_Window *window = SDL_CreateWindow("Life Forms", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    // Initialize stars (max 32 vertices for any constellation)
    Star stars[32];
    for (int i = 0; i < 32; i++) {
        stars[i].pos.x = 0;
        stars[i].pos.y = 0;
        stars[i].target.x = 0;
        stars[i].target.y = 0;
        stars[i].connect_progress = 0;
        stars[i].is_active = 0;
    }

    // Animation phases
    enum Phase {PHASE_SCATTER, PHASE_CONNECT, PHASE_HOLD, PHASE_DISSOLVE};
    int current_phase = PHASE_SCATTER;
    float phase_timer = 0;
    int current_constellation = 0;
    int num_stars_in_pattern = 0;

    // Hide cursor during screensaver
    system("hyprctl keyword cursor:invisible true &>/dev/null");

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                // Only quit on mouse motion after 2 seconds to prevent immediate quit
                Uint32 current_time = SDL_GetTicks();
                if ((current_time - start_time) > 2000) { // 2 second grace period
                    quit = 1;
                }
            }
        }

        const Constellation *constellation = &constellations[current_constellation];
        const float phase_duration = 3.0f; // seconds per phase

        phase_timer += 0.016f * speed_mult;

        // Phase logic
        if (current_phase == PHASE_SCATTER) {
            // Initialize star positions
            if (num_stars_in_pattern == 0) {
                num_stars_in_pattern = constellation->num_vertices;
                for (int i = 0; i < num_stars_in_pattern; i++) {
                    // Scatter stars randomly
                    stars[i].pos.x = (rand() % W) - W/2.0f;
                    stars[i].pos.y = (rand() % H) - H/2.0f;
                    stars[i].target.x = constellation->vertices[i].x * 2; // scale up
                    stars[i].target.y = constellation->vertices[i].y * 2;
                    stars[i].connect_progress = 0;
                    stars[i].is_active = 1;
                }
            }

            // Move stars toward constellation positions
            float scatter_progress = phase_timer / phase_duration;
            if (scatter_progress > 1) scatter_progress = 1;

            for (int i = 0; i < num_stars_in_pattern; i++) {
                float t = scatter_progress * speed_mult;
                if (t > 1) t = 1;
                stars[i].pos.x += (stars[i].target.x - stars[i].pos.x) * t * 0.1f;
                stars[i].pos.y += (stars[i].target.y - stars[i].pos.y) * t * 0.1f;
            }

            if (phase_timer >= phase_duration) {
                // Stars are in position, start connecting
                current_phase = PHASE_CONNECT;
                phase_timer = 0;
            }

        } else if (current_phase == PHASE_CONNECT) {
            // Gradually connect stars with lines
            float connect_progress = phase_timer / phase_duration;
            for (int i = 0; i < constellation->num_edges; i++) {
                float edge_progress = connect_progress * constellation->num_edges - i;
                if (edge_progress < 0) edge_progress = 0;
                if (edge_progress > 1) edge_progress = 1;

                // Store connection progress on the first vertex of each edge
                stars[constellation->edges[i].v1].connect_progress = edge_progress;
            }

            if (phase_timer >= phase_duration) {
                current_phase = PHASE_HOLD;
                phase_timer = 0;
            }

        } else if (current_phase == PHASE_HOLD) {
            // Hold the formed constellation
            if (phase_timer >= phase_duration * 2) {
                current_phase = PHASE_DISSOLVE;
                phase_timer = 0;
            }

        } else if (current_phase == PHASE_DISSOLVE) {
            // Gradually disconnect stars
            float dissolve_progress = phase_timer / phase_duration;
            for (int i = 0; i < constellation->num_edges; i++) {
                float edge_progress = 1.0f - dissolve_progress * constellation->num_edges + i;
                if (edge_progress < 0) edge_progress = 0;
                if (edge_progress > 1) edge_progress = 1;

                stars[constellation->edges[i].v1].connect_progress = edge_progress;
            }

            if ( dissolve_progress >= constellation->num_edges * 0.1f) {
                // Scatter stars when nearly dissolved
                for (int i = 0; i < num_stars_in_pattern; i++) {
                    stars[i].pos.x += (rand() % 200 - 100) * dissolve_progress;
                    stars[i].pos.y += (rand() % 200 - 100) * dissolve_progress;
                }
            }

            if (phase_timer >= phase_duration * 1.5f) {
                // Move to next constellation
                num_stars_in_pattern = 0;
                current_constellation = (current_constellation + 1) % NUM_CONSTELLATIONS;
                current_phase = PHASE_SCATTER;
                phase_timer = 0;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw connections first
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // dim gray lines
        for (int i = 0; i < constellation->num_edges; i++) {
            const Edge *edge = &constellation->edges[i];
            if (edge->v1 >= num_stars_in_pattern || edge->v2 >= num_stars_in_pattern) continue;

            float line_progress = stars[edge->v1].connect_progress;
            if (line_progress > 0) {
                Star *s1 = &stars[edge->v1];
                Star *s2 = &stars[edge->v2];

                int x1 = W/2 + (int)s1->pos.x;
                int y1 = H/2 + (int)s1->pos.y;
                int x2 = W/2 + (int)(s1->pos.x + (s2->pos.x - s1->pos.x) * line_progress);
                int y2 = H/2 + (int)(s1->pos.y + (s2->pos.y - s1->pos.y) * line_progress);

                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
        }

        // Draw stars
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white stars
        for (int i = 0; i < num_stars_in_pattern; i++) {
            if (stars[i].is_active) {
                int x = W/2 + (int)stars[i].pos.x;
                int y = H/2 + (int)stars[i].pos.y;
                SDL_Rect star_rect = {x - 2, y - 2, 4, 4};
                SDL_RenderFillRect(renderer, &star_rect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup - restore cursor visibility
    system("hyprctl keyword cursor:invisible false 2>/dev/null");

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
