#pragma once

#include <functional>
#include <queue>
#include <unordered_set>
#include <string>
#include <mutex>
#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class ge_directx_resource;
class ge_directx_renderer;
class ge_directx_descriptor_heap;
class ge_directx_descriptor_range;

enum ge_directx_descriptor_heap_type { GE_DDHT_RTV, GE_DDHT_DSV, GE_DDHT_CBV_SRV_UAV, GE_DDHT_SAMPLER };

enum ge_directx_descriptor_type {
    GE_DDT_RTV = 0,
    GE_DDT_DSV,
    GE_DDT_CBV,
    GE_DDT_SRV,
    GE_DDT_UAV,
    GE_DDT_SAMPLER,

    GE_DT_COUNT // <- marks the numbers of elements in this enum
};

// Represents a descriptor (to a resource) that is stored in a descriptor heap.
// Automatically marked as unused in destructor.
class ge_directx_descriptor {
    // Fully managed by descriptor heap.
    friend class ge_directx_descriptor_heap;

  public:
    ~ge_directx_descriptor();

    ge_directx_descriptor() = delete;
    ge_directx_descriptor(const ge_directx_descriptor&) = delete;
    ge_directx_descriptor& operator=(const ge_directx_descriptor&) = delete;

    // Returns offset of this descriptor from the heap start
    // (offset is specified in descriptors, not an actual index).
    //
    // Returned value is only valid until the descriptor heap is not resized, so it's only
    // safe to call this function when you know that the descriptor heap will not resize.
    // You shouldn't store returned offset for more than 1 frame as it might change
    // after a frame is submitted (because the heap may resize).
    int
    get_offset_in_descriptors_for_current_frame() {
        return offset_in_descriptors;
    }

    // Calculates offset of the descriptor from the start of the descriptor range (if @ref get_descriptor range is valid).
    //
    // Returned value is only valid until the descriptor heap is not resized, so it's only
    // safe to call this function when you know that the descriptor heap will not resize.
    // You shouldn't store returned offset for more than 1 frame as it might change
    // after a frame is submitted (because the heap may resize).
    int get_offset_from_range_start_for_current_frame();

    // Do not delete returned pointer, always valid pointer.
    ge_directx_descriptor_heap* get_heap();

    // If this descriptor was allocated from a descriptor range returns a non `nullptr` value.
    ge_directx_descriptor_range* get_descriptor_range();

    // Returns resource that owns this descriptor.
    // Do not delete returned pointer, always valid pointer.
    ge_directx_resource* get_owner_resource();

  private:
    ge_directx_descriptor(
        ge_directx_descriptor_heap* heap, ge_directx_descriptor_type type, ge_directx_resource* resource,
        unsigned int offset_in_descriptors, unsigned int cubemap_face_idx = 0xFFFFFFFF,
        ge_directx_descriptor_range* range = nullptr);

    // Always valid. Heap that allocated this descriptor.
    ge_directx_descriptor_heap* heap;

    // Always valid. Resource to which the descriptor is bound.
    ge_directx_resource* resource;

    // `nullptr` if not allocated from a range.
    ge_directx_descriptor_range* range;

    // 0xFFFFFFFF if not references a cubemap's face.
    unsigned int cubemap_face_idx;

    // Offset of this descriptor from the heap start (offset is specified in descriptors,
    // not an actual index).
    unsigned int offset_in_descriptors;

    ge_directx_descriptor_type type;
};

// Works as a mini descriptor heap that operates on descriptors in a continuous range
// (can be used for bindless bindings of descriptor arrays).
// Size of the range automatically changes (expands/shrinks) depending on the usage.
class ge_directx_descriptor_range {
    // Fully managed by descriptor heap.
    friend class ge_directx_descriptor_heap;

  public:
    ~ge_directx_descriptor_range();

    ge_directx_descriptor_range() = delete;
    ge_directx_descriptor_range(const ge_directx_descriptor_range&) = delete;
    ge_directx_descriptor_range& operator=(const ge_directx_descriptor_range&) = delete;

    // Returns index of the first descriptor of this range in the descriptor heap.
    int get_range_start_in_heap();

  private:
    ge_directx_descriptor_range(
        const char* name, ge_directx_descriptor_heap* heap, const std::function<void()>& on_range_indices_changed);

    // Looks if there is a free (unused) index in the range that can be used to create a new descriptor or
    // if there's no space (expansion needed). Returned index is marked as "in-use" and expects a new descriptor to use it.
    //
    // Returns -1 if no free space left (expansion needed), otherwise index (offset in descriptors) from the heap start.
    int try_reserve_index();

    void mark_descriptor_as_unused(ge_directx_descriptor* descriptor);

