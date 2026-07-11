#pragma once

#include <shape/plane.h>
#include <shape/aabb.h>
#include <glm/mat4x4.hpp>

// Frustum shape.
class ge_frustum {
  public:
    ge_frustum() = default;
    ge_frustum(
        const glm::vec3& location, const glm::vec3& forward, const glm::vec3& right, const glm::vec3& up,
        float near_plane, float far_plane, unsigned int fov_deg, float aspect_ratio);

    // Tests if the AABB is inside of the frustum or intersects it, otherwise returns `false`.
    bool is_aabb_in_frustum(const ge_aabb& aabb_model_space, const glm::mat4& world_mat) const;

    ge_plane top_face;
    ge_plane bottom_face;
    ge_plane right_face;
    ge_plane left_face;
    ge_plane near_face;
    ge_plane far_face;
};
