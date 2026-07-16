#pragma once

#include <vector>
#include <node/spatial_node.h>

class ge_gpu_resource;
class ge_material;

// 3D triangle mesh.
class ge_mesh_node : public ge_spatial_node {
  public:
    ge_mesh_node();
    ge_mesh_node(const char* name);

    virtual ~ge_mesh_node() override;

    ge_mesh_node(const ge_mesh_node&) = delete;
    ge_mesh_node& operator=(const ge_mesh_node&) = delete;

    // Mesh node takes the ownership of the material.
    void set_material_own(ge_material* material);

    void set_is_visible(bool visible);

    // Do not delete returned pointer, valid while the mesh exists.
    ge_material* get_material();

    bool is_visible();

  protected:
    virtual void on_after_spawned() override;
    virtual void on_before_despawned() override;
    virtual void on_after_world_transform_changed() override;

  private:
    void alloc_buffers();
    void dealloc_buffers();

    void add_to_rendering();
    void remove_from_rendering();

    void recalc_normal_mat();

    ge_gpu_resource* vertex_buffer;
    std::vector<ge_gpu_resource*> index_buffers;

    // Matrix to convert normals to world space.
    glm::mat3 normal_mat;

    // not 0xFFFFFFFF if registered for rendering
    unsigned int registered_render_id;

    // Path to the mesh geometry file. Can be empty to create a default model on spawn.
    std::string path_to_geo;

    // Always valid pointer.
    ge_material* material;

    bool visible;
};