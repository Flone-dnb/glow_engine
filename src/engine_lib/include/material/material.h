#pragma once

class ge_renderer;

// A user friendly bridge between a mesh and shaders.
// Used to configure shader parameters.
class ge_material {
  public:
    static ge_material* create(const char* path_to_vertex_shader, const char* path_to_pixel_shader);
    static ge_material* create_from_file(const char* path_to_material);

    ge_material() = delete;
    ge_material(const ge_material&) = delete;
    ge_material& operator=(const ge_material&) = delete;

    const char* get_path_to_vertex_shader();
    const char* get_path_to_pixel_shader();

  private:
    ge_material(const char* path_to_vertex_shader, const char* path_to_pixel_shader);

    const char* path_to_vertex_shader;
    const char* path_to_pixel_shader;
};