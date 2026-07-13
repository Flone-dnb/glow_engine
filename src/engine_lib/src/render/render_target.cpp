#include <render/render_target.h>

#if defined(WIN32)
#include <render/directx/directx_render_target.h>
#endif

ge_render_target*
ge_render_target::create(ge_renderer* renderer, unsigned int width, unsigned int height) {
#if defined(WIN32)
    return new ge_directx_render_target(renderer, width, height);
#else
    return new ge_render_target(width, height);
#endif
}

ge_render_target::ge_render_target(unsigned int width, unsigned int height) {
    this->width = width;
    this->height = height;
}

unsigned int
ge_render_target::get_width() {
    return width;
}

unsigned int
ge_render_target::get_height() {
    return height;
}

void
ge_render_target::get_clear_color(float clear[4]) {
    clear = clear_color;
}