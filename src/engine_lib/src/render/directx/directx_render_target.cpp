#include "directx_render_target.h"

#include <render/directx/directx_resource.h>
#include <render/texture_format.h>

ge_directx_render_target::ge_directx_render_target(ge_renderer* renderer, unsigned int width, unsigned int height)
    : ge_render_target(width, height) {
    float clear[4];
    get_clear_color(clear);

    resource = ge_directx_resource::create_render_target(
        (ge_directx_renderer*)renderer, GE_TF_R8G8B8A8_UNORM, width, height, clear);
}

ge_directx_render_target::~ge_directx_render_target() { delete resource; }

ge_directx_resource*
ge_directx_render_target::get_resource() {
    return resource;
}
