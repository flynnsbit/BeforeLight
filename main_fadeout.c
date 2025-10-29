#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <time.h>
#include <unistd.h> // for getopt
#include "assets/omarchy_logo.h"

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

    // Try taking screenshot using grim (Wayland screenshot tool)
    SDL_Surface *screenshot_surf = NULL;
    SDL_Log("Attempting screen capture...");
    int grim_result = system("grim fadeout_temp.png > /dev/null 2>&1");
    if (grim_result == 0) {
        SDL_Log("Screen capture succeeded");
        screenshot_surf = IMG_Load("fadeout_temp.png");
        unlink("fadeout_temp.png");
    } else {
        SDL_Log("Screen capture failed (exit code %d)", grim_result);
    }

    if (!screenshot_surf) {
        SDL_Log("Cannot capture screen, using embedded Omarchy logo as fallback");
        SDL_RWops *rw = SDL_RWFromMem(omarchy_logo, omarchy_logo_len);
        if (rw) {
            screenshot_surf = IMG_Load_RW(rw, 1);  // 1 to auto-close rw
        }
        if (!screenshot_surf) {
            SDL_Log("Failed to load embedded logo: %s", IMG_GetError());
        }
    }

    if (!screenshot_surf) {
        SDL_Log("No background available");
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    int win_w = 800;
    int win_h = 600;
    int win_x = SDL_WINDOWPOS_UNDEFINED;
    int win_y = SDL_WINDOWPOS_UNDEFINED;
    SDL_Rect bounds = {0};
    if (do_fullscreen) {
        flags |= SDL_WINDOW_BORDERLESS;
        SDL_GetDisplayBounds(0, &bounds); // assume display 0
        win_w = bounds.w;
        win_h = bounds.h;
        win_x = bounds.x;
        win_y = bounds.y;
    }

    // Now create window
    SDL_Window *window = SDL_CreateWindow("Fade Out", win_x, win_y, win_w, win_h, flags);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_FreeSurface(screenshot_surf);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (do_fullscreen) {
        // Make window fullscreen in Hyprland to hide the bar
        SDL_Delay(500); // Allow window to be mapped and settled
        SDL_RaiseWindow(window); // Make the window active
        SDL_Delay(100); // Allow focus
        system("(hyprctl dispatch fullscreen > /dev/null 2>&1)");
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

    int W, H;
    if (do_fullscreen) {
        int display = SDL_GetWindowDisplayIndex(window);
        SDL_Rect bounds;
        SDL_GetDisplayBounds(display, &bounds);
        W = bounds.w;
        H = bounds.h;
        SDL_Log("Fullscreen display size: W=%d H=%d", W, H);
        // Set logical renderer size to match display
        SDL_RenderSetLogicalSize(renderer, W, H);
    } else {
        SDL_GetRendererOutputSize(renderer, &W, &H);
        SDL_Log("Renderer size: W=%d H=%d", W, H);
    }

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

    // Simple fade-to-black effect parameters
    // No additional parameters needed - just fade the whole screen

    // Hide cursor during screensaver
    system("hyprctl keyword cursor:invisible true &>/dev/null");

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();
    float fade_progress = 0;

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

        // Simple screen-wide fade to black (takes about 5 seconds to complete at normal speed)
        fade_progress = (time_s * speed_mult) / 5.0f;
        if (fade_progress > 1) fade_progress = 1;

        // Calculate fade amount (0 = fully visible, 255 = fully black)
        float fade_amount = fade_progress * 255.0f;

        // Draw background texture first (if available)
        if (bg_tex) {
            SDL_RenderCopy(renderer, bg_tex, NULL, NULL);
        }

        // Draw black overlay that fades in from transparent to opaque
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)fade_amount);
        SDL_RenderFillRect(renderer, NULL);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps

        // Restart animation when complete (continuous loop)
        if (fade_progress >= 1.0f) {
            start_time = SDL_GetTicks(); // Reset for next cycle
            fade_progress = 0;
        }
    }

    // Exit fullscreen on quit to show Waybar immediately
    system("(hyprctl dispatch fullscreen > /dev/null 2>&1)");
    SDL_Delay(200); // Allow Hyprland to process fullscreen exit

    // Cleanup - restore cursor visibility
    system("hyprctl keyword cursor:invisible false 2>/dev/null");

    // Cleanup
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
