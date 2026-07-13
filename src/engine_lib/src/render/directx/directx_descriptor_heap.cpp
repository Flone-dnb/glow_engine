#include <render/directx/directx_descriptor_heap.h>

#include <material/texture_filtering.h>
#include <io/log.h>
#include <render/directx/directx_renderer.h>
#include <render/directx/directx_resource.h>

static const int heap_grow_size = 300;
static const int range_grow_size = 50;

static const char*
heap_type_to_str(ge_directx_descriptor_heap_type type) {
    switch (type) {
        case (GE_DDHT_RTV): {
            return "RTV";
        }
        case (GE_DDHT_DSV): {
            return "DSV";
        }
        case (GE_DDHT_CBV_SRV_UAV): {
            return "CBV/SRV/UAV";
        }
        case (GE_DDHT_SAMPLER): {
            return "SAMPLER";
        }
        default: {
            ge_log_error("unhandled case");
            abort();
        }
    }
}

static D3D12_DESCRIPTOR_HEAP_TYPE
heap_type_to_d3d(ge_directx_descriptor_heap_type type) {
    switch (type) {
        case (GE_DDHT_RTV): {
            return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        }
        case (GE_DDHT_DSV): {
            return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        }
        case (GE_DDHT_CBV_SRV_UAV): {
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        }
        case (GE_DDHT_SAMPLER): {
            return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        }
        default: {
            ge_log_error("unhandled case");
            abort();
        }
    }
}

static bool
is_descriptor_type_handled_by_this_heap(ge_directx_descriptor_heap_type heap_type, ge_directx_descriptor_type descriptor_type) {
    switch (heap_type) {
        case (GE_DDHT_RTV): {
            return descriptor_type == GE_DDT_RTV;
        }
        case (GE_DDHT_DSV): {
            return descriptor_type == GE_DDT_DSV;
        }
        case (GE_DDHT_CBV_SRV_UAV): {
            return descriptor_type == GE_DDT_CBV || descriptor_type == GE_DDT_SRV || descriptor_type == GE_DDT_UAV;
        }
        case (GE_DDHT_SAMPLER): {
            return descriptor_type == GE_DDT_SAMPLER;
        }
        default: {
            ge_log_error("unhandled case");
            abort();
        }
    }
}

ge_directx_descriptor_heap::~ge_directx_descriptor_heap() {
    std::lock_guard<std::mutex> guard(mtx_res.first);
    auto& state = mtx_res.second;

    if (!state.descriptors.empty()) {
        ge_log_error_fmt(
            "descriptor heap %s is being destroyed but there are still %zu descriptor(s) alive",
            heap_type_to_str(heap_type), state.descriptors.size());
        abort();
    }

    if (!state.ranges.empty()) {
        ge_log_error_fmt(
            "descriptor heap %s is being destroyed but there are still %zu ranges(s) alive",
            heap_type_to_str(heap_type), state.ranges.size());
        abort();
    }

    if (state.size != 0) {
        ge_log_error_fmt(
            "descriptor heap %s is being destroyed but its size is %i", heap_type_to_str(heap_type), state.size);
        abort();
    }
}

