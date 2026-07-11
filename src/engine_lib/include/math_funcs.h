#pragma once

#include <stdbool.h>
#include <io/log.h>
#include <misc/globals.h>
#include <cmath>
#include <glm/gtx/vector_angle.hpp>

static inline void
ge_math_fix_diagonal_movement_speedup(glm::vec2& movement) {
    const float square_sum = movement.x * movement.x + movement.y * movement.y;
    if (square_sum < 0.1f) { // don't normalize if vector is zero or very small to avoid NaNs
        return;
    }

    const float length = sqrtf(square_sum);
    if (length <= 1.0f) { // only normalize when exceeding 1 to keep small gamepad thumbstick movements
        return;
    }

    // normalize
    movement.x /= length;
    movement.y /= length;
}

static inline void
ge_math_normalize_safely(glm::vec3& vec) {
    const float square_sum = glm::dot(vec, vec);
    if (square_sum < 0.0001f) {
        vec.x = 0.0f;
        vec.y = 0.0f;
        vec.z = 0.0f;
        return;
    }

    const float inv_sqrt = 1.0f / sqrtf(square_sum);
    vec *= inv_sqrt;
}

static inline glm::vec3
ge_math_convert_norm_dir_to_rot(const glm::vec3& dir) {
    if (glm::all(glm::epsilonEqual(dir, glm::vec3(0.0f, 0.0f, 0.0f), 0.0001f))) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

#if defined(DEBUG)
    // Make sure we are given a normalized direction.
    if (!glm::epsilonEqual(glm::length(dir), 1.0f, 0.0001f)) {
        ge_log_error("the specified direction vector should have been normalized");
        abort();
    }
#endif

    glm::vec3 out;
    out.x = glm::degrees(asinf(-dir[1]));
    out.y = glm::degrees(atan2f(dir[0], dir[2]));
    out.z = 0.0f;

    if (std::isnan(out.x)) {
        out.x = 0.0f;
    }
    if (std::isnan(out.y)) {
        out.y = 0.0f;
    }

    return out;
}

// Creates a new rotation matrix from a rotation (in degrees).
static inline glm::mat4
ge_math_make_rotation_mat(const glm::vec3& rotation_deg) {
    return glm::rotate(glm::radians(rotation_deg.z), glm::vec3(0.0f, 0.0f, 1.0f))
           * glm::rotate(glm::radians(rotation_deg.y), glm::vec3(0.0f, 1.0f, 0.0f))
           * glm::rotate(glm::radians(rotation_deg.x), glm::vec3(1.0f, 0.0f, 0.0f));
}

// Converts rotation in degrees to a normalized direction vector.
static inline glm::vec3
ge_math_convert_rot_to_norm_dir(const glm::vec3& rot_deg) {
    glm::mat4 rot_mat = ge_math_make_rotation_mat(rot_deg);

    glm::vec4 forward = glm::vec4(ge_get_world_forward(), 0.0f);

    glm::vec4 result = rot_mat * forward;
    return result;
}

// Calculates 1 / vector while checking for zero division.
static inline glm::vec3
ge_math_calc_reciprocal(const glm::vec3& vec) {
    glm::vec3 out;

    if (std::abs(vec.x) < 0.0001f) {
        out.x = 0.0f;
    } else {
        out.x = 1.0f / vec.x;
    }

    if (std::abs(vec.y) < 0.0001f) {
        out.y = 0.0f;
    } else {
        out.y = 1.0f / vec.y;
    }

    if (std::abs(vec.z) < 0.0001f) {
        out.z = 0.0f;
    } else {
        out.z = 1.0f / vec.z;
    }

    return out;
}

// Wraps the value to be in the range [min; max], examples:
// (370.0, -360.0, 360.0): -350
// (-730, -360, 360): -10
static inline float
ge_math_wrap_to_range(float value, float min, float max) {
    float width = max - min;
    float offsetValue = value - min;
    return (offsetValue - (floorf(offsetValue / width) * width)) + min;
}
