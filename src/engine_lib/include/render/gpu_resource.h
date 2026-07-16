#pragma once

#include <glm/vec4.hpp>
#include <render/texture_format.h>

class ge_renderer;

enum ge_gpu_resource_usage {
    GE_GRU_VERTEX_BUFFER,
    GE_GRU_INDEX_BUFFER,
    GE_GRU_ARRAY_BUFFER, //< (RW)StructuredBuffer
    OTHER
};

// Base class for GPU resources.
class ge_gpu_resource {
  public:
    virtual ~ge_gpu_resource() = default;

    ge_gpu_resource() = delete;
    ge_gpu_resource(const ge_gpu_resource&) = delete;
    ge_gpu_resource& operator=(const ge_gpu_resource&) = delete;

    // Create a new render target texture.
    static ge_gpu_resource* create_render_target(
        ge_renderer* renderer, ge_texture_format format, unsigned int width, unsigned int height,
        const glm::vec4& clear_color);

    static ge_gpu_resource* create_resource_with_data(
        ge_renderer* renderer, unsigned int element_size, unsigned int element_count, void* data,
        ge_gpu_resource_usage usage);

    // Returns size (in bytes) of one array elements (if array), otherwise size of the whole resource.
    // Returned value may be 0 in some cases (for ex. for textures).
    unsigned int get_element_size();

    // Total number of elements in the array (if array), otherwise 1.
    // Returned value may be 0 in some cases (for ex. for textures).
    unsigned int get_element_count();

    ge_renderer* get_renderer();

  protected:
    ge_gpu_resource(ge_renderer* renderer, unsigned int element_size, unsigned int element_count);

  private:
    ge_renderer* renderer;

    // may be zero for things like textures
    unsigned int element_size;
    unsigned int element_count;
};