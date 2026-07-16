#include <geometry/vertex_pack.h>

#include <stdlib.h>
#include <string>
#include <io/log.h>

ge_vertex_pack*
ge_vertex_pack::create(ge_vertex_format format, unsigned int vertex_count) {
    ge_vertex_pack* pack = new ge_vertex_pack();
    memset(pack->attribute_offsets, 255, sizeof(pack->attribute_offsets[0]) * GE_VA_COUNT);
    pack->vertex_count = vertex_count;

    switch (format) {
        case (GE_VF_POS_NORMAL_UV): {
            unsigned char offset = 0;

            pack->attribute_offsets[GE_VA_POSITION] = offset;
            offset += sizeof(float) * 3;

            pack->attribute_offsets[GE_VA_NORMAL] = offset;
            offset += sizeof(float) * 3;

            pack->attribute_offsets[GE_VA_UV] = offset;
            offset += sizeof(float) * 2;

            pack->vertex_sizeof = offset;

            break;
        }
        default: {
            ge_log_error("unhandled case");
            abort();
            break;
        }
    }

    return pack;
}

ge_vertex_pack::~ge_vertex_pack() { delete data; }
