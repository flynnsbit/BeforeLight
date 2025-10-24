/**
 * Starry Night Screensaver
 * Hyprland-compatible night sky screensaver with drifting stars and meteors
 * Inspired by After Dark Starry Night (https://starry-n1ght.netlify.app/)
 *
 * Controls:
 * - ESC or mouse/keyboard input to exit
 * - -s F: speed multiplier (default 1.0)
 * - -d N: star density (0=sparse, 1=dense, default 0.5)
 * - -m F: meteor frequency multiplier (default 1.0, higher = more meteors)
 *
 * Requires: SDL2, mesa/opengl (wayland)
 * Build: gcc -o starrynight starrynight.c -lSDL2 -lGL -lm
 * Run: SDL_VIDEODRIVER=wayland ./starrynight
 */

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define PI 3.14159265359f
#define STAR_COUNT 500  // Space for drifting sky stars only
#define GAP_STAR_COUNT 10000  // Stars specifically in gaps between buildings
#define METEOR_COUNT 10
#define METEOR_PARTICLES 20
#define CITY_BUILDINGS 13     // Number of solid buildings with windows

typedef struct {
    float x, y;           // Position in pixels
    float vx, vy;         // Velocity for drift effect
    float brightness;     // Current brightness 0-1 (for twinkling)
    float base_brightness; // Base brightness 0-1
    float twinkle_phase;  // Sin phase offset for unique twinkling
    float twinkle_speed;  // How fast it twinkles
    float size;           // Star size in pixels
    bool is_bright;       // Extra-bright star status for glow effect
    int building_gap;     // Which building gap this star belongs to (-1 for sky)
} Star;

// Static building properties - calculated once during initialization
typedef struct {
    float x, y;        // Bottom-left position
    float width, height; // Building dimensions
    float right_edge;  // Right edge x coordinate
} Building;

Building buildings[CITY_BUILDINGS]; // Static building array

// Gap stars - fill spaces between buildings
Star *gap_stars; // Stars that specifically fill gaps between buildings

typedef struct {
    float x, y;           // Current position
    float vx, vy;         // Velocity
    float life;           // 0-1 lifetime remaining
    float tail_particlesx[METEOR_PARTICLES]; // Positions for tail effect
    float tail_particlesy[METEOR_PARTICLES];
    float tail_alphas[METEOR_PARTICLES];
    int active;           // Currently animating
} Meteor;

// Function prototypes
void init_stars(Star *stars, int count, int screen_width, int screen_height);
void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height);
void render_stars(Star *stars, int count, int screen_width, int screen_height);
void render_gradient_background(int screen_width, int screen_height);
void init_meteor(Meteor *meteor, int screen_width, int screen_height);
void render_meteor(Meteor *meteor, int screen_width, int screen_height);
void update_meteor(Meteor *meteor, float dt, int screen_width, int screen_height);
void init_opengl(int width, int height);
void usage(const char *prog);

