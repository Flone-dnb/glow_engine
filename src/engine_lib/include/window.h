#pragma once

class ge_game_instance;
class ge_render_target;
class ge_swap_chain;
class SDL_Window;

#if defined(WIN32)
struct HWND__;
typedef struct HWND__* HWND;
#endif

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

    // Returns size of the window in pixels.
    void get_size(unsigned int& width, unsigned int& height);

    bool is_fullscreen();

    // Returns `nullptr` if swap chain was not created yet.
    ge_swap_chain*& get_swap_chain();

#if defined(WIN32)
    HWND get_win32_handle();
#endif

  private:
    ge_window(ge_game_instance* game_instance, SDL_Window* sdl_window, bool is_fullscreen);

    // Called by game instance to process available window events.
    void process_window_events();
    
    // Called by game instance to draw from render target.
    void draw_render_target();

    // Render target that this window displays.
    ge_render_target* render_target;

    // Reference to the game instance that created this window.
    ge_game_instance* game_instance;

    // Internal window.
    SDL_Window* sdl_window;

    ge_swap_chain* swap_chain;

    bool requested_to_close;
    bool fullscreen;
};