#pragma once

#include <vector>
#include <geometry/vertex_pack.h>

// Generates a cube.
void ge_mesh_generator_cube(
    ge_vertex_pack*& out_vertices, std::vector<ge_mesh_index>& out_indices, float x_size = 1.0f, float y_size = 1.0f,
    float z_size = 1.0f);