void
ge_directx_descriptor_heap::assign_descriptor(
    ge_directx_resource* resource, ge_directx_descriptor_type descriptor_type, ge_directx_descriptor_range* range,
    bool descriptor_per_cubemap_face) {
    if (!is_descriptor_type_handled_by_this_heap(heap_type, descriptor_type)) {
        ge_log_error("this heap does not create descriptors of the specified type");
        abort();
    }

    std::lock_guard<std::mutex> guard(mtx_res.first);
    auto& state = mtx_res.second;

    const auto set_descriptor = [&](unsigned int cubemap_face_idx) {
        int descriptor_idx = 0;

        if (range != nullptr) {
            auto range_it = state.ranges.find(range);
            if (range_it == state.ranges.end()) {
                ge_log_error("unable to allocate a descriptor from the specified descriptor range because the "
                             "range is invalid");
                abort();
            }

            descriptor_idx = range->try_reserve_index();
            if (descriptor_idx < 0) {
                expand_range(range, state);

                descriptor_idx = range->try_reserve_index();
                if (descriptor_idx < 0) {
                    ge_log_error_fmt(
                        "failed to reserve index from range %s even after expanding it", range->name.c_str());
                    abort();
                }
            }
        } else {
            if (state.size == state.capacity) {
                recreate_heap(state.capacity + heap_grow_size, nullptr, state);
            }

            if (state.next_free_idx == state.capacity) {
                if (state.no_longer_used_indices.empty()) {
                    ge_log_error("failed to allocate a new descriptor - no free space");
                    abort();
                }

                descriptor_idx = state.no_longer_used_indices.front();
                state.no_longer_used_indices.pop();
            } else {
                descriptor_idx = state.next_free_idx;
                state.next_free_idx += 1;
            }

            state.size += 1;
        }

        auto heap_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(state.heap->GetCPUDescriptorHandleForHeapStart());
        heap_handle.Offset(descriptor_idx, descriptor_size);

        create_view(heap_handle, resource, descriptor_type, cubemap_face_idx);
        ge_directx_descriptor* descriptor =
            new ge_directx_descriptor(this, descriptor_type, resource, descriptor_idx, cubemap_face_idx, range);

        if (range != nullptr) {
            range->descriptors.insert(descriptor);
        } else {
            state.descriptors.insert(descriptor);
        }

        if (cubemap_face_idx != 0xFFFFFFFF) {
            resource->descriptors[descriptor_type].cubemap_faces[cubemap_face_idx] = descriptor;
        } else {
            resource->descriptors[descriptor_type].descriptor = descriptor;
        }
    };

    D3D12_RESOURCE_DESC desc = resource->get_dx_resource()->GetDesc();
    bool is_cubemap = desc.DepthOrArraySize == 6;

    // Bind to the entire resource.
    set_descriptor(0xFFFFFFFF);

    if (is_cubemap && descriptor_per_cubemap_face) {
        for (UINT16 face_idx = 0; face_idx < desc.DepthOrArraySize; face_idx++) {
            set_descriptor(face_idx);
        }
    }
}

ge_directx_descriptor_range*
ge_directx_descriptor_heap::allocate_descriptor_range(
    const char* name, const std::function<void()>& on_range_indices_changes) {
    std::lock_guard<std::mutex> guard(mtx_res.first);
    auto& state = mtx_res.second;

    ge_directx_descriptor_range* range = new ge_directx_descriptor_range(name, this, on_range_indices_changes);
    state.ranges.insert(range);

    expand_range(range, state);

    return range;
}

unsigned int
ge_directx_descriptor_heap::get_descriptor_size() {
    return descriptor_size;
}

ID3D12DescriptorHeap*
ge_directx_descriptor_heap::get_dx_heap() {
    return mtx_res.second.heap.Get();
}

ge_directx_descriptor_heap::ge_directx_descriptor_heap(ge_directx_renderer* renderer, ge_directx_descriptor_heap_type type) {
    static_assert(heap_grow_size % 2 == 0, "because we use INT/INT");

    std::lock_guard<std::mutex> guard(mtx_res.first);
    auto& state = mtx_res.second;

    this->renderer = renderer;
    heap_type = type;
    state.capacity = 0;
    state.size = 0;
    state.next_free_idx = 0;

    switch (heap_type) {
        case (GE_DDHT_RTV): {
            descriptor_size = renderer->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            break;
        }
        case (GE_DDHT_DSV): {
            descriptor_size = renderer->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            break;
        }
        case (GE_DDHT_CBV_SRV_UAV): {
            descriptor_size =
                renderer->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            break;
        }
        case (GE_DDHT_SAMPLER): {
            descriptor_size =
                renderer->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            break;
        }
    }

    int init_capacity = heap_grow_size;
    if (type == GE_DDHT_SAMPLER) {
        init_capacity = 10; // use a smaller capacity since there won't be that much samplers
    }

    recreate_heap(init_capacity, nullptr, state);

    if (type == GE_DDHT_SAMPLER) {
        // Create pointer sampler.
        D3D12_SAMPLER_DESC desc;
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        desc.BorderColor[0] = 1.0f;
        desc.BorderColor[1] = 1.0f;
        desc.BorderColor[2] = 1.0f;
        desc.BorderColor[3] = 1.0f;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = D3D12_FLOAT32_MAX;
        renderer->get_device()->CreateSampler(
            &desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(
                       state.heap->GetCPUDescriptorHandleForHeapStart(), GE_TF_POINT, descriptor_size));

        // Create linear sampler.
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        renderer->get_device()->CreateSampler(
            &desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(
                       state.heap->GetCPUDescriptorHandleForHeapStart(), GE_TF_LINEAR, descriptor_size));

        // Create anisotropic sampler.
        desc.Filter = D3D12_FILTER_ANISOTROPIC;
        renderer->get_device()->CreateSampler(
            &desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(
                       state.heap->GetCPUDescriptorHandleForHeapStart(), GE_TF_ANISOTROPIC, descriptor_size));
    }
}