// Main function
int main(int argc, char *argv[]) {
    // Parse command line arguments
    float speed_mult = 1.0f;
    float star_density = 0.5f;
    float meteor_freq = 1.0f;
    int ch;

    while ((ch = getopt(argc, argv, "s:d:m:h")) != -1) {
        switch (ch) {
            case 's':
                speed_mult = atof(optarg);
                break;
            case 'd':
                star_density = atof(optarg);
                if (star_density < 0) star_density = 0;
                if (star_density > 1) star_density = 1;
                break;
            case 'm':
                meteor_freq = atof(optarg);
                if (meteor_freq < 0) meteor_freq = 0;
                if (meteor_freq > 5) meteor_freq = 5;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    // Force Wayland for Hyprland compatibility
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");

    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Get display info for auto-detection
    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    int screen_width = dm.w;
    int screen_height = dm.h;

    // PRE-CALCULATE ALL BUILDING PROPERTIES - ONLY ONCE AT STARTUP
    // NOW WITH VARIABLE WIDTHS (UP TO 50% WIDER)
    for (int build_idx = 0; build_idx < CITY_BUILDINGS; build_idx++) {
        float spacing = (float)screen_width / CITY_BUILDINGS;
        float random_offset = (float)(rand() % (int)(spacing * 0.6f) - spacing * 0.3f);

        // Store static building properties
        buildings[build_idx].x = (float)build_idx * spacing + random_offset + 15;

        // VARIABLE WIDTHS: from base (12-32px) up to 50% wider for architectural variation
        float base_width = (float)(12 + rand() % 20); // 12-32 pixels
        float width_multiplier = 1.25f + (float)rand() / RAND_MAX * 0.75f; // 1.25-2.0x (25-50% wider)
        buildings[build_idx].width = base_width * width_multiplier; // 15-64 pixels wide

        buildings[build_idx].height = 120.0f + (float)(rand() % 150); // 120-270 pixels tall
        buildings[build_idx].y = 50.0f; // From bottom - all buildings align at same y
        buildings[build_idx].right_edge = buildings[build_idx].x + buildings[build_idx].width;
    }

    // CALCULATE CONTINUOUS STAR FIELD ACROSS ENTIRE SCREEN WIDTH
    // Stars will be positioned across full horizontal range with vertical density control
    gap_stars = malloc(GAP_STAR_COUNT * sizeof(Star));

    // Stars are now distributed across full screen width (no gaps - everything is "open area")
    // Vertical distribution controls where stars appear relative to buildings

    // CREATE CONTINUOUS STAR FIELD WITH GRADUAL HEIGHT-BASED DENSITY ACROSS ENTIRE WIDTH
    int star_idx = 0;

    // Find the maximum building height to determine blend zones
    float max_building_height = 0.0f;
    for (int i = 0; i < CITY_BUILDINGS; i++) {
        if (buildings[i].height > max_building_height) {
            max_building_height = buildings[i].height;
        }
    }

    // Define density zones for continuous coverage
    // Zone 1: Building area (dense - near ground level)
    // Zone 2: Above building tops (gradually decreasing)
    // Zone 3: Upper atmosphere (matching sky density)

    float building_top_level = buildings[0].y + max_building_height; // Top of tallest building
    float zone2_end = building_top_level + (max_building_height * 0.5f); // 50% above buildings
    float zone3_start = screen_height / 4; // Where sky stars begin

    // Create stars across entire screen width with height-based density
    for (int j = 0; j < GAP_STAR_COUNT; j++) {
        Star *star = &gap_stars[star_idx];

        // Position across full screen width (not just gaps)
        star->x = (float)rand() / RAND_MAX * screen_width;

        // Position with gradual density decrease from building level to full height
        // Map star to different vertical zones with varying density
        float rand_val = (float)rand() / RAND_MAX;

        if (rand_val < 0.6f) {
            // 60% of stars in dense building level zone (near ground, between buildings)
            star->y = buildings[0].y + rand_val / 0.6f * max_building_height;
        } else if (rand_val < 0.9f) {
            // 30% of stars in medium density zone (above building tops)
            float zone_progress = (rand_val - 0.6f) / 0.3f; // 0-1 in this zone
            star->y = building_top_level + zone_progress * (zone2_end - building_top_level);
        } else {
            // 10% of stars in light upper zone (towards where sky stars begin)
            float zone_progress = (rand_val - 0.9f) / 0.1f; // 0-1 in this zone
            star->y = zone3_start + zone_progress * (screen_height - zone3_start);
        }

        // Define motion
        star->vx = (float)(rand() % 20 - 10) / 500.0f; // Very slow drift
        star->vy = (float)(rand() % 20 - 10) / 500.0f;

        // Standard star properties - slightly dimmer than sky stars for layering
        star->base_brightness = 0.3f + (float)(rand() % 50) / 100.0f; // 0.3-0.8 range
        star->brightness = star->base_brightness;
        star->twinkle_phase = (float)(rand() % 628) / 100.0f;
        star->twinkle_speed = 0.6f + (float)(rand() % 80) / 100.0f; // Varied twinkling
        star->size = 0.8f + (float)(rand() % 15) / 10.0f; // Smaller 0.8-2.3 pixels
        star->is_bright = (rand() % 100) < 15; // 15% bright gap stars (fewer than sky)
        star->building_gap = -1; // No longer tracking specific gaps

        star_idx++;
    }

    // Create fullscreen window using SDL_WINDOW_FULLSCREEN_DESKTOP for proper Hyprland integration
    SDL_Window *window = SDL_CreateWindow("Starry Night",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          screen_width, screen_height,
                                          SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL);

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create OpenGL context for hardware acceleration
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "GL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize OpenGL
    init_opengl(screen_width, screen_height);

    // FORCE IMMEDIATE BUFFER SWAPPING - DISABLE ALL FADING EFFECTS
    SDL_GL_SetSwapInterval(0);  // Disable VSYNC and any fade transitions
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // Ensure double buffering
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); // Stencil buffer for masking

    // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    SDL_ShowCursor(0);  // Hide mouse cursor

    // BUILDING TEMPLATES AND COMPLEX LIGHTING LOGIC REMOVED - CLEANING UP VISUAL ARTIFACTS

    // Initialize star system - adjust count based on density setting
    int actual_star_count = (int)(STAR_COUNT * (0.3f + star_density * 0.7f)); // 225-750 stars
    Star *stars = (Star *)malloc(actual_star_count * sizeof(Star));
    init_stars(stars, actual_star_count, screen_width, screen_height);

    // Initialize meteor system
    Meteor meteors[METEOR_COUNT];
    for (int i = 0; i < METEOR_COUNT; i++) {
        meteors[i].life = 0;  // Start inactive
    }

    // OLD BACKGROUND STAR SYSTEM REMOVED - Replaced with gap stars that fill spaces between buildings

    Uint64 last_time = SDL_GetTicks64();
    float meteor_timer = 0;

    // Main animation loop
    SDL_Event event;
    bool running = true;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    running = false;
                    break;
            }
        }

        Uint64 current_time = SDL_GetTicks64();
        float dt = (float)(current_time - last_time) / 1000.0f;
        last_time = current_time;

        // Update sky stars and gap stars
        update_stars(stars, actual_star_count, dt * speed_mult, screen_width, screen_height);
        update_stars(gap_stars, GAP_STAR_COUNT, dt * speed_mult, screen_width, screen_height);

        // Update and handle meteors - much more frequent for visibility
        meteor_timer += dt * speed_mult;
        float meteor_interval = 3.0f / meteor_freq; // Base interval of 3 seconds, adjusted by frequency

        if (meteor_timer >= meteor_interval) {
            meteor_timer -= meteor_interval;

            // Find inactive meteor to activate
            for (int i = 0; i < METEOR_COUNT; i++) {
                if (meteors[i].life <= 0) {
                    init_meteor(&meteors[i], screen_width, screen_height);
                    break;
                }
            }
        }

        // Update active meteors
        for (int i = 0; i < METEOR_COUNT; i++) {
            if (meteors[i].life > 0) {
                update_meteor(&meteors[i], dt * speed_mult, screen_width, screen_height);
            }
        }

        // BUILDING LIGHT FILLING SYSTEM REMOVED - No more gradual light increases
        // Render scene - DISABLE all clearing to eliminate ANY possible fade effects
        // glClear(GL_COLOR_BUFFER_BIT);

        // Clear stencil buffer to 0 for masking
        glClearStencil(0);
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);

        // Render solid black background first
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBegin(GL_QUADS);
        glColor3f(0.0f, 0.0f, 0.0f); // ABSOLUTE PURE BLACK
        glVertex2f(0.0f, 0.0f);
        glVertex2f(screen_width, 0.0f);
        glVertex2f(screen_width, screen_height);
        glVertex2f(0.0f, screen_height);
        glEnd();

        // SETUP STENCIL FOR BUILDING MASKING
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  // Disable color writing
        glStencilFunc(GL_ALWAYS, 1, 0xFF);  // Write 1 to stencil where buildings are
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        // Draw building masks to stencil buffer (invisible on screen)
        // NOTE: Now using PRE-CALCULATED STATIC building properties (no flicker!)
        for (int build_idx = 0; build_idx < CITY_BUILDINGS; build_idx++) {
            Building *building = &buildings[build_idx];
            float build_x_start = building->x;
            float build_y_start = building->y;
            float build_width = building->width;
            float build_height = building->height;

            glBegin(GL_QUADS);
            glVertex2f(build_x_start, build_y_start);
            glVertex2f(build_x_start + build_width, build_y_start);
            glVertex2f(build_x_start + build_width, build_y_start + build_height);
            glVertex2f(build_x_start, build_y_start + build_height);
            glEnd();
        }

        // ENABLE STENCIL MASKING - only render where stencil is 0 (not buildings)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);  // Re-enable color writing
        glStencilFunc(GL_EQUAL, 0, 0xFF);  // Only render where stencil is 0
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        // Render sky stars (buildings static, stars work normally)
        render_stars(stars, actual_star_count, screen_width, screen_height);

        // RENDER GAP STARS BETWEEN BUILDINGS - NO STENCIL NEEDED, they are in open areas
        render_stars(gap_stars, GAP_STAR_COUNT, screen_width, screen_height);

        // No stencil operations needed for gap stars - they render in open spaces

        // MAKE BUILDINGS VISIBLE - render them on top of stars
        // NOW BACK TO YELLOW BUILDINGS AS REQUESTED
        glColor3f(1.0f, 0.756f, 0.027f); // Mustard yellow buildings (RGB: 255, 193, 7)
        for (int build_idx = 0; build_idx < CITY_BUILDINGS; build_idx++) {
            Building *building = &buildings[build_idx];
            float build_x_start = building->x;
            float build_y_start = building->y;
            float build_width = building->width;
            float build_height = building->height;

            glBegin(GL_QUADS);
            glVertex2f(build_x_start, build_y_start);
            glVertex2f(build_x_start + build_width, build_y_start);
            glVertex2f(build_x_start + build_width, build_y_start + build_height);
            glVertex2f(build_x_start, build_y_start + build_height);
            glEnd();
        }

        // Render meteors
        for (int i = 0; i < METEOR_COUNT; i++) {
            if (meteors[i].life > 0) {
                render_meteor(&meteors[i], screen_width, screen_height);
            }
        }

        // NO WINDOW RENDERING AT ALL - removed entire window rendering system

        // Swap buffers
        SDL_GL_SwapWindow(window);
        SDL_Delay(16); // Cap at ~60 FPS
    }

    // Cleanup
    free(stars);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "Starry Night Screensaver for Hyprland/Wayland\n\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default 1.0)\n");
    fprintf(stderr, "  -d F    Star density 0.0-1.0 (default 0.5)\n");
    fprintf(stderr, "  -m F    Meteor frequency multiplier (default 1.0)\n");
    fprintf(stderr, "  -h      Show this help\n\n");
    fprintf(stderr, "Run with: SDL_VIDEODRIVER=wayland ./starrynight\n");
    fprintf(stderr, "Exit with ESC or mouse/keyboard input after 5s delay\n");
}

