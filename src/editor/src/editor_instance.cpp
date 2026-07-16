#include <editor_instance.h>

#include <render/render_target.h>
#include <world/world_manager.h>
#include <node/camera_node.h>
#include <world/world.h>
#include <render/swap_chain.h>
#include <window.h>

void
ge_editor_instance::on_game_started() {
    // Create window.
    window = create_window_maximized("glow engine editor");
    unsigned int rt_width, rt_height;
    window->get_swap_chain()->get_size(rt_width, rt_height);

    // Create render target.
    render_target = ge_render_target::create(get_renderer(), rt_width, rt_height);
    window->set_render_target(render_target);

    // Setup sample world.
    ge_world* world = get_world_manager()->create_world();
    viewport_camera = new ge_camera_node();
    viewport_camera->set_render_target(render_target);
    world->get_root_node()->attach_child_node(viewport_camera);
}

void
ge_editor_instance::on_game_finished() {
    delete render_target;
    render_target = nullptr;
}
