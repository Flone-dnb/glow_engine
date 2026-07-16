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

ge_directx_resource::ge_directx_resource(
    ge_directx_renderer* renderer, const ComPtr<D3D12MA::Allocation>& resource, unsigned int element_size,
    unsigned int element_count)
    : ge_gpu_resource(renderer, element_size, element_count) {
    this->resource = resource;

    for (descriptors_info& info : descriptors) {
        info.descriptor = nullptr;
        for (ge_directx_descriptor*& descriptor : info.cubemap_faces) {
            descriptor = nullptr;
        }
    }
}

ge_directx_resource::~ge_directx_resource() {
    ge_directx_renderer* renderer = (ge_directx_renderer*)get_renderer();
    UINT64 fence = renderer->signal_fence();

    renderer->queue_resource_destruction(
        [renderer = renderer, fence = fence, resource = resource, descriptors = descriptors]() mutable -> bool {
            if (!renderer->is_fence_completed(fence)) {
                return false;
            }

            resource = nullptr;

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

            return true;
        });
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
    return resource->GetResource();
}

ge_directx_resource*
ge_directx_resource::create_render_target(
    ge_directx_renderer* renderer, ge_texture_format format, unsigned int width, unsigned int height,
    const glm::vec4& clear_color) {
    DXGI_FORMAT dx_format = convert_to_dx_format(format);

    CD3DX12_RESOURCE_DESC desc =
        CD3DX12_RESOURCE_DESC::Tex2D(dx_format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE clear{};
    clear.Format = dx_format;
    clear.Color[0] = clear_color[0];
    clear.Color[1] = clear_color[1];
    clear.Color[2] = clear_color[2];
    clear.Color[3] = clear_color[3];

    ComPtr<D3D12MA::Allocation> allocated_resource;

    D3D12MA::ALLOCATION_DESC alloc_desc{};
    alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    HRESULT result = renderer->get_mem_allocator()->CreateResource(
        &alloc_desc, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, allocated_resource.GetAddressOf(), IID_NULL,
        nullptr);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    ge_directx_resource* resource = new ge_directx_resource(renderer, allocated_resource, 0, 0);

    renderer->get_rtv_descriptor_heap()->assign_descriptor(resource, GE_DDT_RTV);

    return resource;
}

ge_directx_resource*
ge_directx_resource::create_resource_with_data(
    ge_directx_renderer* renderer, unsigned int element_size, unsigned int element_count, void* data,
    ge_gpu_resource_usage usage) {
    size_t data_size = (size_t)element_size * element_count;

    D3D12_SUBRESOURCE_DATA upload_data = {};
    upload_data.pData = data;
    upload_data.RowPitch = data_size;
    upload_data.SlicePitch = upload_data.RowPitch;

    auto upload_res_desc = CD3DX12_RESOURCE_DESC::Buffer(data_size);
    auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(data_size);

    return create_resource_with_upload(
        renderer, res_desc, upload_data, upload_res_desc, false, element_size, element_count);
}

ge_directx_resource*
ge_directx_resource::create_resource_with_upload(
    ge_directx_renderer* renderer, const D3D12_RESOURCE_DESC& res_desc, const D3D12_SUBRESOURCE_DATA& to_upload,
    const D3D12_RESOURCE_DESC& upload_res_desc, bool is_texture, unsigned int element_size,
    unsigned int element_count) {
    // In order to create a GPU resource with our data from the CPU we have to do a few steps:
    // 1. Create a GPU resource with DEFAULT heap type (CPU read-only heap) AKA the resulting resource.
    // 2. Create a GPU resource with UPLOAD heap type (CPU read-write heap) AKA the upload resource.
    // 3. Copy our data from the CPU to the resulting resource by using the upload resource.
    // 4. Wait for the GPU to finish copying data and delete the upload resource.

    // 1. Create the resulting resource.
    D3D12MA::ALLOCATION_DESC alloc_desc{};
    alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<D3D12MA::Allocation> resulting_alloc;
    auto init_res_state = D3D12_RESOURCE_STATE_COPY_DEST;

    HRESULT result = renderer->get_mem_allocator()->CreateResource(
        &alloc_desc, &res_desc,
        res_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER
            ? D3D12_RESOURCE_STATE_COMMON // D3D debug layer warns about buffers with `COPY_DEST` state
                                          // and it seems to want `STATE_COMMON`. We keep the intended
                                          // state in the variable for the barrier (below) to work.
            : init_res_state,
        nullptr, resulting_alloc.GetAddressOf(), IID_NULL, nullptr);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    // 2. Create the upload resource.
    alloc_desc = {};
    alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<D3D12MA::Allocation> upload_alloc;
    auto init_upload_state = D3D12_RESOURCE_STATE_GENERIC_READ;

    result = renderer->get_mem_allocator()->CreateResource(
        &alloc_desc, &upload_res_desc, init_upload_state, nullptr, upload_alloc.GetAddressOf(), IID_NULL, nullptr);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12CommandQueue> command_queue;
    ComPtr<ID3D12CommandAllocator> command_allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    create_command_objs_for_upload(renderer, fence, command_queue, command_allocator, command_list);

    // 3. Copy the data.
    UpdateSubresources(
        command_list.Get(), resulting_alloc->GetResource(), upload_alloc->GetResource(), 0, 0, 1, &to_upload);

    // Prepare final state.
    D3D12_RESOURCE_STATES res_state = (res_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0
                                          ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                          : D3D12_RESOURCE_STATE_GENERIC_READ;
    if (is_texture) {
        res_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // Change state.
    CD3DX12_RESOURCE_BARRIER transition =
        CD3DX12_RESOURCE_BARRIER::Transition(resulting_alloc->GetResource(), init_res_state, res_state);
    command_list->ResourceBarrier(1, &transition);

    result = command_list->Close();
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    ID3D12CommandList* commandLists[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // 4. Wait for the operation to finish.
    const UINT64 fence_value = 100;
    command_queue->Signal(fence.Get(), fence_value);

    HANDLE event = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);
    if (event == nullptr) {
        ge_log_error("failed to create event to wait for a fence");
        abort();
    }
    result = fence->SetEventOnCompletion(fence_value, event);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    return new ge_directx_resource(renderer, resulting_alloc, element_size, element_count);
}

void
ge_directx_resource::create_command_objs_for_upload(
    ge_directx_renderer* renderer, ComPtr<ID3D12Fence>& out_fence, ComPtr<ID3D12CommandQueue>& out_queue,
    ComPtr<ID3D12CommandAllocator>& out_allocator, ComPtr<ID3D12GraphicsCommandList>& out_list) {
    // Fence.
    HRESULT result = renderer->get_device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&out_fence));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    // Command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    result = renderer->get_device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&out_queue));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    // Command allocator.
    result = renderer->get_device()->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(out_allocator.GetAddressOf()));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    // Command list.
    result = renderer->get_device()->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, out_allocator.Get(), nullptr, IID_PPV_ARGS(out_list.GetAddressOf()));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    result = out_allocator->Reset();
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    result = out_list->Reset(out_allocator.Get(), nullptr);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
}
