#include <geometry/mesh_generator.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

void
ge_mesh_generator_cube(
    ge_vertex_pack*& out_vertices, std::vector<ge_mesh_index>& out_indices, float x_size, float y_size, float z_size) {
    out_vertices = ge_vertex_pack::create(GE_VF_POS_NORMAL_UV, 24);
    const unsigned int vert_size = out_vertices->vertex_sizeof;

    float x = x_size / 2.0f;
    float y = y_size / 2.0f;
    float z = z_size / 2.0f;

    // Init UVs.
    unsigned char off = out_vertices->attribute_offsets[GE_VA_UV];
    for (unsigned int i = 0; i < out_vertices->vertex_count; i += 4) {
        (*(glm::vec2*)&out_vertices->data[vert_size * (i + 0) + off]) = glm::vec2(1.0f, 1.0f);
        (*(glm::vec2*)&out_vertices->data[vert_size * (i + 1) + off]) = glm::vec2(0.0f, 1.0f);
        (*(glm::vec2*)&out_vertices->data[vert_size * (i + 2) + off]) = glm::vec2(1.0f, 0.0f);
        (*(glm::vec2*)&out_vertices->data[vert_size * (i + 3) + off]) = glm::vec2(0.0f, 0.0f);
    }

    // Init normals.
    off = out_vertices->attribute_offsets[GE_VA_NORMAL];
    unsigned int normal_i = 0;
    for (unsigned int i = normal_i; normal_i < i + 4; normal_i++) {
        (*(glm::vec3*)&out_vertices->data[vert_size * normal_i + off]) = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    for (unsigned int i = normal_i; normal_i < i + 4; normal_i++) {
        (*(glm::vec3*)&out_vertices->data[vert_size * normal_i + off]) = glm::vec3(-1.0f, 0.0f, 0.0f);
    }
    for (unsigned int i = normal_i; normal_i < i + 4; normal_i++) {
        (*(glm::vec3*)&out_vertices->data[vert_size * normal_i + off]) = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    for (unsigned int i = normal_i; normal_i < i + 4; normal_i++) {
        (*(glm::vec3*)&out_vertices->data[vert_size * normal_i + off]) = glm::vec3(0.0f, -1.0f, 0.0f);
    }
    for (unsigned int i = normal_i; normal_i < i + 4; normal_i++) {
        (*(glm::vec3*)&out_vertices->data[vert_size * normal_i + off]) = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    for (unsigned int i = normal_i; normal_i < i + 4; normal_i++) {
        (*(glm::vec3*)&out_vertices->data[vert_size * normal_i + off]) = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    // Init positions.
    off = out_vertices->attribute_offsets[GE_VA_POSITION];
    unsigned int i = 0;

    // +X face.
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, -y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, -y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, y, z);
    i += 1;

    // -X face.
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, -y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, -y, z);
    i += 1;

    // +Y face.
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, y, z);
    i += 1;

    // -Y face.
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, -y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, -y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, -y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, -y, z);
    i += 1;

    // +Z face.
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, -y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, -y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, y, z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, y, z);
    i += 1;

    // -Z face.
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(-x, -y, -z);
    i += 1;
    (*(glm::vec3*)&out_vertices->data[vert_size * i + off]) = glm::vec3(x, -y, -z);
    i += 1;

    out_indices.resize(36);
    out_indices[0] = 0; // +X face.
    out_indices[1] = 1;
    out_indices[2] = 2;
    out_indices[3] = 3;
    out_indices[4] = 2;
    out_indices[5] = 1;
    out_indices[6] = 4; // -X face.
    out_indices[7] = 5;
    out_indices[8] = 6;
    out_indices[9] = 7;
    out_indices[10] = 6;
    out_indices[11] = 5;
    out_indices[12] = 8; // +Y face.
    out_indices[13] = 9;
    out_indices[14] = 10;
    out_indices[15] = 11;
    out_indices[16] = 10;
    out_indices[17] = 9;
    out_indices[18] = 12; // -Y face.
    out_indices[19] = 13;
    out_indices[20] = 14;
    out_indices[21] = 15;
    out_indices[22] = 14;
    out_indices[23] = 13;
    out_indices[24] = 16; // +Z face.
    out_indices[25] = 17;
    out_indices[26] = 18;
    out_indices[27] = 19;
    out_indices[28] = 18;
    out_indices[29] = 17;
    out_indices[30] = 20; // -Z face.
    out_indices[31] = 21;
    out_indices[32] = 22;
    out_indices[33] = 23;
    out_indices[34] = 22;
    out_indices[35] = 21;
}
