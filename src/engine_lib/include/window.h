#pragma once

class ge_game_instance;
class ge_render_target;
class SDL_Window;

// A window to render (some part of) the game. There can be many windows
// (for example, in the editor) but games usually have just one.
class ge_window {
    // Use game instance to create new windows.
    friend class ge_game_instance;

  public:
    ge_window() = delete;
    ~ge_window();

    ge_window(const ge_window&) = delete;
    ge_window& operator=(const ge_window&) = delete;

    // Sets (or clears) a render target to display on the window.
    // You must make sure the pointer will be valid while the window uses it.
    void set_render_target(ge_render_target* target);

    // Marks the window to be closed.
    void close();

  private:
    ge_window(ge_game_instance* game_instance, SDL_Window* sdl_window);

    // Called by game instance to process available window events.
    void process_window_events();
    
    // Called by game instance to draw from render target.
    void draw();

    // Render target that this window displays.
    ge_render_target* render_target;

    // Reference to the game instance that created this window.
    ge_game_instance* game_instance;

    // Internal window.
    SDL_Window* sdl_window;

    bool requested_to_close;
};