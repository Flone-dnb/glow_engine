#pragma once

#include <glm/vec3.hpp>

// Plane shape.
class ge_plane {
  public:
    ge_plane() = default;

    // Creates a new plane with normal and location.
    ge_plane(const glm::vec3& normal, const glm::vec3& location);

    // Tells if the point is fully behind (inside of the negative halfspace of) a plane.
    bool is_point_behind_plane(const glm::vec3& point) const;

    // Plane's normal.
    glm::vec3 normal;

    // Distance from the origin to the nearest point on the plane.
    float distance;
};