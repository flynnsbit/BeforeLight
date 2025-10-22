#include <SDL.h>
#include <SDL_image.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
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

int x_error_handler(Display *display, XErrorEvent *error) {
    // Ignore X errors and continue
    (void)display; // Suppress unused parameter warning
    (void)error;   // Suppress unused parameter warning
    return 0;
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

    // Take screenshot before creating window
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        SDL_Log("Cannot open X display for screenshot");
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Screen *screen = DefaultScreenOfDisplay(display);
    int screen_num = DefaultScreen(display);
    Window root = RootWindow(display, screen_num);
    int screen_width = WidthOfScreen(screen);
    int screen_height = HeightOfScreen(screen);

    // Change to img directory first to load fallback
    chdir("..");
    chdir("img");

    // Set custom error handler to prevent X errors from killing the program
    XSetErrorHandler(x_error_handler);

    SDL_Surface *screenshot_surf = NULL;
    XImage *ximg = XGetImage(display, root, 0, 0, screen_width, screen_height, AllPlanes, ZPixmap);
    if (!ximg) {
        // Try fallback with different visual
        ximg = XGetImage(display, root, 0, 0, screen_width, screen_height, AllPlanes, XYPixmap);
    }
    if (ximg) {
        // Create SDL surface from XImage
        screenshot_surf = SDL_CreateRGBSurface(0, screen_width, screen_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
        if (screenshot_surf) {
            // Copy pixel data from XImage to SDL surface
            for (int y = 0; y < screen_height; y++) {
                for (int x = 0; x < screen_width; x++) {
                    unsigned long pixel = XGetPixel(ximg, x, y);
                    unsigned char r = (pixel >> 16) & 0xff;
                    unsigned char g = (pixel >> 8) & 0xff;
                    unsigned char b = pixel & 0xff;
                    Uint32 color = SDL_MapRGB(screenshot_surf->format, r, g, b);
                    ((Uint32*)screenshot_surf->pixels)[y * screenshot_surf->w + x] = color;
                }
            }
        }
        XDestroyImage(ximg);
    } else {
        SDL_Log("Cannot capture screen, creating gradient background");
        // Create a fallback surface with gradient if capture fails
        screenshot_surf = SDL_CreateRGBSurface(0, 800, 600, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
        if (screenshot_surf) {
            // Fill with radial gradient for demonstration
            float center_x = 400, center_y = 300;
            for (int y = 0; y < 600; y++) {
                for (int x = 0; x < 800; x++) {
                    float dist = sqrtf((x - center_x)*(x - center_x) + (y - center_y)*(y - center_y));
                    float factor = 1.0f - dist / sqrtf(center_x*center_x + center_y*center_y);
                    if (factor < 0) factor = 0;
                    if (factor > 1) factor = 1;
                    unsigned char r = (unsigned char)(factor * 100);
                    unsigned char g = (unsigned char)(factor * 150);
                    unsigned char b = (unsigned char)(factor * 200);
                    Uint32 color = SDL_MapRGB(screenshot_surf->format, r, g, b);
                    ((Uint32*)screenshot_surf->pixels)[y * 800 + x] = color;
                }
            }
        }
    }

    XCloseDisplay(display);

    if (!screenshot_surf) {
        SDL_Log("No background available");
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Now create window
    SDL_Window *window = SDL_CreateWindow("Fade Out", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_FreeSurface(screenshot_surf);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_FreeSurface(screenshot_surf);
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

    // Create texture from screenshot
    SDL_Texture *bg_tex = SDL_CreateTextureFromSurface(renderer, screenshot_surf);
    if (!bg_tex) {
        SDL_Log("Cannot create texture from screenshot: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_FreeSurface(screenshot_surf);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_FreeSurface(screenshot_surf);

    // Fade out parameters
    float max_radius;           // Maximum radius needed to cover screen
    float hole_x = W / 2.0f, hole_y = H / 2.0f; // Center of screen
    max_radius = sqrtf(W*W/4.0f + H*H/4.0f) + 50; // Diagonal + margin

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();
    float fade_progress = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float time_s = (current_time - start_time) / 1000.0f;

        // Fade progress (takes about 5 seconds to complete at normal speed)
        fade_progress = (time_s * speed_mult) / 5.0f;
        if (fade_progress > 1) fade_progress = 1;

        // Calculate current hole radius based on progress
        // Exponential growth for black hole effect
        float radius_progress = fade_progress * fade_progress * fade_progress; // Cubic for slower start, faster end
        float current_radius = 10.0f + radius_progress * (max_radius - 10.0f);

        // Also fade the overall screen to black as radius grows
        float screen_fade = radius_progress * 255.0f;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw background texture
        if (bg_tex) {
            SDL_SetTextureAlphaMod(bg_tex, (Uint8)(255 - screen_fade)); // Fade background inverse to black hole
            SDL_RenderCopy(renderer, bg_tex, NULL, NULL);
        }

        // Draw black hole (large filled circle)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Draw multiple concentric circles for smoother black hole
        int steps = (int)current_radius / 3;
        for (int i = 0; i <= steps; i++) {
            float r = (float)i * 3;
            if (r > current_radius) r = current_radius;

            int num_segments = 32 + i * 2; // More segments for larger circles
            if (num_segments > 128) num_segments = 128;

            // Draw filled circle using triangles
            for (int seg = 0; seg < num_segments; seg++) {
                float angle1 = seg * (2 * PI) / num_segments;
                float angle2 = (seg + 1) * (2 * PI) / num_segments;

                float x1 = hole_x + r * cosf(angle1);
                float y1 = hole_y + r * sinf(angle1);
                float x2 = hole_x + r * cosf(angle2);
                float y2 = hole_y + r * sinf(angle2);

                SDL_RenderDrawLine(renderer, (int)hole_x, (int)hole_y, (int)x1, (int)y1);
                SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
            }
        }

        // Add some dynamic elements near the black hole edge
        if (current_radius < max_radius * 0.8f) {
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 100); // Dark gray tendrils

            // Draw some tendrils curving toward the black hole
            int num_tendrils = 8;
            for (int i = 0; i < num_tendrils; i++) {
                float angle = (2 * PI * i) / num_tendrils;

                float start_r = current_radius - 20;
                if (start_r < 10) start_r = 10;

                // Curve the tendril
                int curve_steps = 10;
                for (int step = 0; step < curve_steps - 1; step++) {
                    float t1 = (float)step / curve_steps;
                    float t2 = (float)(step + 1) / curve_steps;

                    // Position along tendril
                    float r1 = start_r + 50 * t1;
                    float r2 = start_r + 50 * t2;

                    // Curve towards center
                    float curve_angle_1 = angle + (PI/2 - angle) * t1 * 0.3f;
                    float curve_angle_2 = angle + (PI/2 - angle) * t2 * 0.3f;

                    SDL_RenderDrawLine(renderer,
                        (int)(hole_x + r1 * cosf(curve_angle_1)),
                        (int)(hole_y + r1 * sinf(curve_angle_1)),
                        (int)(hole_x + r2 * cosf(curve_angle_2)),
                        (int)(hole_y + r2 * sinf(curve_angle_2)));
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps

        // Restart animation when complete (continuous loop)
        if (fade_progress >= 1.0f) {
            start_time = SDL_GetTicks(); // Reset for next cycle
            fade_progress = 0;
        }
    }

    // Cleanup
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
