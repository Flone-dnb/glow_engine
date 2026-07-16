#pragma once

#include <render/mesh_renderer.h>

// Renderer that does nothing (i.e. no rendering).
class ge_dummy_mesh_renderer : public ge_mesh_renderer {
  public:
    ge_dummy_mesh_renderer(ge_renderer* renderer) : ge_mesh_renderer(renderer) {}
    virtual ~ge_dummy_mesh_renderer() = default;

    ge_dummy_mesh_renderer() = delete;
    ge_dummy_mesh_renderer(const ge_dummy_mesh_renderer&) = delete;
    ge_dummy_mesh_renderer& operator=(const ge_dummy_mesh_renderer&) = delete;

    virtual unsigned int
    register_mesh_for_rendering(
        ge_gpu_resource* vertex_buffer, unsigned int index_buffer_count, ge_gpu_resource** index_buffers,
        ge_material* material) override {
        auto out = id;
        id += 1;
        return out;
    }
    virtual void
    unregister_mesh_from_rendering(unsigned int id) override {}

  private:
    unsigned int id = 0;
};