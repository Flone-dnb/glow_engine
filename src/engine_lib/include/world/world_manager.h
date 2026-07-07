#pragma once

// Responsible for creating and destroying game world.
class ge_world_manager {
    // Game instance creates world manager.
    friend class ge_game_instance;

  public:
    ge_world_manager(const ge_world_manager&) = delete;
    ge_world_manager& operator=(const ge_world_manager&) = delete;

    // Destroys all worlds.
    void destroy_worlds();

  private:
    ge_world_manager() = default;

    // Called from game instance every frame.
    void on_tick();
};