void
ge_directx_descriptor_heap::recreate_heap(
    int new_capacity, ge_directx_descriptor_range* changed_range, guarded_resource& state) {
    ge_log_info_fmt(
        "waiting for the GPU to finish work up to this point to (re)create %s descriptor heap from "
        "capacity %i to %i (current actual heap size: %i) %s%s",
        heap_type_to_str(heap_type), state.capacity, new_capacity, state.size,
        changed_range != nullptr ? "due to changes in descriptor range " : "",
        changed_range != nullptr ? changed_range->name.c_str() : "");

    std::lock_guard<std::mutex> draw_guard(renderer->get_draw_mutex());
    renderer->wait_for_gpu_to_finish_work();

    D3D12_DESCRIPTOR_HEAP_TYPE d3d_heap_type = heap_type_to_d3d(heap_type);
    bool is_shader_visible =
        d3d_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || d3d_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = new_capacity;
    desc.Type = d3d_heap_type;
    desc.Flags = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT result =
        renderer->get_device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(state.heap.ReleaseAndGetAddressOf()));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    state.capacity = new_capacity;

    // Rebind views and update indices.
    // Start from 0 heap index, increment and update old offsets
    // to "shrink" heap usage (needed for heap shrinking).
    auto heap_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(state.heap->GetCPUDescriptorHandleForHeapStart());
    int curr_heap_idx = 0;

    // Prepare a lambda to update a descriptor.
    const auto update_descriptor = [&](ge_directx_descriptor* descriptor) {
#if defined(DEBUG)
        // Self check: make sure we don't assign indices out of heap bounds.
        if (curr_heap_idx >= state.capacity) {
            ge_log_error_fmt("next free descriptor index %i reached heap capacity %i", curr_heap_idx, state.capacity);
            abort();
        }
#endif

        create_view(heap_handle, descriptor->resource, descriptor->type, descriptor->cubemap_face_idx);

        descriptor->offset_in_descriptors = curr_heap_idx;

        heap_handle.Offset(1, descriptor_size);
        curr_heap_idx += 1;
    };

    // First assign space for continuous descriptor ranges.
    for (ge_directx_descriptor_range* range : state.ranges) {
        bool first_init = range->range_start_in_heap < 0;
        range->range_start_in_heap = curr_heap_idx;

        range->next_free_idx = 0;
        range->no_longer_used_indices = {};

        // Update old descriptors.
        for (ge_directx_descriptor* descriptor : range->descriptors) {
#if defined(DEBUG)
            // Self check: make sure we don't assign indices out of heap bounds.
            if (range->next_free_idx >= range->capacity) {
                ge_log_error_fmt(
                    "next free range descriptor index %i reached range capacity %i", range->next_free_idx,
                    range->capacity);
                abort();
            }
#endif

            update_descriptor(descriptor);
            range->next_free_idx += 1;
        }

        if (!first_init) {
            range->on_range_indices_changed();
        } // else: the user still haven't received the range pointer so no need to trigger the callback

        // Jump to the end of the range.
        int skip_count = range->capacity - (int)range->descriptors.size();
        if (skip_count < 0) {
            ge_log_error_fmt(
                "unexpected descriptor count, range capacity: %i, range size: %zu", range->capacity,
                range->descriptors.size());
            abort();
        }
        curr_heap_idx += skip_count;
        heap_handle.Offset(skip_count, descriptor_size);
    }

    // Update single descriptors.
    for (ge_directx_descriptor* descriptor : state.descriptors) {
        update_descriptor(descriptor);
    }

    state.next_free_idx = curr_heap_idx;
    state.no_longer_used_indices = {};
}

