#include <window.h>

#include <io/log.h>
#include <game_instance.h>
#include <SDL3/SDL_events.h>

ge_window::ge_window(ge_game_instance* game_instance, SDL_Window* sdl_window, bool is_fullscreen) {
    this->game_instance = game_instance;
    this->sdl_window = sdl_window;
    this->fullscreen = is_fullscreen;
    render_target = nullptr;
    swap_chain = nullptr;
    requested_to_close = false;
}

ge_window::~ge_window() {
    if (swap_chain != nullptr) {
        delete swap_chain;
    }
    game_instance->on_before_window_destructed(this);
}

void
ge_window::close() {
    requested_to_close = true;
}

void
ge_window::get_size(unsigned int& width, unsigned int& height) {
    int x, y;
    SDL_GetWindowSize(sdl_window, &x, &y);
    width = (unsigned int)x;
    height = (unsigned int)y;
}

bool
ge_window::is_fullscreen() {
    return fullscreen;
}

ge_swap_chain*&
ge_window::get_swap_chain() {
    return swap_chain;
}

#if defined(WIN32)
HWND
ge_window::get_win32_handle() {
    SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
    return (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}
#endif

void
ge_window::set_render_target(ge_render_target* target) {
    render_target = target;
}

void
ge_window::process_window_events() {
    // note: don't check requested_to_close here, game instance will check that

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case (SDL_EVENT_WINDOW_RESIZED):
            case (SDL_EVENT_WINDOW_MAXIMIZED):
            case (SDL_EVENT_WINDOW_MINIMIZED): {
                game_instance->on_window_size_changed(this);
                break;
            }
            case (SDL_EVENT_QUIT): {
                requested_to_close = true;
                break;
            }
        }
    }
}

void
ge_window::draw_render_target() {
    if (render_target == nullptr) {
        return;
    }

    // TODO
}