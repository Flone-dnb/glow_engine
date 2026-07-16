#include <render/gpu_resource.h>

#include <render/directx/directx_resource.h>

ge_gpu_resource*
ge_gpu_resource::create_render_target(
    ge_renderer* renderer, ge_texture_format format, unsigned int width, unsigned int height,
    const glm::vec4& clear_color) {
#if defined(WIN32)
    return ge_directx_resource::create_render_target(
        (ge_directx_renderer*)renderer, format, width, height, clear_color);
#else
    return new ge_gpu_resource(renderer, 0, 0);
#endif
}

ge_gpu_resource*
ge_gpu_resource::create_resource_with_data(
    ge_renderer* renderer, unsigned int element_size, unsigned int element_count, void* data,
    ge_gpu_resource_usage usage) {
#if defined(WIN32)
    return ge_directx_resource::create_resource_with_data(
        (ge_directx_renderer*)renderer, element_size, element_count, data, usage);
#else
    return new ge_gpu_resource(renderer, 0, 0);
#endif
}

unsigned int
ge_gpu_resource::get_element_size() {
    return element_size;
}

unsigned int
ge_gpu_resource::get_element_count() {
    return element_count;
}

ge_renderer*
ge_gpu_resource::get_renderer() {
    return renderer;
}

ge_gpu_resource::ge_gpu_resource(ge_renderer* renderer, unsigned int element_size, unsigned int element_count) {
    this->renderer = renderer;
    this->element_count = element_count;
    this->element_size = element_size;
}
