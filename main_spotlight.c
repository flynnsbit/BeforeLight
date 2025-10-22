#include <SDL.h>
#include <SDL_image.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <time.h>
#include <unistd.h> // for getopt

extern char *optarg;

int x_error_handler(Display *display, XErrorEvent *error) {
    // Ignore X errors and continue
    return 0;
}

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
        SDL_Log("Cannot capture screen, using fallback background image");
        // Load fallback image
        screenshot_surf = IMG_Load("spotlight_bg.png");
        if (!screenshot_surf) {
            SDL_Log("Cannot load spotlight_bg.png: %s, trying macos-desktop.png", IMG_GetError());
            screenshot_surf = IMG_Load("macos-desktop.png");
            if (!screenshot_surf) {
                SDL_Log("Cannot load images: %s, creating dummy background", IMG_GetError());
                // Create a dummy surface with a gradient background as test
                screenshot_surf = SDL_CreateRGBSurface(0, 800, 600, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
                if (screenshot_surf) {
                    // Fill with a simple pattern
                    for (int y = 0; y < 600; y++) {
                        for (int x = 0; x < 800; x++) {
                            Uint8 r = (x * 255) / 800;
                            Uint8 g = (y * 255) / 600;
                            Uint8 b = 100;
                            Uint32 color = SDL_MapRGB(screenshot_surf->format, r, g, b);
                            ((Uint32*)screenshot_surf->pixels)[y * 800 + x] = color;
                        }
                    }
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
    SDL_Window *window = SDL_CreateWindow("Spotlight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    // Create texture from screenshot
    SDL_Texture *bg_tex = SDL_CreateTextureFromSurface(renderer, screenshot_surf);
    if (!bg_tex) {
        SDL_Log("Cannot create texture from screenshot: %s", SDL_GetError());
        SDL_FreeSurface(screenshot_surf);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_FreeSurface(screenshot_surf);

    // Spotlight properties
    float spotlight_x = W / 2.0f, spotlight_y = H / 2.0f;
    float spotlight_vx = 0, spotlight_vy = 0; // movement velocity
    int path_timer = 0; // timer for path changes
    int spotlight_radius = 120; // beam radius

    // Main loop
    SDL_Event e;
    int quit = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        // Update spotlight movement (random path)
        if (path_timer <= 0) {
            // Choose new direction and speed
            float angle = (rand() % 360) * (3.14159f / 180.0f);
            float speed = 20.0f + (rand() % 100) / 10.0f;
            spotlight_vx = cosf(angle) * speed;
            spotlight_vy = sinf(angle) * speed;
            path_timer = 60 + rand() % 120; // Change direction every 1-3 seconds
        }
        path_timer--;

        spotlight_x += spotlight_vx * speed_mult * 0.016f;
        spotlight_y += spotlight_vy * speed_mult * 0.016f;

        // Bounce off walls
        if (spotlight_x < spotlight_radius) { spotlight_x = spotlight_radius; spotlight_vx = -spotlight_vx; }
        if (spotlight_x > W - spotlight_radius) { spotlight_x = W - spotlight_radius; spotlight_vx = -spotlight_vx; }
        if (spotlight_y < spotlight_radius) { spotlight_y = spotlight_radius; spotlight_vy = -spotlight_vy; }
        if (spotlight_y > H - spotlight_radius) { spotlight_y = H - spotlight_radius; spotlight_vy = -spotlight_vy; }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Draw background texture if available
        if (bg_tex) {
            SDL_RenderCopy(renderer, bg_tex, NULL, NULL);
        }

        // Draw spotlight effect (simple white circle overlay)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        for (int r = 0; r < spotlight_radius; r += 2) {
            int diameter = r * 2;
            SDL_Rect beam = {
                (int)(spotlight_x - r), (int)(spotlight_y - r),
                diameter, diameter
            };
            Uint8 alpha = 100 - (r * 100 / spotlight_radius);
            if (alpha > 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                SDL_RenderDrawRect(renderer, &beam);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
