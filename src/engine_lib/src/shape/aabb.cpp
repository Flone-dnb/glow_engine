#include <shape/aabb.h>

#include <glm/geometric.hpp>
#include <glm/common.hpp>

bool
ge_aabb::is_behind_plane(const ge_plane& plane) const {
    // Source: https://github.com/gdbooks/3DCollisions/blob/master/Chapter2/static_aabb_plane.md
    float proj_radius = glm::dot(extents, glm::abs(plane.normal));
    float distance = glm::dot(plane.normal, center) - plane.distance;
    return !(-proj_radius <= distance);
}
