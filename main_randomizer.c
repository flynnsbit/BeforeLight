#include <SDL.h>
#include <SDL_ttf.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d N    Duration per screensaver in seconds (default: 45)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -r 0|1  Show effect name during transitions (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

typedef struct {
    char name[64];
    char path[256];
    int requires_ttf;
} ScreenSaver;

int main(int argc, char *argv[]) {
    int opt;
    int duration = 45;  // seconds per screensaver
    int show_names = 1;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "d:f:r:h")) != -1) {
        switch (opt) {
            case 'd':
                duration = atoi(optarg);
                if (duration < 10) duration = 10;
                if (duration > 300) duration = 300;
                break;
            case 'f':
                do_fullscreen = atoi(optarg);
                break;
            case 'r':
                show_names = atoi(optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Build list of available screensavers
    const char *build_path = "./build/";
    DIR *dir = opendir(build_path);
    if (!dir) {
        SDL_Log("Cannot open build directory: %s", build_path);
        return 1;
    }

    ScreenSaver screensavers[32];
    int screenaver_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Look for files starting with common prefixes
        const char *name = entry->d_name;
        if ((strncmp(name, "fishsaver", 9) == 0) ||
            (strncmp(name, "bouncingball", 12) == 0) ||
            (strncmp(name, "globe", 5) == 0) ||
            (strncmp(name, "hardrain", 8) == 0) ||
            (strncmp(name, "warp", 4) == 0) ||
            (strncmp(name, "toastersaver", 12) == 0) ||
            (strncmp(name, "messages", 8) == 0) ||
            (strncmp(name, "logo", 4) == 0) ||
            (strncmp(name, "rainstorm", 9) == 0) ||
            (strncmp(name, "spotlight", 9) == 0) ||
            (strncmp(name, "lifeforms", 9) == 0) ||
            (strncmp(name, "fadeout", 7) == 0) ||
            (strncmp(name, "matrix", 6) == 0)) {

            if (screenaver_count >= 32) break;

            strcpy(screensavers[screenaver_count].name, name);
            sprintf(screensavers[screenaver_count].path, "%s%s", build_path, name);

            // Mark which ones require SDL_ttf
            screensavers[screenaver_count].requires_ttf = 0;
            if (strstr(name, "messages") != NULL || strstr(name, "matrix") != NULL) {
                screensavers[screenaver_count].requires_ttf = 1;
            }

            screenaver_count++;
        }
    }
    closedir(dir);

    if (screenaver_count == 0) {
        SDL_Log("No screensavers found in %s", build_path);
        return 1;
    }

    SDL_Log("Found %d screensavers to randomize between:", screenaver_count);
    for (int i = 0; i < screenaver_count; i++) {
        SDL_Log("  %s", screensavers[i].name);
    }

    srand(time(NULL));

    // Initialize SDL for text display if needed
    TTF_Font *font = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Load font for displaying effect names
    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
        "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/TTF/FreeMonoBold.ttf"
    };
    for (size_t i = 0; i < sizeof(font_paths) / sizeof(char*); i++) {
        font = TTF_OpenFont(font_paths[i], 18);
        if (font) break;
    }

    // Create window for displaying effect names
    SDL_Window *window = SDL_CreateWindow("Randomizer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 400, 100, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        if (font) TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        if (font) TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    pid_t child_pid = -1;
    Uint32 start_time = SDL_GetTicks();
    int current_index = -1;
    int running_child = 0;

    while (1) {
        // Check for user input to stop randomization
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                if (running_child) {
                    kill(child_pid, SIGTERM);
                    int status;
                    waitpid(child_pid, &status, 0);
                }
                goto cleanup;
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float elapsed = (current_time - start_time) / 1000.0f;

        // Time to switch screensavers?
        if (elapsed >= duration || current_index == -1) {
            // Kill current screensaver if running
            if (running_child) {
                kill(child_pid, SIGTERM);
                int status;
                waitpid(child_pid, &status, 0);
                running_child = 0;
            }

            // Select new random screensaver (different from current)
            int new_index;
            do {
                new_index = rand() % screenaver_count;
            } while (new_index == current_index && screenaver_count > 1);

            current_index = new_index;

            // Launch new screensaver
            pid_t pid = fork();
            if (pid == 0) {
                // Child process: execute screensaver
                char fullscreen_arg[16];  // Larger buffer for safety
                sprintf(fullscreen_arg, "-f%d", do_fullscreen);
                execl(screensavers[current_index].path, screensavers[current_index].name, fullscreen_arg, NULL);

                // If execl fails, exit child
                _exit(1);
            } else if (pid > 0) {
                child_pid = pid;
                running_child = 1;
                SDL_Log("Launching: %s (PID: %d)", screensavers[current_index].name, pid);
            } else {
                SDL_Log("Failed to fork for screensaver");
            }

            start_time = SDL_GetTicks();

            // Display transition message
            if (show_names) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                if (font) {
                    SDL_Color white = {255, 255, 255, 255};
                    char display_text[128];
                    sprintf(display_text, "Now Playing: %s", screensavers[current_index].name);

                    SDL_Surface *text_surf = TTF_RenderText_Blended(font, display_text, white);
                    if (text_surf) {
                        SDL_Texture *text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
                        SDL_FreeSurface(text_surf);

                        if (text_tex) {
                            SDL_Rect dst_rect = {
                                (400 - text_surf->w) / 2,
                                (100 - text_surf->h) / 2,
                                text_surf->w,
                                text_surf->h
                            };
                            SDL_RenderCopy(renderer, text_tex, NULL, &dst_rect);
                            SDL_DestroyTexture(text_tex);
                        }
                    }

                    SDL_RenderPresent(renderer);

                    // Show transition for 3 seconds, but continue monitoring for input
                    Uint32 transition_start = SDL_GetTicks();
                    while ((SDL_GetTicks() - transition_start) < 3000) {
                        SDL_Delay(100);  // Brief delay to reduce CPU usage
                        // Check for escape input during transition
                        while (SDL_PollEvent(&e)) {
                            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                                if (running_child) {
                                    kill(child_pid, SIGTERM);
                                    int status;
                                    waitpid(child_pid, &status, 0);
                                }
                                goto cleanup;
                            }
                        }
                    }
                }
            }
        }

        SDL_Delay(500);  // Main loop delay
    }

cleanup:
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
