#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <render/mesh_renderer.h>
#include <directx/d3dx12.h>

class ge_directx_pso;

// you can increase this value by 1 or maybe 2 if needed, see the usage
#define GE_MAX_INDEX_BUFFER_PER_MESH 2

class ge_directx_mesh_renderer : public ge_mesh_renderer {
  public:
    ge_directx_mesh_renderer(ge_renderer* renderer);
    virtual ~ge_directx_mesh_renderer() override = default;

    ge_directx_mesh_renderer() = delete;
    ge_directx_mesh_renderer(const ge_directx_mesh_renderer&) = delete;
    ge_directx_mesh_renderer& operator=(const ge_directx_mesh_renderer&) = delete;

    virtual unsigned int register_mesh_for_rendering(
        ge_gpu_resource* vertex_buffer, unsigned int index_buffer_count, ge_gpu_resource** index_buffers,
        ge_material* material) override;
    virtual void unregister_mesh_from_rendering(unsigned int handle) override;

  private:
    // Information about different meshes using the same PSO.
    struct pso_group {
        std::shared_ptr<ge_directx_pso> pso;

        // how much meshes to render
        unsigned int count;
    };

    struct mesh_render_data {
        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

        unsigned int index_buffer_count;
        D3D12_GPU_VIRTUAL_ADDRESS index_buffer_addrs[GE_MAX_INDEX_BUFFER_PER_MESH];
        UINT index_buffer_sizes[GE_MAX_INDEX_BUFFER_PER_MESH];
    };

    struct guarded_data {
        // Stores meshes sorted by PSO so first N meshes use the same PSO (item 0 from @ref pso_groups)
        // then next M meshes use another PSO (item 1 from @ref pso_groups) and so on.
        std::vector<mesh_render_data> render_data;

        std::vector<pso_group> pso_groups;

        // Stores indices that public API users use.
        // This array can have "holes" (invalid items, 0xFFFFFFFF) and does not shrink.
        std::vector<unsigned int> handle_to_data;
    };

    std::pair<std::mutex, guarded_data> mtx_data;
};