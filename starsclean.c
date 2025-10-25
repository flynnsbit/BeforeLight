/**
 * Star Clean - Minimal star field screensaver
 * Clean, focused implementation with individually twinkling stars
 *
 * Compile: gcc -Wall -Wextra -O2 `sdl2-config --cflags` -o starsclean starsclean.c `sdl2-config --libs` -lGL -lm
 * Run: ./starsclean
 */

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define PI 3.14159265359f
#define STAR_COUNT 1500  // Clean field of twinkling stars

typedef struct {
    float x, y;           // Position in pixels
    float vx, vy;         // Velocity for drift effect
    float brightness;     // Current brightness 0-1 (for twinkling)
    float base_brightness; // Base brightness 0-1
    float twinkle_phase;  // Sin phase offset for unique twinkling
    float twinkle_speed;  // How fast it twinkles
    float size;           // Star size in pixels
    int is_bright;        // Extra-bright star status for glow effect
} Star;

/**
 * Initialize star system - create clean field of twinkling stars
 * Each star gets unique twinkling parameters for independent variation
 */
void init_stars(Star *stars, int count, int screen_width, int screen_height) {
    for (int i = 0; i < count; i++) {
        // Position: anywhere on screen (full coverage, no restrictions)
        stars[i].x = rand() % screen_width;
        stars[i].y = rand() % screen_height;

        // Velocity: gentle drifting movement
        stars[i].vx = -0.1f - (float)(rand() % 4) / 10.0f; // Subtle leftward drift
        stars[i].vy = (float)(rand() % 10 - 5) / 20.0f;    // Very slight vertical drift

        // Brightness range: smooth variation around baseline
        stars[i].base_brightness = 0.5f + (float)(rand() % 5) / 10.0f; // 0.5-1.0
        stars[i].brightness = stars[i].base_brightness;

        // Twinkling parameters: individual patterns
        stars[i].twinkle_phase = (float)(rand() % 628) / 100.0f; // Random 0-6.28 radians
        stars[i].twinkle_speed = 0.5f + (float)(rand() % 150) / 100.0f; // 0.5-2.0 range

        // Size variation (unused in current render)
        stars[i].size = 1.0f + (float)(rand() % 20) / 10.0f; // 1.0-3.0 pixels

        // Glow effect: 15% of stars get enhanced brightness
        stars[i].is_bright = (rand() % 100) < 15;
    }
}

/**
 * Update star system - handle twinkling only (drift disabled)
 * Stars remain static but oscillate brightness individually
 */
void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height) {
    static float time = 0;
    time += dt;

    for (int i = 0; i < count; i++) {
        (void)dt; (void)screen_width; (void)screen_height; // Suppress unused parameter warnings

        // STATIONS DISABLED: Keep stars static at their initial positions
        // stars[i].x += stars[i].vx * dt;
        // if (stars[i].x < 0) stars[i].x = screen_width;
        // if (stars[i].x > screen_width) stars[i].x = 0;
        // if (stars[i].y < 20) stars[i].y = screen_height - 20;
        // if (stars[i].y > screen_height - 20) stars[i].y = 20;

        // Individual twinkling: sinusoidal oscillation around base brightness
        // ±0.3f amplitude creates smooth, natural variation
        float twinkle_offset = sinf(time * stars[i].twinkle_speed + stars[i].twinkle_phase) * 0.3f;
        stars[i].brightness = stars[i].base_brightness + twinkle_offset;

        // Clamp brightness to prevent over/underflow
        if (stars[i].brightness < 0.2f) stars[i].brightness = 0.2f;
        if (stars[i].brightness > 1.0f) stars[i].brightness = 1.0f;
    }
}

/**
 * Render star field - draw all stars with individual colors and brightness
 * Bright stars get enhanced glow effect for visual richness
 */
void render_stars(Star *stars, int count, int screen_width, int screen_height) {
    (void)screen_width; (void)screen_height; // Unused parameters

    glPointSize(1.0f); // Standard star size
    glBegin(GL_POINTS);

    for (int i = 0; i < count; i++) {
        // Color: slight yellow tint for warmer appearance
        float r = 1.0f, g = 1.0f, b = 0.9f;

        // Bright stars get more yellow tint
        if (stars[i].is_bright) {
            g = 0.95f;
            b = 0.85f;
        }

        // Set color with individual star brightness
        glColor4f(r, g, b, 1.0f);  // Full alpha - twinkling controlled by brightness

        // Draw main star point
        glVertex2f(stars[i].x, stars[i].y);

        // Glow effect for bright stars - additional points for visual richness
        if (stars[i].is_bright && stars[i].brightness > 0.8f) {
            glColor4f(r, g, b, 1.0f);  // Matching glow color
            glVertex2f(stars[i].x - 1, stars[i].y);
            glVertex2f(stars[i].x + 1, stars[i].y);
            glVertex2f(stars[i].x, stars[i].y - 1);
            glVertex2f(stars[i].x, stars[i].y + 1);
        }
    }

    glEnd();
}

/**
 * Initialize OpenGL for 2D star rendering
 */
void init_opengl(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Enable blending for potential effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Smooth points for nicer star appearance
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0f);

    // Disable depth testing for 2D rendering
    glDisable(GL_DEPTH_TEST);
}

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Get display mode for fullscreen
    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    int screen_width = dm.w;
    int screen_height = dm.h;

    // Create fullscreen OpenGL window
    SDL_Window *window = SDL_CreateWindow("Star Clean",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          screen_width, screen_height,
                                          SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL);

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create OpenGL context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "GL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize OpenGL
    init_opengl(screen_width, screen_height);

    // Force Wayland compatibility
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");

    // Initialize random seed
    srand(time(NULL));

    // Create star system
    Star *stars = malloc(STAR_COUNT * sizeof(Star));
    if (!stars) {
        fprintf(stderr, "Failed to allocate memory for stars\n");
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    init_stars(stars, STAR_COUNT, screen_width, screen_height);

    // Timing variables
    Uint64 last_time = SDL_GetTicks64();
    SDL_Event event;
    int running = 1;

    // Main render loop
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_KEYDOWN:
                case SDL_MOUSEBUTTONDOWN:
                    running = 0;
                    break;
            }
        }

        // Calculate delta time
        Uint64 current_time = SDL_GetTicks64();
        float dt = (float)(current_time - last_time) / 1000.0f;
        last_time = current_time;

        // Update star system
        update_stars(stars, STAR_COUNT, dt, screen_width, screen_height);

        // Clear screen to black
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render stars
        render_stars(stars, STAR_COUNT, screen_width, screen_height);

        // Swap buffers
        SDL_GL_SwapWindow(window);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    free(stars);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