void init_opengl(int width, int height) {
    glViewport(0, 0, width, height);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Enable blending for transparency effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Point smoothing for star glow
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0f);

    // Enable stencil buffer for building masks
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Disable depth test for 2D rendering
    glDisable(GL_DEPTH_TEST);
}

void init_stars(Star *stars, int count, int screen_width, int screen_height) {
    for (int i = 0; i < count; i++) {
        stars[i].x = (float)(rand() % screen_width);
        stars[i].y = (float)(screen_height / 4 + (rand() % (screen_height * 3 / 4))); // Avoid bottom quarter

        // Gentle drifting motion
        stars[i].vx = -0.1f - (float)(rand() % 4) / 10.0f; // Subtle leftward drift
        stars[i].vy = (float)(rand() % 10 - 5) / 20.0f;    // Very slight vertical drift

        stars[i].base_brightness = 0.5f + (float)(rand() % 5) / 10.0f; // 0.5-1.0
        stars[i].brightness = stars[i].base_brightness;
        stars[i].twinkle_phase = (float)(rand() % 628) / 100.0f; // 0-~6.28 for sin waves
        stars[i].twinkle_speed = 0.5f + (float)(rand() % 150) / 100.0f; // 0.5-2.0 range

        // Size variation
        stars[i].size = 1.0f + (float)(rand() % 20) / 10.0f; // 1.0-3.0 pixels

    // Some stars are extra bright
    stars[i].is_bright = (rand() % 100) < 15; // 15% are bright
    }
}

