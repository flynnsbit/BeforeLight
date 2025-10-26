#include <SDL.h>
#include <SDL_image.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <time.h>
#include <unistd.h> // for getopt
#include <stdlib.h>

#define PI 3.141592653589793f

extern char *optarg;

int x_error_handler(Display *display __attribute__((unused)), XErrorEvent *error __attribute__((unused))) {
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

    // Change to project directory for consistent paths
    chdir("..");

    // Try taking screenshot using grim (Wayland screenshot tool)
    SDL_Surface *screenshot_surf = NULL;
    int grim_result = system("grim spotlight_temp.png 2>/dev/null");
    if (grim_result == 0) {
        screenshot_surf = IMG_Load("spotlight_temp.png");
        unlink("spotlight_temp.png");
    }

    if (!screenshot_surf) {
        // Fallback to X11 method
        Display *display = XOpenDisplay(NULL);
        if (!display) {
            SDL_Log("Cannot open X display for screenshot");
        } else {
            Screen *screen = DefaultScreenOfDisplay(display);
            int screen_num = DefaultScreen(display);
            Window root = RootWindow(display, screen_num);
            int screen_width = WidthOfScreen(screen);
            int screen_height = HeightOfScreen(screen);

            // Set custom error handler to prevent X errors from killing the program
            XSetErrorHandler(x_error_handler);

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
            }
            XCloseDisplay(display);
        }
    }

    if (!screenshot_surf) {
        SDL_Log("Cannot capture screen, using fallback background image");
        // Load fallback image
        screenshot_surf = IMG_Load("img/spotlight_bg.png");
        if (!screenshot_surf) {
            SDL_Log("Cannot load img/spotlight_bg.png: %s, trying img/macos-desktop.png", IMG_GetError());
            screenshot_surf = IMG_Load("img/macos-desktop.png");
            if (!screenshot_surf) {
                SDL_Log("Cannot load img/macos-desktop.png: %s, creating dummy background", IMG_GetError());
                // Create a dummy surface with a gradient background as test
                screenshot_surf = SDL_CreateRGBSurface(0, 800, 600, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
                if (screenshot_surf) {
                    // Fill with solid white for visibility
                    for (int y = 0; y < 600; y++) {
                        for (int x = 0; x < 800; x++) {
                            Uint32 color = SDL_MapRGB(screenshot_surf->format, 255, 255, 255);
                            ((Uint32*)screenshot_surf->pixels)[y * 800 + x] = color;
                        }
                    }
                }
            }
        }
    }

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
    float radius = 120.0f;

    // Geometry setup for circular spotlight
    const int segments = 64; // more segments for smoother circle
    SDL_Vertex vertices[segments + 1];
    int indices[segments * 3];

    // Initialize triangle fan indices
    for (int i = 0; i < segments; i++) {
        int a = i;
        int b = (i + 1) % segments;
        indices[i * 3] = 0;
        indices[i * 3 + 1] = a + 1;
        indices[i * 3 + 2] = b + 1;
    }

    // Get texture dimensions
    int tex_width, tex_height;
    SDL_QueryTexture(bg_tex, NULL, NULL, &tex_width, &tex_height);
    float scale_factor = (float)W / tex_width; // scale to avoid zoom, show 1:1
    float spotlight_x = W / 2.0f;
    float spotlight_y = H / 2.0f;
    float spotlight_vx = (rand() % 400 - 200) * 1.0f;
    float spotlight_vy = (rand() % 400 - 200) * 1.0f;

    // Main loop
    SDL_Event e;
    int quit = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        // Update spotlight movement
        float dt = 0.016f;
        spotlight_x += spotlight_vx * dt * speed_mult;
        spotlight_y += spotlight_vy * dt * speed_mult;

        // Bounce off walls
        if (spotlight_x <= radius || spotlight_x >= W - radius) {
            spotlight_vx = -spotlight_vx;
            spotlight_x = spotlight_x <= radius ? radius : W - radius;
        }
        if (spotlight_y <= radius || spotlight_y >= H - radius) {
            spotlight_vy = -spotlight_vy;
            spotlight_y = spotlight_y <= radius ? radius : H - radius;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Update geometry vertices for the circular spotlight
        vertices[0].position.x = spotlight_x;
        vertices[0].position.y = spotlight_y;
        vertices[0].tex_coord.x = (spotlight_x / W) * scale_factor;
        vertices[0].tex_coord.y = (spotlight_y / H) * scale_factor;
        vertices[0].color = (SDL_Color){255, 255, 255, 255};

        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * PI * i / segments;
            float px = spotlight_x + cosf(angle) * radius;
            float py = spotlight_y + sinf(angle) * radius;
            vertices[i + 1].position.x = px;
            vertices[i + 1].position.y = py;
            vertices[i + 1].tex_coord.x = (px / W) * scale_factor;
            vertices[i + 1].tex_coord.y = (py / H) * scale_factor;
            vertices[i + 1].color = (SDL_Color){255, 255, 255, 255};
        }

        // Render only the circular spotlight area from the background texture
        SDL_RenderGeometry(renderer, bg_tex, vertices, segments + 1, indices, segments * 3);

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
