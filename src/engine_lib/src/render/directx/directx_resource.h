#pragma once

#include <array>
#include <render/directx/directx_descriptor_heap.h>
#include <render/texture_format.h>
#include <directx/d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class ge_directx_renderer;

// GPU resource.
class ge_directx_resource {
    // Assigns descriptors.
    friend class ge_directx_descriptor_heap;

  public:
    // Create a new render target texture.
    static ge_directx_resource* create_render_target(
        ge_directx_renderer* renderer, ge_texture_format format, unsigned int width, unsigned int height,
        float clear_color[4]);

    ~ge_directx_resource();

    ge_directx_resource(const ge_directx_resource&) = delete;
    ge_directx_resource& operator=(const ge_directx_resource&) = delete;

    // Returns `(size_t)-1` if no such descriptor bound.
    D3D12_CPU_DESCRIPTOR_HANDLE get_bound_descriptor_cpu_handle(ge_directx_descriptor_type descriptor_type);

    ID3D12Resource* get_dx_resource();

    // Returns size (in bytes) of one array elements (if array), otherwise size of the whole resource.
    // Returned value may be 0 in some cases (for ex. for textures).
    unsigned int get_element_size();

    // Total number of elements in the array (if array), otherwise 1.
    // Returned value may be 0 in some cases (for ex. for textures).
    unsigned int get_element_count();

  private:
    struct descriptors_info {
        // `nullptr` if not bound.
        ge_directx_descriptor* descriptor;

        // `nullptr` if not bound, otherwise if the resource is a cubemap each descriptor references a specific cubemap face.
        std::array<ge_directx_descriptor*, 6> cubemap_faces;
    };

    ge_directx_resource() = delete;
    ge_directx_resource(
        ge_directx_renderer* renderer, const ComPtr<ID3D12Resource>& resource, unsigned int element_size,
        unsigned int element_count);

    std::array<descriptors_info, GE_DT_COUNT> descriptors;

    ComPtr<ID3D12Resource> dx_resource;

    ge_directx_renderer* renderer;

    unsigned int element_size;
    unsigned int element_count;
};