void
ge_directx_descriptor_heap::create_view(
    const D3D12_CPU_DESCRIPTOR_HANDLE& heap_handle, ge_directx_resource* resource, ge_directx_descriptor_type type,
    unsigned int cubemap_face_idx) {
    D3D12_RESOURCE_DESC res_desc = resource->get_dx_resource()->GetDesc();

    switch (type) {
        case (GE_DDT_RTV): {
            D3D12_RENDER_TARGET_VIEW_DESC rtv_desc{};
            rtv_desc.Format = res_desc.Format;

            if (res_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
                ge_log_error("unable to create RTV: unsupported resource dimension");
                abort();
            }

            rtv_desc.ViewDimension =
                res_desc.SampleDesc.Count > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice = 0;
            rtv_desc.Texture2D.PlaneSlice = 0;

            if (res_desc.DepthOrArraySize > 1) {
                rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtv_desc.Texture2DArray.MipSlice = 0;
                rtv_desc.Texture2DArray.FirstArraySlice = 0;
                rtv_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
            }

            if (cubemap_face_idx != 0xFFFFFFFF) {
                rtv_desc.Texture2DArray.FirstArraySlice = cubemap_face_idx;
                rtv_desc.Texture2DArray.ArraySize = 1;
            }

            renderer->get_device()->CreateRenderTargetView(resource->get_dx_resource(), &rtv_desc, heap_handle);
            break;
        }
        case (GE_DDT_DSV): {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
            dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
            dsv_desc.Format = res_desc.Format;

            if (res_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
                ge_log_error("unable to create DSV: unsupported resource dimension");
                abort();
            }

            dsv_desc.ViewDimension =
                res_desc.SampleDesc.Count > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Texture2D.MipSlice = 0;

            if (res_desc.DepthOrArraySize > 1) {
                dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsv_desc.Texture2DArray.MipSlice = 0;
                dsv_desc.Texture2DArray.FirstArraySlice = 0;
                dsv_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
            }

            if (cubemap_face_idx != 0xFFFFFFFF) {
                dsv_desc.Texture2DArray.FirstArraySlice = cubemap_face_idx;
                dsv_desc.Texture2DArray.ArraySize = 1;
            }

            renderer->get_device()->CreateDepthStencilView(resource->get_dx_resource(), &dsv_desc, heap_handle);
            break;
        }
        case (GE_DDT_CBV): {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
            cbv_desc.BufferLocation = resource->get_dx_resource()->GetGPUVirtualAddress();
            cbv_desc.SizeInBytes = res_desc.Width;

            renderer->get_device()->CreateConstantBufferView(&cbv_desc, heap_handle);
            break;
        }
        case (GE_DDT_SRV): {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            switch (res_desc.Dimension) {
                case (D3D12_RESOURCE_DIMENSION_TEXTURE3D): {
                    ge_log_error("unable to create SRV: texture 3D not supported");
                    abort();
                    break;
                }
                case (D3D12_RESOURCE_DIMENSION_TEXTURE2D): {
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MostDetailedMip = 0;
                    srv_desc.Texture2D.MipLevels = res_desc.MipLevels;
                    if (res_desc.Format == DXGI_FORMAT_D32_FLOAT) {
                        // SRV cannot be created with the depth component so use the red component instead.
                        srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
                    } else {
                        srv_desc.Format = res_desc.Format;
                    }

                    if (res_desc.DepthOrArraySize > 1) {
                        srv_desc.ViewDimension = res_desc.DepthOrArraySize == 6 ? D3D12_SRV_DIMENSION_TEXTURECUBE
                                                                                : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        srv_desc.Texture2DArray.PlaneSlice = 0;
                        srv_desc.Texture2DArray.MostDetailedMip = 0;
                        srv_desc.Texture2DArray.MipLevels = res_desc.MipLevels;
                        srv_desc.Texture2DArray.FirstArraySlice = 0;
                        srv_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
                    }

                    if (cubemap_face_idx != 0xFFFFFFFF) {
                        srv_desc.Texture2DArray.FirstArraySlice = cubemap_face_idx;
                        srv_desc.Texture2DArray.ArraySize = 1;
                    }
                    break;
                }
                case (D3D12_RESOURCE_DIMENSION_BUFFER): {
                    // Make sure element size / count are specified (because they can be 0 sometimes).
                    if (resource->get_element_count() == 0 || resource->get_element_size() == 0) {
                        ge_log_error("unable to create a SRV because resource element size/count was not specified");
                        abort();
                    }
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    srv_desc.Buffer.FirstElement = 0;
                    srv_desc.Buffer.StructureByteStride = resource->get_element_size();
                    srv_desc.Buffer.NumElements = resource->get_element_count();
                    srv_desc.Format = DXGI_FORMAT_UNKNOWN; // must be UNKNOWN if StructureByteStride > 0
                    break;
                }
            }

            renderer->get_device()->CreateShaderResourceView(resource->get_dx_resource(), &srv_desc, heap_handle);
            break;
        }
        case (GE_DDT_UAV): {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
            uav_desc.Format = res_desc.Format;
            uav_desc.Texture2D.MipSlice = 0;

            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_UNKNOWN;
            switch (res_desc.Dimension) {
                case (D3D12_RESOURCE_DIMENSION_BUFFER): {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    break;
                }
                case (D3D12_RESOURCE_DIMENSION_TEXTURE1D): {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    break;
                }
                case (D3D12_RESOURCE_DIMENSION_TEXTURE2D): {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    break;
                }
                case (D3D12_RESOURCE_DIMENSION_TEXTURE3D): {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                    break;
                }
            }

            if (res_desc.Dimension == D3D12_RESOURCE_DIMENSION_UNKNOWN
                || res_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
                // Make sure element size / count are specified (because they can be 0 sometimes).
                if (resource->get_element_count() == 0 || resource->get_element_size() == 0) {
                    ge_log_error("unable to create a SRV because resource element size/count was not specified");
                    abort();
                }
                uav_desc.Buffer.FirstElement = 0;
                uav_desc.Buffer.StructureByteStride = resource->get_element_size();
                uav_desc.Buffer.NumElements = resource->get_element_count();
                uav_desc.Format = DXGI_FORMAT_UNKNOWN; // must be UNKNOWN if StructureByteStride > 0
            }

            renderer->get_device()->CreateUnorderedAccessView(
                resource->get_dx_resource(), nullptr, &uav_desc, heap_handle);
            break;
        }
        case (GE_DDT_SAMPLER): {
            ge_log_error("unable to create a sampler view - operation not supported");
            abort();
            break;
        }
        case (GE_DT_COUNT): {
            ge_log_error("unable to create a descriptor view - invalid descriptor type");
            abort();
            break;
        }
    }
}

