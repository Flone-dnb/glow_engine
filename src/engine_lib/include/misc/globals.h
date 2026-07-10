#pragma once

#include <glm/vec3.hpp>

// Returns application name.
// Do not free/destroy returned pointer.
const char* ge_get_app_name(void);

// Returns a normalized vector that points in the world's forward direction.
glm::vec3 ge_get_world_forward();

// Returns a normalized vector that points in the world's right direction.
glm::vec3 ge_get_world_right();

// Returns a normalized vector that points in the world's up direction.
glm::vec3 ge_get_world_up();

// Pauses the thread on N milliseconds.
void ge_sleep(float ms);