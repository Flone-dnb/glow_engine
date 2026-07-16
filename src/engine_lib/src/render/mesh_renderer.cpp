#include <render/mesh_renderer.h>

#if defined(WIN32)
#include <render/directx/directx_mesh_renderer.h>
#else
#include <render/dummy_mesh_renderer.h>
#endif

ge_mesh_renderer*
ge_mesh_renderer::create(ge_renderer* renderer) {
#if defined(WIN32)
    return new ge_directx_mesh_renderer(renderer);
#else
    return new ge_dummy_mesh_renderer(renderer);
#endif
}

ge_mesh_renderer::ge_mesh_renderer(ge_renderer* renderer) { this->renderer = renderer; }

ge_renderer*
ge_mesh_renderer::get_renderer() {
    return renderer;
}