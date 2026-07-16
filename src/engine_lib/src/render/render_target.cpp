#include <render/render_target.h>

#if defined(WIN32)
#include <render/directx/directx_render_target.h>
#endif

ge_render_target*
ge_render_target::create(ge_renderer* renderer, unsigned int width, unsigned int height, const glm::vec4& clear_color) {
#if defined(WIN32)
    return new ge_directx_render_target(renderer, width, height, clear_color);
#else
    return new ge_render_target(width, height, clear_color);
#endif
}

ge_render_target::ge_render_target(unsigned int width, unsigned int height, const glm::vec4& clear_color) {
    this->width = width;
    this->height = height;
    this->clear_color = clear_color;
}

unsigned int
ge_render_target::get_width() {
    return width;
}

unsigned int
ge_render_target::get_height() {
    return height;
}

glm::vec4
ge_render_target::get_clear_color() {
    return clear_color;
}