void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height) {
    static float time = 0;
    time += dt;

    for (int i = 0; i < count; i++) {
        Star *s = &stars[i];

        // Update position with gentle drift (no settling behavior anymore)
        s->x += s->vx * dt;
        s->y += s->vy * dt;

        // Wrap horizontally only
        if (s->x < 0) s->x = screen_width;
        if (s->x > screen_width) s->x = 0;

        // All stars stay within bounds vertically
        if (s->y < 20) s->y = screen_height - 20; // Avoid bottom area for floating stars
        if (s->y > screen_height - 20) s->y = 20;

        // Update twinkling brightness with sine wave
        float twinkle_offset = sinf(time * s->twinkle_speed + s->twinkle_phase) * 0.4f;
        s->brightness = s->base_brightness + twinkle_offset;

        // Clamp brightness
        if (s->brightness < 0.2f) s->brightness = 0.2f;
        if (s->brightness > 1.0f) s->brightness = 1.0f;
    }
}

void render_stars(Star *stars, int count, int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    glBegin(GL_POINTS);

    for (int i = 0; i < count; i++) {
        Star *s = &stars[i];

        // Color: slight yellow tint for warmer stars
        float r = 1.0f, g = 1.0f, b = 0.9f;

        // Extra bright stars get more yellow
        if (s->is_bright) {
            g = 0.95f;
            b = 0.85f;
        }

        // Set star color with brightness
        glColor4f(r, g, b, s->brightness);

        // Draw the star point
        glVertex2f(s->x, s->y);

        // Bright stars get a glow effect (multiple translucent points)
        if (s->is_bright && s->brightness > 0.8f) {
            glColor4f(r, g, b, s->brightness * 0.3f);
            glVertex2f(s->x - 1, s->y);
            glVertex2f(s->x + 1, s->y);
            glVertex2f(s->x, s->y - 1);
            glVertex2f(s->x, s->y + 1);
        }
    }

    glEnd();
}

