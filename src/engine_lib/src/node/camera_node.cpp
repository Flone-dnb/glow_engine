#include <node/camera_node.h>

#include <string.h>
#include <world/world.h>
#include <world/world_manager.h>
#include <game_instance.h>
#include <render/renderer.h>
#include <render/render_target.h>
#include <io/log.h>

ge_camera_node::ge_camera_node() : ge_camera_node("Camera Node") {}

ge_camera_node::ge_camera_node(const char* name) : ge_spatial_node(name) {
    render_target = nullptr;
    registered = false;
    fov = 90;
    near_plane = 0.3f;
    far_plane = 500.0f;
    near_clip_plane_height = 0.0f;
    far_clip_plane_height = 0.0f;
    view_mat = glm::identity<glm::mat4>();
    proj_mat = glm::identity<glm::mat4>();
    memset(&frustum, 0, sizeof(ge_frustum));
}

void
ge_camera_node::set_render_target(ge_render_target* render_target) {
    this->render_target = render_target;

    if (is_spawned() && this->render_target != nullptr) {
        recalc_view_mat();
        recalc_proj_mat();
        recalc_frustum();

        register_to_render();
    }
}

void
ge_camera_node::set_fov(unsigned int fov) {
    this->fov = fov;

    if (is_spawned() && this->render_target != nullptr) {
        recalc_proj_mat();
        recalc_frustum();
    }
}

void
ge_camera_node::set_near_plane(float near_plane) {
    this->near_plane = near_plane;

    if (is_spawned() && this->render_target != nullptr) {
        recalc_proj_mat();
        recalc_frustum();
    }
}

void
ge_camera_node::set_far_plane(float far_plane) {
    this->far_plane = far_plane;

    if (is_spawned() && this->render_target != nullptr) {
        recalc_proj_mat();
        recalc_frustum();
    }
}

unsigned int
ge_camera_node::get_fov() {
    return fov;
}

float
ge_camera_node::get_near_plane() {
    return near_plane;
}

float
ge_camera_node::get_far_plane() {
    return far_plane;
}

ge_render_target*
ge_camera_node::get_render_target() {
    return render_target;
}

const ge_frustum&
ge_camera_node::get_frustum() const {
    return frustum;
}

void
ge_camera_node::on_after_spawned() {
    ge_spatial_node::on_after_spawned();

    if (render_target == nullptr) {
        return;
    }

    recalc_view_mat();
    recalc_proj_mat();
    recalc_frustum();

    register_to_render();
}

void
ge_camera_node::on_after_world_transform_changed() {
    if (render_target == nullptr || !is_spawned()) {
        return;
    }

    recalc_view_mat();
    recalc_frustum();
}

void
ge_camera_node::on_before_despawned() {
    if (render_target == nullptr) {
        return;
    }
    unregister_from_render();
    render_target = nullptr;
}

void
ge_camera_node::recalc_view_mat() {
    view_mat = glm::lookAt(get_world_location(), get_world_location() + get_world_forward(), get_world_up());
}

void
ge_camera_node::recalc_proj_mat() {
#if defined(DEBUG)
    if (render_target == nullptr) {
        ge_log_error("expected render target to be valid");
        abort();
    }
#endif

    float fov_rad = glm::radians((float)fov);
    proj_mat = glm::perspectiveFov(
        fov_rad, (float)render_target->get_width(), (float)render_target->get_height(), near_plane, far_plane);

    float proj_window_size = 2.0f; // because view space window is [-1; 1]
    float tan_half_fov = std::tan(0.5f * fov_rad);

    near_clip_plane_height = proj_window_size * near_plane * tan_half_fov;
    far_clip_plane_height = proj_window_size * far_plane * tan_half_fov;
}

void
ge_camera_node::recalc_frustum() {
#if defined(DEBUG)
    if (render_target == nullptr) {
        ge_log_error("expected render target to be valid");
        abort();
    }
#endif

    frustum = ge_frustum(
        get_world_location(), get_world_forward(), get_world_right(), get_world_up(), near_plane, far_plane, fov,
        (float)render_target->get_width() / (float)render_target->get_height());
}

void
ge_camera_node::register_to_render() {
    if (!is_spawned() || render_target == nullptr) {
        ge_log_error("expected the camera to be spawned and have a valid render target");
        abort();
    }

    if (registered) {
        return;
    }

    ge_world* world = get_world_if_spawned();
    if (world == nullptr) {
        ge_log_error("expected the camera to be spawned");
        abort();
    }

    world->get_world_manager()->get_game_instance()->get_renderer()->register_camera(this);
    registered = true;
}

void
ge_camera_node::unregister_from_render() {
    if (!is_spawned() || render_target == nullptr) {
        ge_log_error("expected the camera to be spawned and have a valid render target");
        abort();
    }

    if (!registered) {
        ge_log_error("expected the camera to be registered at this point");
        abort();
    }

    ge_world* world = get_world_if_spawned();
    if (world == nullptr) {
        ge_log_error("expected the camera to be spawned");
        abort();
    }

    world->get_world_manager()->get_game_instance()->get_renderer()->unregister_camera(this);
    registered = false;
}
