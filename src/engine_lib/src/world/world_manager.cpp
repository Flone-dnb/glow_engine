#include <world/world_manager.h>

#include <io/log.h>
#include <world/world.h>

ge_world_manager::ge_world_manager(ge_game_instance* game_instance) { this->game_instance = game_instance; }

ge_world_manager::~ge_world_manager() {
    if (!worlds.empty()) {
        ge_log_error_fmt("world manager is being destroyed but %zu world(s) still exist", worlds.size());
        abort();
    }
}

ge_world*
ge_world_manager::create_world() {
    ge_world* world = new ge_world(this);
    worlds.push_back(world);
    return world;
}

void
ge_world_manager::destroy_world(ge_world* world_to_destroy) {
    for (auto it = worlds.begin(); it != worlds.end(); it++) {
        ge_world* world = *it;
        if (world == world_to_destroy) {
            delete world;
            worlds.erase(it);
            return;
        }
    }

    ge_log_error("unable to find the specified world to destroy");
}

void
ge_world_manager::destroy_worlds() {
    for (ge_world* world : worlds) {
        delete world;
    }
    worlds.clear();
    worlds.shrink_to_fit();
}

ge_game_instance*
ge_world_manager::get_game_instance() {
    return game_instance;
}

void
ge_world_manager::on_tick() {
    // TODO
}