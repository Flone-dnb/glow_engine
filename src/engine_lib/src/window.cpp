#include <window.h>

#include <io/log.h>
#include <game_instance.h>
#include <SDL3/SDL_events.h>

ge_window::ge_window(ge_game_instance* game_instance, SDL_Window* sdl_window) {
    this->game_instance = game_instance;
    this->sdl_window = sdl_window;
    render_target = nullptr;
    requested_to_close = false;
}

ge_window::~ge_window() { game_instance->on_before_window_destructed(this); }

void
ge_window::close() {
    requested_to_close = true;
}

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
            case (SDL_EVENT_QUIT): {
                requested_to_close = true;
            }
        }
    }
}

void
ge_window::draw() {
    if (render_target == nullptr) {
        return;
    }

    // TODO
}