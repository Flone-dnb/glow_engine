#include <shape/plane.h>

#include <glm/geometric.hpp>

ge_plane::ge_plane(const glm::vec3& normal, const glm::vec3& location) {
    this->normal = normal;
    distance = glm::dot(location, normal);
}

bool
ge_plane::is_point_behind_plane(const glm::vec3& point) const {
    // Source: Real-time collision detection, Christer Ericson (2005).
    return glm::dot(point, normal) - distance < 0.0f;
}
