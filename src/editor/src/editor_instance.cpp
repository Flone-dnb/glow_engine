#include <editor_instance.h>

#include <render/render_target.h>
#include <world/world_manager.h>
#include <node/camera_node.h>
#include <world/world.h>
#include <window.h>

void
ge_editor_instance::on_game_started() {
    // Create window.
    window = create_window_maximized("glow engine editor");
    unsigned int window_width, window_height;
    window->get_size(window_width, window_height);

    // Create render target.
    render_target = new ge_render_target(window_width, window_height);
    window->set_render_target(render_target);

    // Setup sample world.
    ge_world* world = get_world_manager()->create_world();
    viewport_camera = new ge_camera_node();
    viewport_camera->set_render_target(render_target);
    world->get_root_node()->attach_child_node(viewport_camera);
}

void
ge_editor_instance::on_window_size_changed(ge_window* changed_window) {
    if (changed_window != window) {
        return;
    }

    if (viewport_camera != nullptr) {
        viewport_camera->set_render_target(nullptr);
    }
    window->set_render_target(nullptr);

    delete render_target;

    unsigned int window_width, window_height;
    window->get_size(window_width, window_height);

    render_target = new ge_render_target(window_width, window_height);
    window->set_render_target(render_target);
    if (viewport_camera != nullptr) {
        viewport_camera->set_render_target(render_target);
    }
}

void
ge_editor_instance::on_game_finished() {
    delete render_target;
    render_target = nullptr;
}
