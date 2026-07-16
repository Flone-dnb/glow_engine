#pragma once

#include <array>
#include <render/gpu_resource.h>
#include <render/directx/directx_descriptor_heap.h>
#include <render/texture_format.h>
#include <glm/vec4.hpp>
#include <D3D12MemoryAllocator/include/D3D12MemAlloc.h>
#include <directx/d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class ge_directx_renderer;

// GPU resource.
class ge_directx_resource : public ge_gpu_resource {
    // Assigns descriptors.
    friend class ge_directx_descriptor_heap;

  public:
    virtual ~ge_directx_resource() override;

    ge_directx_resource(const ge_directx_resource&) = delete;
    ge_directx_resource& operator=(const ge_directx_resource&) = delete;

    // Create a new render target texture.
    static ge_directx_resource* create_render_target(
        ge_directx_renderer* renderer, ge_texture_format format, unsigned int width, unsigned int height,
        const glm::vec4& clear_color);

    static ge_directx_resource* create_resource_with_data(
        ge_directx_renderer* renderer, unsigned int element_size, unsigned int element_count, void* data,
        ge_gpu_resource_usage usage);

    // Returns `(size_t)-1` if no such descriptor bound.
    D3D12_CPU_DESCRIPTOR_HANDLE get_bound_descriptor_cpu_handle(ge_directx_descriptor_type descriptor_type);

    ID3D12Resource* get_dx_resource();

  private:
    struct descriptors_info {
        // `nullptr` if not bound.
        ge_directx_descriptor* descriptor;

        // `nullptr` if not bound, otherwise if the resource is a cubemap each descriptor references a specific cubemap face.
        std::array<ge_directx_descriptor*, 6> cubemap_faces;
    };

    static ge_directx_resource* create_resource_with_upload(
        ge_directx_renderer* renderer, const D3D12_RESOURCE_DESC& res_desc, const D3D12_SUBRESOURCE_DATA& to_upload,
        const D3D12_RESOURCE_DESC& upload_res_desc, bool is_texture, unsigned int element_size,
        unsigned int element_count);

    static void create_command_objs_for_upload(
        ge_directx_renderer* renderer, ComPtr<ID3D12Fence>& out_fence, ComPtr<ID3D12CommandQueue>& out_queue,
        ComPtr<ID3D12CommandAllocator>& out_allocator, ComPtr<ID3D12GraphicsCommandList>& out_list);

    ge_directx_resource() = delete;
    ge_directx_resource(
        ge_directx_renderer* renderer, const ComPtr<D3D12MA::Allocation>& resource, unsigned int element_size,
        unsigned int element_count);

    std::array<descriptors_info, GE_DT_COUNT> descriptors;

    ComPtr<D3D12MA::Allocation> resource;
};