void
ge_directx_descriptor_heap::expand_range(ge_directx_descriptor_range* range, guarded_resource& state) {
    range->capacity += range_grow_size;
    state.size += range_grow_size;

    if (state.size > state.capacity) {
        recreate_heap(state.capacity + heap_grow_size, range, state);
    } else {
        // Re-create the heap to allocate space for this range.
        recreate_heap(state.capacity, range, state);
    }
}

bool
ge_directx_descriptor_heap::is_shrinking_possible(guarded_resource& state) {
    if (state.capacity < heap_grow_size * 2) {
        return false;
    }

    // Only shrink if we can erase `grow_size` and will still have some free space
    // (i.e. will not be on the edge to expand).
    if (state.size > (state.capacity - heap_grow_size - heap_grow_size / 2)) {
        return false;
    }

    return true;
}

void
ge_directx_descriptor_heap::on_before_descriptor_destroyed(
    ge_directx_descriptor* descriptor, ge_directx_descriptor_range* range) {
    std::lock_guard<std::mutex> guard(mtx_res.first);
    auto& state = mtx_res.second;

    if (range == nullptr) {
        auto descriptor_it = state.descriptors.find(descriptor);
        if (descriptor_it == state.descriptors.end()) {
            ge_log_error_fmt(
                "descriptor notified the %s heap about being destroyed but the heap is unable to find this descriptor",
                heap_type_to_str(heap_type));
            abort();
        }
        state.descriptors.erase(descriptor_it);

        state.no_longer_used_indices.push(descriptor->offset_in_descriptors);
        state.size -= 1;

        if (is_shrinking_possible(state)) {
            recreate_heap(state.capacity - heap_grow_size, range, state);
        }

        return;
    }

    auto range_it = state.ranges.find(range);
    if (range_it == state.ranges.end()) {
        ge_log_error_fmt(
            "descriptor notified the %s heap about being destroyed but the heap is unable to find descriptor's range",
            heap_type_to_str(heap_type));
        abort();
    }

    range->mark_descriptor_as_unused(descriptor);

    if (!is_shrinking_possible(state)) {
        // Nothing else to do.
        return;
    }

    range->capacity -= range_grow_size;
    state.size -= range_grow_size;

    recreate_heap(state.capacity - heap_grow_size, range, state);
}

