#pragma once

#include <node/spatial_node.h>
#include <shape/frustum.h>

class ge_render_target;

// Implements camera functionality to render game world to some render target
// (which can be displayed on a window).
class ge_camera_node : public ge_spatial_node {
  public:
    ge_camera_node();
    ge_camera_node(const char* name);

    ge_camera_node(const ge_camera_node&) = delete;
    ge_camera_node& operator=(const ge_camera_node&) = delete;

    // Makes the camera render to the specified render target when the camera is spawned.
    // This function can also be used to set `nullptr` as render target to disable rendering.
    void set_render_target(ge_render_target* render_target);

    // Sets vertical field of view (in degrees).
    void set_fov(unsigned int fov);
    unsigned int get_fov();

    // Sets distance from view space origin to camera's near/far clipping plane.
    void set_near_plane(float near_plane);
    void set_far_plane(float far_plane);
    float get_near_plane();
    float get_far_plane();

    // Returns `nullptr` if no render target set.
    ge_render_target* get_render_target();

    // Returns camera's frustum, valid while spawned.
    const ge_frustum& get_frustum() const;

  protected:
    virtual void on_after_spawned() override;
    virtual void on_after_world_transform_changed() override;
    virtual void on_before_despawned() override;

  private:
    void recalc_view_mat();
    void recalc_proj_mat();
    void recalc_frustum();

    void register_to_render();
    void unregister_from_render();

    glm::mat4 view_mat;
    glm::mat4 proj_mat;

    ge_frustum frustum;

    // `nullptr` if the camera is not drawing anything (being inactive).
    // Do not delete this pointer, reference to some render target.
    ge_render_target* render_target;

    // Vertical field of view in degrees.
    unsigned int fov;

    // Distance from view space origin to camera's near/far clipping plane.
    float near_plane;
    float far_plane;

    // Height of camera's near/far clip plane.
    float near_clip_plane_height;
    float far_clip_plane_height;

    bool registered;
};