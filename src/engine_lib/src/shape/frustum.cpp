#include <shape/frustum.h>

#include <misc/globals.h>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

ge_frustum::ge_frustum(
    const glm::vec3& location, const glm::vec3& forward, const glm::vec3& right, const glm::vec3& up, float near_plane,
    float far_plane, float fov_deg, float aspect_ratio) {
    float tan_half_fov = std::tan(0.5f * glm::radians(fov_deg));
    float far_half_height = far_plane * tan_half_fov;
    float far_half_width = far_half_height * aspect_ratio;

    glm::vec3 to_far = far_plane * forward;

    near_face = ge_plane(forward, location + near_plane * forward);
    far_face = ge_plane(-forward, location + to_far);
    right_face = ge_plane(glm::normalize(glm::cross(to_far + right * far_half_width, up)), location);
    left_face = ge_plane(glm::normalize(glm::cross(up, to_far - right * far_half_width)), location);
    top_face = ge_plane(glm::normalize(glm::cross(right, to_far + up * far_half_height)), location);
    bottom_face = ge_plane(glm::normalize(glm::cross(to_far - up * far_half_height, right)), location);
}

bool
ge_frustum::is_aabb_in_frustum(const ge_aabb& aabb_model_space, const glm::mat4& world_mat) const {
    // Before comparing frustum faces against AABB we need to take care of something:
    // we can't just transform AABB to world space (using world matrix) as this would result
    // in OBB (oriented bounding box) because of rotation in world matrix while we need an AABB.

    // Prepare an AABB that stores OBB (in world space) converted to AABB (in world space).
    ge_aabb aabb;
    aabb.center = world_mat * glm::vec4(aabb_model_space.center, 1.0f);

    // Calculate OBB directions in world space
    // (directions are considered to point from OBB's center).
    const glm::vec3 obbScaledforward = world_mat * glm::vec4(ge_get_world_forward(), 0.0f) * aabb_model_space.extents.x;
    const glm::vec3 obbScaledRight = world_mat * glm::vec4(ge_get_world_right(), 0.0f) * aabb_model_space.extents.y;
    const glm::vec3 obbScaledUp = world_mat * glm::vec4(ge_get_world_up(), 0.0f) * aabb_model_space.extents.z;

    // If the specified world matrix contained a rotation OBB's directions are no longer aligned
    // with world axes. We need to adjust these OBB directions to be world axis aligned and save them
    // as resulting AABB extents.

    // We can convert scaled OBB directions to AABB extents (directions) by projecting each OBB direction
    // onto world axis.

    // Calculate X extent.
    aabb.extents.x = std::abs(glm::dot(obbScaledforward, glm::vec3(1.0f, 0.0f, 0.0f))) + // project OBB X on world X
                     std::abs(glm::dot(obbScaledRight, glm::vec3(1.0f, 0.0f, 0.0f))) +   // project OBB Y on world X
                     std::abs(glm::dot(obbScaledUp, glm::vec3(1.0f, 0.0f, 0.0f)));       // project OBB Z on world X

    // Calculate Y extent.
    aabb.extents.y = std::abs(glm::dot(obbScaledforward, glm::vec3(0.0f, 1.0f, 0.0f))) + // project OBB X on world Y
                     std::abs(glm::dot(obbScaledRight, glm::vec3(0.0f, 1.0f, 0.0f))) +   // project OBB Y on world Y
                     std::abs(glm::dot(obbScaledUp, glm::vec3(0.0f, 1.0f, 0.0f)));       // project OBB Z on world Y

    // Calculate Z extent.
    aabb.extents.z = std::abs(glm::dot(obbScaledforward, glm::vec3(0.0f, 0.0f, 1.0f))) + // project OBB X on world Z
                     std::abs(glm::dot(obbScaledRight, glm::vec3(0.0f, 0.0f, 1.0f))) +   // project OBB Y on world Z
                     std::abs(glm::dot(obbScaledUp, glm::vec3(0.0f, 0.0f, 1.0f)));       // project OBB Z on world Z

    // Test each AABB face against the frustum.
    return !aabb.is_behind_plane(left_face) && !aabb.is_behind_plane(right_face) && !aabb.is_behind_plane(top_face)
           && !aabb.is_behind_plane(bottom_face) && !aabb.is_behind_plane(near_face) && !aabb.is_behind_plane(far_face);
}
