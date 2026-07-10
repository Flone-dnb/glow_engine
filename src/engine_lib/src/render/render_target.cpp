#include <render/render_target.h>

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