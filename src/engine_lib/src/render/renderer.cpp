#include <render/renderer.h>

#include <stdlib.h>
#include <io/log.h>

#if defined(WIN32)
#include <render/directx/directx_renderer.h>
#endif
#include <render/dummy_renderer.h>

ge_renderer*
ge_renderer::create(ge_game_instance* game_instance) {
#if defined(WIN32)
    return new ge_directx_renderer(game_instance);
#else
    ge_log_info("creating a dummy renderer (rendering is disabled)");
    return new ge_dummy_renderer(game_instance);
#endif
}

ge_renderer::ge_renderer(ge_game_instance* game_instance) { this->game_instance = game_instance; }

ge_game_instance*
ge_renderer::get_game_instance() {
    return game_instance;
}

const std::vector<ge_camera_node*>&
ge_renderer::get_registered_cameras() const {
    return registered_cameras;
}

ge_renderer::~ge_renderer() {
    if (!registered_cameras.empty()) {
        ge_log_error("the renderer is being destroyed but therer are still some cameras registered");
        abort();
    }
}

void
ge_renderer::register_camera(ge_camera_node* camera) {
    registered_cameras.push_back(camera);
}

void
ge_renderer::unregister_camera(ge_camera_node* camera) {
    for (auto it = registered_cameras.begin(); it != registered_cameras.end(); it++) {
        if ((*it) == camera) {
            registered_cameras.erase(it);
            return;
        }
    }

    ge_log_error("unable to unregister the specified camera because it wasn't registered previously");
    abort();
}
