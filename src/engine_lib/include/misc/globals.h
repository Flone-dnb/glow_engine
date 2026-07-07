#pragma once

// Returns application name.
// Do not free/destroy returned pointer.
const char* ge_get_app_name(void);

// Returns a normalized vector that points in the world's forward direction.
void ge_get_world_forward(float out[3]);

// Returns a normalized vector that points in the world's right direction.
void ge_get_world_right(float out[3]);

// Returns a normalized vector that points in the world's up direction.
void ge_get_world_up(float out[3]);

// Pauses the thread on N milliseconds.
void ge_sleep(float ms);