#pragma once

#include <shape/plane.h>

// Axis-aligned bounding box shape.
class ge_aabb {
  public:
    ge_aabb() = default;

    // Tells if the AABB is fully behind (inside the negative halfspace of) a plane.
    bool is_behind_plane(const ge_plane& plane) const;

    glm::vec3 center;

    // Half extension (half size).
    glm::vec3 extents;
};