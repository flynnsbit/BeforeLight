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
#define STAR_COUNT 750  // Between 500-1000 for performance
#define METEOR_COUNT 10
#define METEOR_PARTICLES 20

typedef struct {
    float x, y;           // Position in pixels
    float vx, vy;         // Velocity for drift effect
    float brightness;     // Current brightness 0-1 (for twinkling)
    float base_brightness; // Base brightness 0-1
    float twinkle_phase;  // Sin phase offset for unique twinkling
    float twinkle_speed;  // How fast it twinkles
    float size;           // Star size in pixels
    bool is_bright;       // Extra-bright star status for glow effect
    bool settled;         // Whether this star has settled into its final position
    int settled_layer;    // What skyline layer this belongs to (for building height)
} Star;

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

    // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    SDL_ShowCursor(0);  // Hide mouse cursor

    // Initialize star system - adjust count based on density setting
    int actual_star_count = (int)(STAR_COUNT * (0.3f + star_density * 0.7f)); // 225-750 stars
    Star *stars = (Star *)malloc(actual_star_count * sizeof(Star));
    init_stars(stars, actual_star_count, screen_width, screen_height);

    // Initialize meteor system
    Meteor meteors[METEOR_COUNT];
    for (int i = 0; i < METEOR_COUNT; i++) {
        meteors[i].life = 0;  // Start inactive
    }

    Uint64 last_time = SDL_GetTicks64();
    float meteor_timer = 0;
    float elapsed = 0;

    // Variables for idle detection
    Uint32 last_input_time = SDL_GetTicks();
    bool input_detected = false;

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
                case SDL_MOUSEMOTION:
                    running = false;
                    break;
            }
        }

        Uint64 current_time = SDL_GetTicks64();
        float dt = (float)(current_time - last_time) / 1000.0f;
        last_time = current_time;

        elapsed += dt;

        // Update stars
        update_stars(stars, actual_star_count, dt * speed_mult, screen_width, screen_height);

        // Update and handle meteors
        meteor_timer += dt * speed_mult;
        float meteor_interval = 60.0f / meteor_freq; // Base interval of 60 seconds, adjusted by frequency

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

        // Render scene
        glClear(GL_COLOR_BUFFER_BIT);

        // Render gradient background
        render_gradient_background(screen_width, screen_height);

        // Render meteors (behind stars for depth)
        for (int i = 0; i < METEOR_COUNT; i++) {
            if (meteors[i].life > 0) {
                render_meteor(&meteors[i], screen_width, screen_height);
            }
        }

        // Render stars
        render_stars(stars, actual_star_count, screen_width, screen_height);

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
    stars[i].settled = false; // Start as unsettled
    stars[i].settled_layer = 0; // Will be set when settled
    }
}

void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height) {
    static float time = 0;
    time += dt;

    for (int i = 0; i < count; i++) {
        Star *s = &stars[i];

        if (!s->settled) {
            // Update position with gentle drift
            s->x += s->vx * dt;
            s->y += s->vy * dt;

            // Wrap horizontally only
            if (s->x < 0) s->x = screen_width;
            if (s->x > screen_width) s->x = 0;

            // Check if bright star should settle into skyline
            if (s->is_bright && s->y > screen_height - 100 && rand() % 100 < 2) { // Random chance when near bottom
                s->settled = true;
                s->settled_layer = rand() % 3; // 0,1,2 layers for different heights
                s->y = screen_height - 30 - s->settled_layer * 25; // Position in skyline layers
                s->vx = 0; // Stop horizontal movement
                s->vy = 0; // Stop vertical movement
                s->base_brightness = 0.8f; // Brighten settled stars (city lights)
            }
        }

        // Settled stars don't wrap vertically, they stay in skyline
        if (s->settled && s->y > screen_height - 20) s->y = screen_height - 20;
        if (!s->settled && s->y < 20) s->y = screen_height - 20; // Avoid bottom area for floating stars
        if (!s->settled && s->y > screen_height - 20) s->y = 20;

        // Update twinkling brightness with sine wave
        float twinkle_offset = sinf(time * s->twinkle_speed + s->twinkle_phase) * 0.4f;
        s->brightness = s->base_brightness + twinkle_offset;

        // Clamp brightness
        if (s->brightness < 0.2f) s->brightness = 0.2f;
        if (s->brightness > 1.0f) s->brightness = 1.0f;

        // Settled stars (city lights) have slightly yellow tint
        if (s->settled) {
            s->brightness = s->base_brightness + sinf(time * 2.0f + s->twinkle_phase) * 0.2f; // Different twinkling
            if (s->brightness < 0.7f) s->brightness = 0.7f;
        }
    }
}

void render_stars(Star *stars, int count, int screen_width, int screen_height) {
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
    // Solid black background for city skyline effect
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Pure black
    glClear(GL_COLOR_BUFFER_BIT);
}

void init_meteor(Meteor *meteor, int screen_width, int screen_height) {
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

void update_meteor(Meteor *meteor, float dt, int screen_width, int screen_height) {
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

void render_meteor(Meteor *meteor, int screen_width, int screen_height) {
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
