#include <node/mesh_node.h>

#include <render/gpu_resource.h>
#include <render/renderer.h>
#include <render/mesh_renderer.h>
#include <geometry/mesh_generator.h>
#include <material/material.h>
#include <io/log.h>

ge_mesh_node::ge_mesh_node() : ge_mesh_node("Mesh Node") {}

ge_mesh_node::ge_mesh_node(const char* name) : ge_spatial_node(name) {
    vertex_buffer = nullptr;
    visible = true;
    registered_render_id = 0xFFFFFFFF;
    normal_mat = glm::identity<glm::mat4>();
    material = ge_material::create("engine/shader/mesh.vert.hlsl", "engine/shader/mesh.pix.hlsl");
}

ge_mesh_node::~ge_mesh_node() { delete material; }

void
ge_mesh_node::set_material_own(ge_material* material) {
    bool rendered = registered_render_id != 0xFFFFFFFF;
    if (rendered) {
        remove_from_rendering();
    }

    ge_material* old_mat = this->material;
    this->material = material;
    delete old_mat;

    if (rendered) {
        add_to_rendering();
    }
}

void
ge_mesh_node::set_is_visible(bool visible) {
    if (this->visible == visible) {
        return;
    }
    this->visible = visible;

    if (registered_render_id != 0xFFFFFFFF) {
        remove_from_rendering();
    }

    if (this->visible) {
        add_to_rendering();
    }
}

ge_material*
ge_mesh_node::get_material() {
    return material;
}

bool
ge_mesh_node::is_visible() {
    return visible;
}

void
ge_mesh_node::on_after_spawned() {
    ge_spatial_node::on_after_spawned();

    if (visible) {
        alloc_buffers();
        add_to_rendering();
    }
}

void
ge_mesh_node::on_before_despawned() {
    ge_spatial_node::on_before_despawned();

    if (registered_render_id != 0xFFFFFFFF) {
        remove_from_rendering();
        dealloc_buffers();
    }
}

void
ge_mesh_node::on_after_world_transform_changed() {
    if (registered_render_id == 0xFFFFFFFF) {
        return;
    }

    recalc_normal_mat();
    // TODO: update on render
}

void
ge_mesh_node::add_to_rendering() {
    ge_renderer* renderer = get_renderer_if_spawned();
    if (renderer == nullptr) {
        ge_log_error("expected to be spawned");
        abort();
    }

    registered_render_id = renderer->get_mesh_renderer()->register_mesh_for_rendering(
        vertex_buffer, (unsigned int)index_buffers.size(), index_buffers.data(), material);
}

void
ge_mesh_node::remove_from_rendering() {
    ge_renderer* renderer = get_renderer_if_spawned();
    if (renderer == nullptr) {
        ge_log_error("expected to be spawned");
        abort();
    }

    renderer->get_mesh_renderer()->unregister_mesh_from_rendering(registered_render_id);
    registered_render_id = 0xFFFFFFFF;
}

void
ge_mesh_node::recalc_normal_mat() {
    normal_mat = glm::transpose(glm::inverse(get_world_mat()));
}

void
ge_mesh_node::alloc_buffers() {
    ge_renderer* renderer = get_renderer_if_spawned();
    if (renderer == nullptr) {
        ge_log_error("expected to be spawned");
        abort();
    }

    ge_vertex_pack* vertices = nullptr;
    std::vector<std::vector<ge_mesh_index>> indices_per_slots;

    if (path_to_geo.empty()) {
        indices_per_slots.resize(1);
        ge_mesh_generator_cube(vertices, indices_per_slots[0]);
    } else {
        // TODO: load geometry
        ge_log_error("not implemented");
        abort();
    }

    index_buffers.resize(indices_per_slots.size());

    vertex_buffer = ge_gpu_resource::create_resource_with_data(
        renderer, vertices->vertex_sizeof, vertices->vertex_count, vertices->data, GE_GRU_VERTEX_BUFFER);
    for (unsigned int i = 0; i < index_buffers.size(); i++) {
        index_buffers[i] = ge_gpu_resource::create_resource_with_data(
            renderer, sizeof(ge_mesh_index), (unsigned int)indices_per_slots[i].size(), indices_per_slots[i].data(),
            GE_GRU_INDEX_BUFFER);
    }

    delete vertices;
}

void
ge_mesh_node::dealloc_buffers() {
    delete vertex_buffer;
    for (ge_gpu_resource*& res : index_buffers) {
        delete res;
    }
    index_buffers.clear();
    index_buffers.shrink_to_fit();
}