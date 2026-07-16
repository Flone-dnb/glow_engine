#pragma once

// Type for mesh indices.
typedef unsigned short ge_mesh_index;

enum ge_vertex_format {
    GE_VF_POS_NORMAL_UV,
};

// Components of a mesh vertex.
enum ge_vertex_attribute {
    GE_VA_POSITION = 0,
    GE_VA_NORMAL,
    GE_VA_UV,
    // ... new attributes go here ...

    GE_VA_COUNT, // <- marks the number of elements
};

// CPU vertex buffer.
struct ge_vertex_pack {
    // Allocates vertex data for filling.
    static ge_vertex_pack* create(ge_vertex_format format, unsigned int vertex_count);

    // Deletes @ref data.
    ~ge_vertex_pack();

    // Actual vertex data.
    unsigned char* data;

    unsigned int vertex_count;
    unsigned int vertex_sizeof;

    // For each attribute stores an offset (in bytes) from the vertex start position.
    // Stores 255 if no such attribute.
    unsigned char attribute_offsets[GE_VA_COUNT];
};
