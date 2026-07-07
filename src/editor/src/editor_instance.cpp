#include <editor_instance.h>

#include <render/render_target.h>
#include <window.h>

void
ge_editor_instance::on_game_started() {
    main_window = create_window_maximized("glow engine editor");
    main_render_target = new ge_render_target();

    main_window->set_render_target(main_render_target);
}

void
ge_editor_instance::on_game_finished() {
    main_window->set_render_target(nullptr);
    main_window = nullptr;

    delete main_render_target;
    main_render_target = nullptr;
}