void
ge_directx_descriptor_heap::on_before_descriptor_range_destroyed(ge_directx_descriptor_range* range) {
    std::lock_guard<std::mutex> guard(mtx_res.first);
    auto& state = mtx_res.second;

    auto range_it = state.ranges.find(range);
    if (range_it == state.ranges.end()) {
        ge_log_error_fmt(
            "a descriptor range notified the %s heap about being destroyed but the heap is unable to find this range",
            heap_type_to_str(heap_type));
        abort();
    }
    state.ranges.erase(range_it);
    state.size -= range->capacity;

    if (is_shrinking_possible(state)) {
        recreate_heap(state.capacity - heap_grow_size, range, state);
    } else {
        // Re-create the heap to remove space for deleted range.
        recreate_heap(state.capacity, range, state);
    }
}

ge_directx_descriptor_range::~ge_directx_descriptor_range() {
    if (!descriptors.empty()) {
        ge_log_error_fmt(
            "descriptor range %s is being destroyed but there are still %zu active descriptors that reference it",
            name.c_str(), descriptors.size());
        abort();
    }

    heap->on_before_descriptor_range_destroyed(this);
}

int
ge_directx_descriptor_range::get_range_start_in_heap() {
    return range_start_in_heap;
}

ge_directx_descriptor_range::ge_directx_descriptor_range(
    const char* name, ge_directx_descriptor_heap* heap, const std::function<void()>& on_range_indices_changed) {
    this->name = name;
    this->heap = heap;
    this->on_range_indices_changed = on_range_indices_changed;
    range_start_in_heap = 0;
    capacity = 0;
    next_free_idx = 0;
}

int
ge_directx_descriptor_range::try_reserve_index() {
    if (next_free_idx == capacity) {
        if (no_longer_used_indices.empty()) {
            return -1;
        }

        int idx = no_longer_used_indices.front();
        no_longer_used_indices.pop();

        return idx;
    }

    int idx = range_start_in_heap + next_free_idx;
    next_free_idx += 1;
    return idx;
}

void
ge_directx_descriptor_range::mark_descriptor_as_unused(ge_directx_descriptor* descriptor) {
    auto descriptor_it = descriptors.find(descriptor);
    if (descriptor_it == descriptors.end()) {
        ge_log_error_fmt(
            "descriptor notified the range %s about being destroyed but the heap is unable to find this descriptor",
            name.c_str());
        abort();
    }
    descriptors.erase(descriptor_it);

    no_longer_used_indices.push(descriptor->get_offset_in_descriptors_for_current_frame());

    // Nothing else to do: the heap will check for range shrink condition.
}

ge_directx_descriptor::~ge_directx_descriptor() { heap->on_before_descriptor_destroyed(this, range); }

int
ge_directx_descriptor::get_offset_from_range_start_for_current_frame() {
#if defined(DEBUG)
    if (range == nullptr) {
        ge_log_error("expected the descriptor to be allocated from a range");
        abort();
    }
#endif

    int result = offset_in_descriptors - range->get_range_start_in_heap();
#if defined(DEBUG)
    if (result < 0) {
        ge_log_error("failed to calculate descriptor offset from descriptor range start because the resulting index is "
                     "negative");
        abort();
    }
#endif

    return result;
}

ge_directx_descriptor_heap*
ge_directx_descriptor::get_heap() {
    return heap;
}

ge_directx_descriptor_range*
ge_directx_descriptor::get_descriptor_range() {
    return range;
}

ge_directx_resource*
ge_directx_descriptor::get_owner_resource() {
    return resource;
}

ge_directx_descriptor::ge_directx_descriptor(
    ge_directx_descriptor_heap* heap, ge_directx_descriptor_type type, ge_directx_resource* resource,
    unsigned int offset_in_descriptors, unsigned int cubemap_face_idx, ge_directx_descriptor_range* range) {
    this->heap = heap;
    this->type = type;
    this->resource = resource;
    this->offset_in_descriptors = offset_in_descriptors;
    this->cubemap_face_idx = cubemap_face_idx;
    this->range = range;
}