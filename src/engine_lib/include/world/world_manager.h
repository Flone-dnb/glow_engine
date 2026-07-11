#pragma once

#include <vector>

class ge_world;
class ge_game_instance;

// Responsible for creating and destroying game world.
class ge_world_manager {
    // Game instance creates world manager.
    friend class ge_game_instance;

  public:
    ge_world_manager() = delete;
    ~ge_world_manager();

    ge_world_manager(const ge_world_manager&) = delete;
    ge_world_manager& operator=(const ge_world_manager&) = delete;

    // Do not delete returned pointer, world manager will manage the destruction,
    // to explicitly destroy use @ref destroy_world.
    ge_world* create_world();

    // If needed, use these functions to explicitly destroy worlds (before game instance destruction).
    void destroy_world(ge_world* world_to_destroy);
    void destroy_worlds();

    // Returns always valid pointer.
    ge_game_instance* get_game_instance();

  private:
    ge_world_manager(ge_game_instance* game_instance);

    // Called from game instance every frame.
    void on_tick();

    std::vector<ge_world*> worlds;

    ge_game_instance* game_instance;
};
