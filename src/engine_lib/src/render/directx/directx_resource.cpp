#include <render/directx/directx_resource.h>

#include <io/log.h>
#include <render/directx/directx_renderer.h>
#include <render/directx/directx_descriptor_heap.h>

static DXGI_FORMAT
convert_to_dx_format(ge_texture_format format) {
    switch (format) {
        case (ge_texture_format::GE_TF_R8G8B8A8_UNORM): {
            return DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        }
        default: {
            ge_log_error("unhandled case");
            abort();
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE
ge_directx_resource::get_bound_descriptor_cpu_handle(ge_directx_descriptor_type descriptor_type) {
    ge_directx_descriptor* descriptor = descriptors[descriptor_type].descriptor;
    if (descriptor == nullptr) {
        return {(size_t)-1};
    }

    ge_directx_descriptor_heap* heap = descriptor->get_heap();

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        heap->get_dx_heap()->GetCPUDescriptorHandleForHeapStart(),
        descriptor->get_offset_in_descriptors_for_current_frame(), heap->get_descriptor_size());
}

ID3D12Resource*
ge_directx_resource::get_dx_resource() {
    return dx_resource.Get();
}

unsigned int
ge_directx_resource::get_element_size() {
    return element_size;
}

unsigned int
ge_directx_resource::get_element_count() {
    return element_count;
}

ge_directx_resource::ge_directx_resource(
    ge_directx_renderer* renderer, const ComPtr<ID3D12Resource>& resource, unsigned int element_size,
    unsigned int element_count) {
    this->renderer = renderer;
    this->element_count = element_count;
    this->element_size = element_size;
    dx_resource = resource;

    for (descriptors_info& info : descriptors) {
        info.descriptor = nullptr;
        for (ge_directx_descriptor*& descriptor : info.cubemap_faces) {
            descriptor = nullptr;
        }
    }
}

ge_directx_resource*
ge_directx_resource::create_render_target(
    ge_directx_renderer* renderer, ge_texture_format format, unsigned int width, unsigned int height,
    float clear_color[4]) {
    DXGI_FORMAT dx_format = convert_to_dx_format(format);

    CD3DX12_RESOURCE_DESC desc =
        CD3DX12_RESOURCE_DESC::Tex2D(dx_format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE clear{};
    clear.Format = dx_format;
    clear.Color[0] = clear_color[0];
    clear.Color[1] = clear_color[1];
    clear.Color[2] = clear_color[2];
    clear.Color[3] = clear_color[3];

    ComPtr<ID3D12Resource> dx_resource;

    HRESULT result = renderer->get_device()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&dx_resource));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
    ge_directx_resource* resource = new ge_directx_resource(renderer, dx_resource, 0, 0);

    renderer->get_rtv_descriptor_heap()->assign_descriptor(resource, GE_DDT_RTV);

    return resource;
}

ge_directx_resource::~ge_directx_resource() {
    // TODO: to make sure the GPU is not using this resource just waiting for all commands to finish
    renderer->wait_for_gpu_to_finish_work();

    for (descriptors_info& info : descriptors) {
        if (info.descriptor != nullptr) {
            delete info.descriptor;
        }
        for (ge_directx_descriptor*& descriptor : info.cubemap_faces) {
            if (descriptor != nullptr) {
                delete descriptor;
            }
        }
    }
}
