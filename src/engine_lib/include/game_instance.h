#pragma once

#include <vector>

class ge_window;
class ge_world_manager;
class ge_world_manager;

// Main object of a game, manages things like the renderer, game worlds, etc.
class ge_game_instance {
    // Windows notifies about destruction.
    friend class ge_window;

  public:
    // Construct to create a new game instance.
    ge_game_instance();
    virtual ~ge_game_instance();

    ge_game_instance(const ge_game_instance&) = delete;
    ge_game_instance& operator=(const ge_game_instance&) = delete;

    // Do not delete returned pointer, game instance will manage all windows.
    ge_window* create_window(unsigned int width, unsigned int height, const char* title);
    ge_window* create_window_maximized(const char* title);
    ge_window* create_window_fullscreen(const char* title);

    // Runs game loop, calls @ref on_game_started when entered the function.
    // In case your game does not have any windows (headless mode) you can specify a non-zero
    // tickrate (ticks per second) at which the game loop will be run.
    void run_game_loop(unsigned int headless_tickrate = 0);

    // Mark to exit game loop started in @ref run_game_loop.
    void stop_game_loop();

  protected:

    // Called when entered @ref run_game_loop.
    virtual void on_game_started() {};

    // Called every frame.
    virtual void on_tick(float delta_time_sec) {};

    // Called before leaving @ref run_game_loop, before game worlds and created windows are destroyed.
    virtual void on_game_finished() {};

  private:
    // Called by window in its destructor.
    void destroy_window(ge_window* window);

    // All active windows. Games usually have just 1 but in the editor
    // there can be more.
    std::vector<ge_window*> windows;

    ge_world_manager* world_manager;

    bool exit_game_loop;
};