#include <material/material.h>

#include <stdlib.h>
#include <io/log.h>

ge_material*
ge_material::create(const char* path_to_vertex_shader, const char* path_to_pixel_shader) {
    return new ge_material(path_to_vertex_shader, path_to_pixel_shader);
}

ge_material*
ge_material::create_from_file(const char* path_to_material) {
    ge_log_error("TODO: not implemented");
    abort();
}

const char*
ge_material::get_path_to_vertex_shader() {
    return path_to_vertex_shader;
}

const char*
ge_material::get_path_to_pixel_shader() {
    return path_to_pixel_shader;
}

ge_material::ge_material(const char* path_to_vertex_shader, const char* path_to_pixel_shader) {
    this->path_to_vertex_shader = path_to_vertex_shader;
    this->path_to_pixel_shader = path_to_pixel_shader;
}