    // Descriptors allocated from this range.
    // Size if this array marks the size (actually used number of elements) of the range.
    std::unordered_set<ge_directx_descriptor*> descriptors;

    // Indices (relative to heap start) of descriptors that were allocated from this range
    // but no longer being used and can be reused.
    std::queue<int> no_longer_used_indices;

    // Callback that will be called after the range was moved in the heap due to things like heap expanding/shrinking.
    std::function<void()> on_range_indices_changed;

    std::string name;

    // Index of the first descriptor of this range in the heap.
    // -1 if range was not initialized.
    int range_start_in_heap;

    // Capacity of the range.
    int capacity;

    ge_directx_descriptor_heap* heap;

    // Index of the next free descriptor (relative to range start) that can be used.
    int next_free_idx;
};

// Manages DirectX descriptors.
// Size of the heap automatically changes (expands/shrinks) depending on the usage.
class ge_directx_descriptor_heap {
    // Only renderer is allowed to create this object.
    friend class ge_directx_renderer;

    // Notifies the heap.
    friend class ge_directx_descriptor;
    friend class ge_directx_descriptor_range;

  public:
    ~ge_directx_descriptor_heap();

    ge_directx_descriptor_heap() = delete;
    ge_directx_descriptor_heap(const ge_directx_descriptor_heap&) = delete;
    ge_directx_descriptor_heap& operator=(const ge_directx_descriptor_heap&) = delete;

    // Creates a new descriptor that points to the given resource, the descriptor is saved in the resource.
    // You can use this function to assign different types of descriptors to the same resource.
    // Optionally if you want to allocate the descriptor from a specific descriptor range you need to specify one.
    // If the resource is a cubemap specify `true` to also bind descriptors that reference specific cubemap faces,
    // otherwise specify `false` to only bind 1 descriptor that references the entire resource.
    void assign_descriptor(
        ge_directx_resource* resource, ge_directx_descriptor_type descriptor_type, ge_directx_descriptor_range* range = nullptr,
        bool descriptor_per_cubemap_face = true);

    // Allocates a continuous range of descriptors that can be used for bindless bindings of descriptor arrays.
    // The callback will be called after the range was moved inthe heap due to things like heap expanding/shrinking.
    //
    // You must delete returned pointer.
    ge_directx_descriptor_range*
    allocate_descriptor_range(const char* name, const std::function<void()>& on_range_indices_changes);

    unsigned int get_descriptor_size();

    ID3D12DescriptorHeap* get_dx_heap();

  private:
    // Mutex guarded resources because GPU resources and be created in parallel.
    struct guarded_resource {
        // It's safe to store raw pointers here bacuse ranges will notify the heap in their destructor.
        std::unordered_set<ge_directx_descriptor_range*> ranges;

        ComPtr<ID3D12DescriptorHeap> heap;

        // Size of this array DOES NOT represent the total number of descriptors allocated from the heap.
        // Instead use @ref size.
        std::unordered_set<ge_directx_descriptor*> descriptors;

        // Indices (relative to heap start) of descriptors that were created
        // but no longer being used and can be reused.
        std::queue<int> no_longer_used_indices;

        int capacity;

        // Includes elements from @ref descriptors and space taken by all allocated ranges.
        int size;

        // Index of the next free descriptor that can be used.
        int next_free_idx;
    };

    // Creates a new heap.
    ge_directx_descriptor_heap(ge_directx_renderer* renderer, ge_directx_descriptor_heap_type type);

    // (Re)creates the heap and updates old descriptors and ranges. This function is also used to expand/shrink the heap.
    // If this event was caused by changes in descriptor range specify one for logging.
    void recreate_heap(int new_capacity, ge_directx_descriptor_range* changed_range, guarded_resource& state);

    void create_view(
        const D3D12_CPU_DESCRIPTOR_HANDLE& heap_handle, ge_directx_resource* resource, ge_directx_descriptor_type type,
        unsigned int cubemap_face_idx = 0xFFFFFFFF);

    // Expands the specified descriptor range and also expands or re-creates the heap to support updated range.
    void expand_range(ge_directx_descriptor_range* range, guarded_resource& state);

    bool is_shrinking_possible(guarded_resource& state);

    // Called from descriptor's destructor to mark descriptor as no longer used.
    void
    on_before_descriptor_destroyed(ge_directx_descriptor* descriptor, ge_directx_descriptor_range* range = nullptr);
    void on_before_descriptor_range_destroyed(ge_directx_descriptor_range* range);

    std::pair<std::mutex, guarded_resource> mtx_res;

    ge_directx_renderer* renderer;

    unsigned int descriptor_size;
    ge_directx_descriptor_heap_type heap_type;
};