void render_gradient_background(int screen_width, int screen_height) {
    // Completely disable any fading - pure solid black background
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); // Pure black
    glVertex2f(0.0f, 0.0f);
    glVertex2f(screen_width, 0.0f);
    glVertex2f(screen_width, screen_height);
    glVertex2f(0.0f, screen_height);
    glEnd();
}

void init_meteor(Meteor *meteor, int screen_width __attribute__((unused)), int screen_height) {
    meteor->x = -50; // Start off-screen left
    meteor->y = screen_height * 0.6f + (rand() % (screen_height / 3)); // Upper third

    // Diagonal downward-right movement
    meteor->vx = 250 + rand() % 150; // 250-400 px/sec
    meteor->vy = 100 + rand() % 100; // 100-200 px/sec

    meteor->life = 1.0f; // Full life
    meteor->active = true;

    // Initialize particle trail positions
    for (int i = 0; i < METEOR_PARTICLES; i++) {
        meteor->tail_particlesx[i] = meteor->x;
        meteor->tail_particlesy[i] = meteor->y;
        meteor->tail_alphas[i] = 0.0f;
    }
}

void update_meteor(Meteor *meteor, float dt, int screen_width,
                  __attribute__((unused)) int screen_height) {
    if (meteor->life <= 0) return;

    meteor->x += meteor->vx * dt;
    meteor->y -= meteor->vy * dt; // Downward
    meteor->life -= dt * 1.2f;    // Fade over ~1.2-2.0 seconds

    // Update particle trail
    for (int i = METEOR_PARTICLES - 1; i > 0; i--) {
        meteor->tail_particlesx[i] = meteor->tail_particlesx[i-1];
        meteor->tail_particlesy[i] = meteor->tail_particlesy[i-1];
        meteor->tail_alphas[i] = meteor->tail_alphas[i-1];
    }

    meteor->tail_particlesx[0] = meteor->x;
    meteor->tail_particlesy[0] = meteor->y;
    meteor->tail_alphas[0] = meteor->life;

    // Deactivate when off screen or faded
    if (meteor->life <= 0 || meteor->x > screen_width + 100 || meteor->y < -100) {
        meteor->active = false;
        meteor->life = 0;
    }
}

void render_meteor(Meteor *meteor, int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    if (meteor->life <= 0 || !meteor->active) return;

    // Render particle trail
    glBegin(GL_POINTS);
    glColor3f(0.8f, 0.9f, 1.0f); // Bright blue-white

    for (int i = 0; i < METEOR_PARTICLES; i++) {
        if (meteor->tail_alphas[i] > 0.1f) {
            glColor4f(0.8f, 0.9f, 1.0f, meteor->tail_alphas[i]);
            glVertex2f(meteor->tail_particlesx[i], meteor->tail_particlesy[i]);
        }
    }
    glEnd();

    // Render bright head of meteor
    glBegin(GL_POINTS);
    glColor4f(1.0f, 1.0f, 1.0f, meteor->life);
    glVertex2f(meteor->x, meteor->y);
    glEnd();
}
