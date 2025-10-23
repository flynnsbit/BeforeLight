#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt

extern char *optarg;

#define PI 3.14159f
#define MAX_BUILDINGS 50
#define MAX_CARS 10
#define MAX_FLOATING_OBJS 20
#define MAX_PARTICLES 500
#define BUILDING_TYPES 5

typedef struct {
    int x, y;
    int width, height;
    int type;  // 0-4 for different building styles
    int window_rows, window_cols;
    int lights[32][16];  // Window light states (1=lit, 0=dark)
} Building;

typedef struct {
    float x, y;
    float vx;
    int type;  // 0=car, 1=vehicle
    SDL_Color color;
} Car;

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    int type;  // 0=rain, 1=snow, 2=smoke
    SDL_Color color;
} Particle;

typedef struct {
    float x, y;
    float vx, vy;
    int type;  // 0=balloon, 1=bird, 2=helicopter
    SDL_Color color;
} FloatingObject;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -w 0|1  Enable weather effects (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

// Generate procedural building
void generate_building(Building *b, int screen_height, int x_pos, int layer) {
    b->x = x_pos;
    b->type = rand() % BUILDING_TYPES;
    b->width = 40 + rand() % 60;  // Building width 40-100px

    // Height based on layer (background smaller, foreground larger)
    float height_scale = 0.3f + (layer * 0.3f);  // 0.3, 0.6, 0.9
    b->height = (int)(screen_height * height_scale * (0.4f + (rand() % 60) / 100.0f));

    // Position Y so buildings sit on ground (adjusted for layer)
    float ground_y = screen_height * 0.8f;  // Ground level at 80% screen height
    b->y = (int)(ground_y - b->height);

    // Window pattern
    b->window_rows = b->height / 25;  // Window height ~25px
    b->window_cols = b->width / 15;   // Window width ~15px

    // Initialize lights randomly
    for (int r = 0; r < b->window_rows && r < 32; r++) {
        for (int c = 0; c < b->window_cols && c < 16; c++) {
            b->lights[r][c] = (rand() % 100 < 65) ? 1 : 0;  // 65% chance of being lit
        }
    }
}

void init_cars(Car cars[], int count, int screen_width, int screen_height) {
    for (int i = 0; i < count; i++) {
        cars[i].y = screen_height * 0.82f + (rand() % 30 - 15);  // Street level
        cars[i].vx = 1.0f + (rand() % 20) / 10.0f;  // Speed 1.0-3.0
        cars[i].type = rand() % 2;  // Car or truck

        // Start some cars from left, some from right
        if (rand() % 2) {
            cars[i].x = -50 - (rand() % 200);  // Coming from left
        } else {
            cars[i].x = screen_width + 50 + (rand() % 200);
            cars[i].vx = -cars[i].vx;  // Going left
        }

        // Random colors
        SDL_Color colors[] = {
            {255, 0, 0, 255},    // Red
            {0, 0, 255, 255},    // Blue
            {255, 255, 0, 255},  // Yellow
            {255, 255, 255, 255}, // White
            {200, 200, 200, 255}, // Silver
            {100, 100, 100, 255}  // Dark
        };
        cars[i].color = colors[rand() % 6];
    }
}

void init_floating_objects(FloatingObject objs[], int count, int screen_width, int screen_height) {
    for (int i = 0; i < count; i++) {
        objs[i].type = rand() % 3;

        if (objs[i].type == 0) {  // Balloon
            objs[i].x = rand() % screen_width;
            objs[i].y = screen_height * 0.9f;  // Start near bottom
            objs[i].vx = (rand() % 20 - 10) / 10.0f;  // Drift slowly
            objs[i].vy = -0.5f - (rand() % 10) / 10.0f;  // Rise slowly
            objs[i].color = (SDL_Color){rand() % 255, rand() % 255, rand() % 255, 255};
        } else if (objs[i].type == 1) {  // Bird
            objs[i].x = rand() % screen_width;
            objs[i].y = screen_height * 0.4f + (rand() % 100);  // Mid-height
            objs[i].vx = 2.0f + (rand() % 40) / 10.0f;  // Faster
            objs[i].vy = (rand() % 20 - 10) / 10.0f;  // Gentle up/down
            if (rand() % 2) objs[i].vx = -objs[i].vx;  // Some go left
            objs[i].color = (SDL_Color){100, 100, 100, 255};  // Gray
        } else {  // Helicopter
            objs[i].x = rand() % screen_width;
            objs[i].y = screen_height * 0.3f + (rand() % 80);
            objs[i].vx = 1.5f + (rand() % 20) / 10.0f;
            objs[i].vy = 0;  // Fly straight
            if (rand() % 2) objs[i].vx = -objs[i].vx;
            objs[i].color = (SDL_Color){50, 50, 50, 255};  // Dark
        }
    }
}

int main(int argc, char *argv[]) {
    int opt;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;
    int weather_enabled = 1;

    while ((opt = getopt(argc, argv, "s:f:w:h")) != -1) {
        switch (opt) {
            case 's':
                speed_mult = atof(optarg);
                if (speed_mult <= 0.1f) speed_mult = 0.1f;
                if (speed_mult > 5.0f) speed_mult = 5.0f;
                break;
            case 'f':
                do_fullscreen = atoi(optarg);
                break;
            case 'w':
                weather_enabled = atoi(optarg);
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

    SDL_Window *window = SDL_CreateWindow("Cityscape", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    // Initialize buildings for 3 layers
    Building buildings[3][MAX_BUILDINGS];
    int building_counts[3] = {15, 12, 8};  // Background, midground, foreground

    for (int layer = 0; layer < 3; layer++) {
        int building_spacing = (W + 200) / building_counts[layer];  // Spread across width

        for (int i = 0; i < building_counts[layer]; i++) {
            int x_pos = (i * building_spacing) - 100 + (rand() % (building_spacing / 2));  // Some variation
            generate_building(&buildings[layer][i], H, x_pos, layer);
        }
    }

    // Initialize cars and floating objects
    Car cars[MAX_CARS];
    FloatingObject floating_objs[MAX_FLOATING_OBJS];
    Particle particles[MAX_PARTICLES];
    int particle_count = 0;

    init_cars(cars, MAX_CARS, W, H);
    init_floating_objects(floating_objs, MAX_FLOATING_OBJS, W, H);

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 last_time = SDL_GetTicks();

    float scroll_offsets[3] = {0, 0, 0};  // Background, mid, foreground scroll
    float base_scroll_speeds[3] = {0.2f, 0.5f, 1.0f};  // Different layer speeds

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        // Update scrolling
        for (int layer = 0; layer < 3; layer++) {
            scroll_offsets[layer] -= base_scroll_speeds[layer] * speed_mult;

            // Wrap scroll (reset when buildings are off screen)
            if (scroll_offsets[layer] < -200) {
                scroll_offsets[layer] = 0;
            }
        }

        // Update cars
        for (int i = 0; i < MAX_CARS; i++) {
            cars[i].x += cars[i].vx * speed_mult;
            if (cars[i].x > W + 100) cars[i].x = -100 - (rand() % 100);
            if (cars[i].x < -100) cars[i].x = W + 100 + (rand() % 100);
        }

        // Update floating objects
        for (int i = 0; i < MAX_FLOATING_OBJS; i++) {
            floating_objs[i].x += floating_objs[i].vx * speed_mult;
            floating_objs[i].y += floating_objs[i].vy * speed_mult;

            // Wrap around screen edges
            if (floating_objs[i].x > W + 50) floating_objs[i].x = -50;
            if (floating_objs[i].x < -50) floating_objs[i].x = W + 50;
            if (floating_objs[i].y < -50) {
                // Reset object to bottom
                floating_objs[i].y = H + 50;
                floating_objs[i].x = rand() % W;
                floating_objs[i].vx = (rand() % 20 - 10) / 10.0f;
            }
        }

        // Update building lights randomly
        if (rand() % 200 < 2) {  // Occasionally toggle lights
            for (int layer = 0; layer < 3; layer++) {
                for (int i = 0; i < building_counts[layer]; i++) {
                    Building *b = &buildings[layer][i];
                    int r = rand() % b->window_rows;
                    int c = rand() % b->window_cols;
                    b->lights[r][c] = 1 - b->lights[r][c];  // Toggle
                }
            }
        }

        // Add weather particles if enabled
        if (weather_enabled && particle_count < MAX_PARTICLES - 50) {
            for (int i = 0; i < 3; i++) {  // Add a few particles each frame
                if (particle_count >= MAX_PARTICLES) break;

                int type = rand() % 2;  // Rain or snow
                particles[particle_count].x = rand() % W;
                particles[particle_count].y = -10;
                particles[particle_count].vx = type ? 0 : 0.1f;  // Snow drifts, rain falls straight
                particles[particle_count].vy = 2.0f + (rand() % 20) / 10.0f;
                particles[particle_count].life = 1.0f;
                particles[particle_count].type = type;

                if (type == 0) {  // Rain
                    particles[particle_count].color = (SDL_Color){100, 150, 255, 200};
                } else {  // Snow
                    particles[particle_count].color = (SDL_Color){255, 255, 255, 200};
                    particles[particle_count].vy *= 0.5f;  // Slower falling
                }

                particle_count++;
            }
        }

        // Update weather particles
        for (int i = 0; i < particle_count; i++) {
            if (particles[i].life <= 0) continue;

            particles[i].x += particles[i].vx * speed_mult;
            particles[i].y += particles[i].vy * speed_mult;

            if (particles[i].y > H + 20) {
                particles[i].life = 0;  // Off screen
            }
        }

        // Remove dead particles
        int write_idx = 0;
        for (int read_idx = 0; read_idx < particle_count; read_idx++) {
            if (particles[read_idx].life > 0) {
                particles[write_idx++] = particles[read_idx];
            }
        }
        particle_count = write_idx;

        // Rendering

        // Sky gradient (day to night transition)
        float time = (SDL_GetTicks() / 10000.0f) * speed_mult;  // Slow cycle
        float sky_r = 135 + 120 * sinf(time);
        float sky_g = 206 + 50 * sinf(time + 2);
        float sky_b = 235 + 20 * sinf(time + 4);

        sky_r = fmaxf(0, fminf(255, sky_r));
        sky_g = fmaxf(0, fminf(255, sky_g));
        sky_b = fmaxf(0, fminf(255, sky_b));

        SDL_SetRenderDrawColor(renderer, (Uint8)sky_r, (Uint8)sky_g, (Uint8)sky_b, 255);
        SDL_RenderClear(renderer);

        // Render layers from back to front
        for (int layer = 0; layer < 3; layer++) {
            float scroll_pos = scroll_offsets[layer];

            for (int i = 0; i < building_counts[layer]; i++) {
                Building *b = &buildings[layer][i];

                // Building position with scroll and layer scaling
                float scale = 0.3f + (layer * 0.3f);  // 0.3, 0.6, 0.9
                int render_x = b->x + (int)scroll_pos;
                int render_y = b->y;
                int render_width = (int)(b->width * scale);
                int render_height = (int)(b->height * scale);

                // Wrap buildings around screen
                while (render_x + render_width < 0) render_x += W + 200;
                while (render_x > W) render_x -= W + 200;

                // Skip if off screen
                if (render_x + render_width < 0 || render_x > W) continue;

                // Building color based on type
                SDL_Color building_colors[BUILDING_TYPES] = {
                    {80, 80, 90, 255},   // Dark gray
                    {90, 85, 75, 255},   // Brown
                    {70, 70, 85, 255},   // Blue-gray
                    {85, 75, 70, 255},   // Tan
                    {75, 85, 75, 255}    // Green-gray
                };

                SDL_SetRenderDrawColor(renderer, building_colors[b->type].r, building_colors[b->type].g, building_colors[b->type].b, 255);
                SDL_Rect building_rect = {render_x, render_y, render_width, render_height};
                SDL_RenderFillRect(renderer, &building_rect);

                // Render windows if in foreground (better visibility)
                if (layer == 2 && scale > 0.8f) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);  // Window light color

                    int window_width = render_width / b->window_cols;
                    int window_height = render_height / b->window_rows;
                    int window_margin = 2;

                    for (int r = 0; r < b->window_rows && r < 32; r++) {
                        for (int c = 0; c < b->window_cols && c < 16; c++) {
                            if (b->lights[r][c]) {
                                SDL_Rect window_rect = {
                                    render_x + (c * window_width) + window_margin,
                                    render_y + (r * window_height) + window_margin,
                                    window_width - 2*window_margin,
                                    window_height - 2*window_margin
                                };
                                SDL_RenderFillRect(renderer, &window_rect);
                            }
                        }
                    }
                }
            }
        }

        // Render cars (on street level)
        for (int i = 0; i < MAX_CARS; i++) {
            int car_width = cars[i].type ? 25 : 20;  // Trucks larger
            int car_height = cars[i].type ? 10 : 8;

            SDL_SetRenderDrawColor(renderer, cars[i].color.r, cars[i].color.g, cars[i].color.b, 255);
            SDL_Rect car_rect = {(int)cars[i].x, (int)cars[i].y, car_width, car_height};
            SDL_RenderFillRect(renderer, &car_rect);

            // Headlights (small bright rectangles)
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect headlight = {(int)cars[i].x + car_width - 3, (int)cars[i].y + 1, 2, car_height-2};
            SDL_RenderFillRect(renderer, &headlight);
        }

        // Render floating objects
        for (int i = 0; i < MAX_FLOATING_OBJS; i++) {
            FloatingObject *obj = &floating_objs[i];
            int size = (obj->type == 0) ? 8 : (obj->type == 1) ? 6 : 12;  // Balloon, bird, helicopter

            SDL_SetRenderDrawColor(renderer, obj->color.r, obj->color.g, obj->color.b, 255);
            SDL_Rect obj_rect = {(int)obj->x - size/2, (int)obj->y - size/2, size, size};

            if (obj->type == 0) {  // Balloon - circle
                int radius = size / 2;
                for (int y = -radius; y <= radius; y++) {
                    for (int x = -radius; x <= radius; x++) {
                        if (x*x + y*y <= radius*radius) {
                            SDL_RenderDrawPoint(renderer, obj->x + x, obj->y + y);
                        }
                    }
                }
                // Balloon string
                SDL_RenderDrawLine(renderer, obj->x, obj->y + radius, obj->x, obj->y + radius + 10);
            } else if (obj->type == 1) {  // Bird - simple shape
                SDL_RenderDrawLine(renderer, obj->x - size/2, obj->y, obj->x + size/2, obj->y);
                SDL_RenderDrawLine(renderer, obj->x + size/2, obj->y, obj->x, obj->y - size/2);
                SDL_RenderDrawLine(renderer, obj->x, obj->y - size/2, obj->x - size/2, obj->y);
            } else {  // Helicopter - simple rectangle
                SDL_RenderFillRect(renderer, &obj_rect);
            }
        }

        // Render weather particles
        if (weather_enabled) {
            for (int i = 0; i < particle_count; i++) {
                Particle *p = &particles[i];
                int alpha = (int)(p->life * p->color.a);
                SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, alpha);

                int size = p->type ? 3 : 1;  // Snow larger than rain
                if (p->type == 0) { // Rain - line
                    SDL_RenderDrawLine(renderer, p->x, p->y, p->x + p->vx * 3, p->y + p->vy * 3);
                } else { // Snow - circle
                    int radius = size;
                    for (int y = -radius; y <= radius; y++) {
                        for (int x = -radius; x <= radius; x++) {
                            if (x*x + y*y <= radius*radius) {
                                SDL_RenderDrawPoint(renderer, p->x + x, p->y + y);
                            }
                        }
                    }
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
