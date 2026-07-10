#pragma once

#include <vector>

class ge_window;
class ge_world_manager;
class ge_world_manager;
class ge_renderer;

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
    // In case you want to run in headless mode (without any game windows) you can specify a non-zero
    // tickrate (ticks per second) at which the game loop will be run.
    void run_game_loop(unsigned int headless_tickrate = 0);

    // Mark to exit game loop started in @ref run_game_loop.
    void stop_game_loop();

    // Returns all currently created windows.
    const std::vector<ge_window*>& get_windows() const;

    // Do not delete returned pointer, valid while game instance exists.
    ge_world_manager* get_world_manager();

    // Do not delete returned pointer, valid while game instance exists.
    ge_renderer* get_renderer();

  protected:
    // Called when entered @ref run_game_loop.
    virtual void on_game_started() {};

    // Called every frame.
    virtual void on_tick(float delta_time_sec) {};

    // Called after window's size changed.
    virtual void on_window_size_changed(ge_window* window) {};

    // Called before leaving @ref run_game_loop, before game worlds and created windows are destroyed.
    virtual void on_game_finished() {};

  private:
    // Called by window in its destructor.
    void on_before_window_destructed(ge_window* window);

    // All active windows. Games usually have just 1 but in the editor
    // there can be more.
    std::vector<ge_window*> windows;

    ge_world_manager* world_manager;
    ge_renderer* renderer;

    bool exit_game_loop;
    bool dont_modify_windows_array;
};