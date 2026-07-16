#pragma once

class ge_renderer;
class ge_gpu_resource;
class ge_material;

// Base class for mesh renderer.
class ge_mesh_renderer {
    // Only main renderer is supposed to create mesh renderer.
    friend class ge_renderer;

  public:
    virtual ~ge_mesh_renderer() = default;

    ge_mesh_renderer() = delete;
    ge_mesh_renderer(const ge_mesh_renderer&) = delete;
    ge_mesh_renderer& operator=(const ge_mesh_renderer&) = delete;

    // Registers mesh to be rendered. The caller guarantees that vertex buffer, index buffers and the material pointers
    // will not change / will be valid until unregistered.
    // Returns a unique ID
    virtual unsigned int register_mesh_for_rendering(
        ge_gpu_resource* vertex_buffer, unsigned int index_buffer_count, ge_gpu_resource** index_buffers,
        ge_material* material) = 0;
    virtual void unregister_mesh_from_rendering(unsigned int id) = 0;

    ge_renderer* get_renderer();

  protected:
    // Create a specific type of mesh renderer.
    static ge_mesh_renderer* create(ge_renderer* renderer);

    // Used internally.
    ge_mesh_renderer(ge_renderer* renderer);

  private:
    ge_renderer* renderer;
};