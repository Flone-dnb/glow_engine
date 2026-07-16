#include <render/directx/directx_mesh_renderer.h>

#include <io/log.h>
#include <render/directx/directx_renderer.h>
#include <render/directx/directx_resource.h>
#include <render/directx/directx_pso_manager.h>

ge_directx_mesh_renderer::ge_directx_mesh_renderer(ge_renderer* renderer) : ge_mesh_renderer(renderer) {}

unsigned int
ge_directx_mesh_renderer::register_mesh_for_rendering(
    ge_gpu_resource* vertex_buffer, unsigned int index_buffer_count, ge_gpu_resource** index_buffers,
    ge_material* material) {
    if (index_buffer_count > GE_MAX_INDEX_BUFFER_PER_MESH) {
        ge_log_error_fmt(
            "failed to register mesh for rendering: the mesh has %u index buffers (material slots) but the maximum "
            "supported is %i (TODO: the maximum can be increased)",
            index_buffer_count, GE_MAX_INDEX_BUFFER_PER_MESH);
        abort();
    }
    if (vertex_buffer->get_element_count() == 0 || vertex_buffer->get_element_size() == 0) {
        ge_log_error("failed to register mesh for rendering: vertex buffer element count/size is zero");
        abort();
    }

    // Fill render data.
    mesh_render_data render_data;

    render_data.vertex_buffer_view.BufferLocation =
        ((ge_directx_resource*)vertex_buffer)->get_dx_resource()->GetGPUVirtualAddress();
    render_data.vertex_buffer_view.StrideInBytes = vertex_buffer->get_element_size();
    render_data.vertex_buffer_view.SizeInBytes = vertex_buffer->get_element_count() * vertex_buffer->get_element_size();

    render_data.index_buffer_count = index_buffer_count;
    for (unsigned int i = 0; i < index_buffer_count; i++) {
        ge_directx_resource* index_buffer = (ge_directx_resource*)index_buffers[i];

        if (index_buffer->get_element_count() == 0 || index_buffer->get_element_size() == 0) {
            ge_log_error("failed to register mesh for rendering: index buffer element count/size is zero");
            abort();
        }

        render_data.index_buffer_addrs[i] = index_buffer->get_dx_resource()->GetGPUVirtualAddress();
        render_data.index_buffer_sizes[i] = index_buffer->get_element_count() * index_buffer->get_element_size();
    }

    // Get PSO.
    ge_directx_pso_manager* pso_manager = ((ge_directx_renderer*)get_renderer())->get_pso_manager();
    std::shared_ptr<ge_directx_pso> pso = pso_manager->get_pso_for_material(material);

    std::lock_guard<std::mutex> guard(mtx_data.first);
    auto& state = mtx_data.second;

    // Find a PSO group.
    unsigned int render_data_idx = 0;
    bool found = false;
    for (unsigned int i = 0; i < state.pso_groups.size(); i++) {
        pso_group* group = &state.pso_groups[i];

        render_data_idx += group->count;
        if (group->pso.get() != pso.get()) {
            continue;
        }

        group->count += 1;
        found = true;
        break;
    }

    // Add render data.
    if (found) {
        state.render_data.insert(state.render_data.begin() + render_data_idx, std::move(render_data));

        // Update handles.
        for (unsigned int i = 0; i < state.handle_to_data.size(); i++) {
            if (state.handle_to_data[i] != 0xFFFFFFFF && state.handle_to_data[i] >= render_data_idx) {
                state.handle_to_data[i] += 1;
            }
        }
    } else {
        pso_group group;
        group.count = 1;
        group.pso = std::move(pso);

        state.pso_groups.push_back(std::move(group));
        state.render_data.push_back(std::move(render_data));
    }

    // Get handle to return.
    unsigned int handle = 0xFFFFFFFF;
    for (unsigned int i = 0; i < state.handle_to_data.size(); i++) {
        if (state.handle_to_data[i] != 0xFFFFFFFF) {
            continue;
        }
        state.handle_to_data[i] = render_data_idx;
        handle = i;
        break;
    }
    if (handle == 0xFFFFFFFF) {
        const unsigned int expand_size = 256;
        if (UINT_MAX - expand_size <= state.handle_to_data.size()) {
            ge_log_error("reached handle array limit");
            abort();
        }

        handle = (unsigned int)state.handle_to_data.size();
        state.handle_to_data.resize(state.handle_to_data.size() + expand_size);
        state.handle_to_data[handle] = render_data_idx;
    }

    return handle;
}

void
ge_directx_mesh_renderer::unregister_mesh_from_rendering(unsigned int handle) {
    std::lock_guard<std::mutex> guard(mtx_data.first);
    auto& state = mtx_data.second;

    if (handle >= state.handle_to_data.size()) {
        ge_log_error("failed to unregister mesh from rendering: the specified handle is invalid");
        abort();
    }
    unsigned int render_data_idx = state.handle_to_data[handle];
    state.handle_to_data[handle] = 0xFFFFFFFF;

    state.render_data.erase(state.render_data.begin() + render_data_idx);

    // Find a group.
    unsigned int start_idx = 0;
    bool found = false;
    for (unsigned int i = 0; i < state.pso_groups.size(); i++) {
        if (render_data_idx >= start_idx && render_data_idx < start_idx + state.pso_groups[i].count) {
            found = true;

            if (state.pso_groups[i].count == 1) {
                state.pso_groups.erase(state.pso_groups.begin() + i);
            } else {
                state.pso_groups[i].count -= 1;
            }

            if (!state.pso_groups.empty()) {
                // Update handles.
                for (unsigned int i = 0; i < state.handle_to_data.size(); i++) {
                    if (state.handle_to_data[i] != 0xFFFFFFFF && state.handle_to_data[i] > render_data_idx) {
                        state.handle_to_data[i] -= 1;
                    }
                }
            }
            break;
        }
    }
    if (!found) {
        ge_log_error("failed to unregister mesh from rendering: unable to find a PSO group");
        abort();
    }
}