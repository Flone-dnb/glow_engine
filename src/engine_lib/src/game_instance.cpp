#include <game_instance.h>

#include <stdlib.h>
#include <io/log.h>
#include <window.h>
#include <world/world_manager.h>
#include <render/renderer.h>
#include <misc/globals.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>

ge_game_instance::ge_game_instance() {
    ge_log_info("state:");
#if defined(ENGINE_ASAN_ENABLED)
    ge_log_info("- AddressSanitizer (ASan) is enabled, expect increased RAM usage!");
#endif
#if defined(DEBUG)
    ge_log_info("- DEBUG is defined, running debug build");
#else
    ge_log_info("- DEBUG is NOT defined, running release build");
#endif
#if defined(ENGINE_DEBUG_TOOLS)
    ge_log_info("- ENGINE_DEBUG_TOOLS is defined, debug tools are enabled");
#else
    ge_log_info("- ENGINE_DEBUG_TOOLS is NOT defined");
#endif

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        ge_log_error(SDL_GetError());
        abort();
    }

    world_manager = new ge_world_manager(this);
    renderer = ge_renderer::create(this);
    exit_game_loop = false;
    dont_modify_windows_array = false;
}

ge_game_instance::~ge_game_instance() {
    if (!windows.empty()) {
        ge_log_error_fmt("game instance is being destroyed but there are still %zu window(s) exist", windows.size());
        abort();
    }
    delete world_manager;
    delete renderer;
    SDL_Quit();
}

ge_window*
ge_game_instance::create_window(unsigned int width, unsigned int height, const char* title) {
    SDL_Window* sdl_window = SDL_CreateWindow(title, (int)width, (int)height, SDL_WINDOW_RESIZABLE);
    if (sdl_window == nullptr) {
        ge_log_error(SDL_GetError());
        abort();
    }

    int x, y;
    if (SDL_GetWindowSizeInPixels(sdl_window, &x, &y) == false) {
        ge_log_error(SDL_GetError());
        abort();
    }
    ge_log_info_fmt("created window of size %dx%d", x, y);

    ge_window* window = new ge_window(this, sdl_window, false);
    windows.push_back(window);
    renderer->on_after_new_window_created(window);

    return window;
}

ge_window*
ge_game_instance::create_window_maximized(const char* title) {
    SDL_Window* sdl_window = SDL_CreateWindow(title, 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    if (sdl_window == nullptr) {
        ge_log_error(SDL_GetError());
        abort();
    }

    int x, y;
    if (SDL_GetWindowSizeInPixels(sdl_window, &x, &y) == false) {
        ge_log_error(SDL_GetError());
        abort();
    }
    ge_log_info_fmt("created window of size %dx%d", x, y);

    ge_window* window = new ge_window(this, sdl_window, false);
    windows.push_back(window);
    renderer->on_after_new_window_created(window);

    return window;
}

ge_window*
ge_game_instance::create_window_fullscreen(const char* title) {
    SDL_Window* sdl_window = SDL_CreateWindow(title, 0, 0, SDL_WINDOW_FULLSCREEN);
    if (sdl_window == nullptr) {
        ge_log_error(SDL_GetError());
        abort();
    }

    int x, y;
    if (SDL_GetWindowSizeInPixels(sdl_window, &x, &y) == false) {
        ge_log_error(SDL_GetError());
        abort();
    }
    ge_log_info_fmt("created window of size %dx%d", x, y);

    ge_window* window = new ge_window(this, sdl_window, true);
    windows.push_back(window);
    renderer->on_after_new_window_created(window);

    return window;
}

void
ge_game_instance::on_before_window_destructed(ge_window* window) {
    SDL_DestroyWindow(window->sdl_window);

    if (!dont_modify_windows_array) {
        for (auto it = windows.begin(); it != windows.end(); it++) {
            if ((*it) == window) {
                windows.erase(it);
                return;
            }
        }
    }
}

void
ge_game_instance::stop_game_loop() {
    exit_game_loop = true;
}

const std::vector<ge_window*>&
ge_game_instance::get_windows() const {
    return windows;
}

ge_world_manager*
ge_game_instance::get_world_manager() {
    return world_manager;
}

ge_renderer*
ge_game_instance::get_renderer() {
    return renderer;
}

void
ge_game_instance::run_game_loop(unsigned int headless_tickrate) {
    const bool is_headless_mode = headless_tickrate > 0;

    on_game_started();

    // Used to calculate delta time.
    Uint64 current_time_counter = SDL_GetPerformanceCounter();
    Uint64 prev_time_counter = 0;
    float delta_time_sec = 0.0f;

    while (!exit_game_loop) {
        // Calculate delta time.
        prev_time_counter = current_time_counter;
        current_time_counter = SDL_GetPerformanceCounter();
        const float delta_time_ms =
            (float)((current_time_counter - prev_time_counter) * 1000) / (float)(SDL_GetPerformanceFrequency());
        delta_time_sec = (float)(delta_time_ms * 0.001f);

        // Process window events.
        for (auto it = windows.begin(); it != windows.end();) {
            ge_window* window = *it;

            window->process_window_events();
            if (window->requested_to_close) {
                dont_modify_windows_array = true;
                delete window;
                it = windows.erase(it);
                dont_modify_windows_array = false;
                continue;
            }

            it++;
        }
        if (!is_headless_mode && windows.empty()) {
            // Last window was closed - end game.
            ge_log_info("all windows are closed, ending game because not running in headless mode");
            break;
        }

        world_manager->on_tick();
        on_tick(delta_time_sec);

        renderer->draw_next_frame();

        for (ge_window* window : windows) {
            window->draw_render_target();
        }

        if (is_headless_mode && windows.empty()) {
            float time_took_ms = (float)((SDL_GetPerformanceCounter() - current_time_counter) * 1000)
                                 / (float)(SDL_GetPerformanceFrequency());
            float target_time_ms = 1000.0f / (float)headless_tickrate;
            float sleep_time_ms = target_time_ms - time_took_ms;
            if (sleep_time_ms <= 0.0f) {
                continue;
            }
            ge_sleep(sleep_time_ms);
        }
    }

    on_game_finished();

    // Destroy game world.
    world_manager->destroy_worlds();

    // Destroy left windows.
    dont_modify_windows_array = true;
    for (ge_window* window : windows) {
        delete window;
    }
    dont_modify_windows_array = false;
    windows.clear();
    windows.shrink_to_fit();
}
