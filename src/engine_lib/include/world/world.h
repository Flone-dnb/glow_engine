#pragma once

class ge_world_manager;
class ge_node;

class ge_world {
    // Use world manager to create/destroy worlds.
    friend class ge_world_manager;

  public:
    ~ge_world();

    ge_world() = delete;
    ge_world(const ge_world&) = delete;
    ge_world& operator=(const ge_world&) = delete;

    // Do not delete returned pointer, always valid while the world exists.
    // Returns always spawned root node of the world.
    ge_node* get_root_node();

    // Returns always valid pointer.
    ge_world_manager* get_world_manager();

  private:
    ge_world(ge_world_manager* world_manager);

    // Always valid pointer, root node of the world.
    ge_node* root_node;

    ge_world_manager* world_manager;
};