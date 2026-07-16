#pragma once

#include <mutex>
#include <functional>
#include <queue>
#include <atomic>
#include <array>
#include <render/renderer.h>
#include <directx/d3dx12.h>
#include <D3D12MemoryAllocator/include/D3D12MemAlloc.h>
#include <dxgi1_4.h>
#include <wrl.h>
using namespace Microsoft::WRL;

// Helper function for logging.
static std::string
hresult_to_string(HRESULT result) {
    LPSTR error_msg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), reinterpret_cast<LPSTR>(&error_msg), 0, nullptr);

    std::string out;
    if (error_msg != nullptr) {
        out = error_msg;
        LocalFree(error_msg);
    } else {
        out += "unknown error";
    }

    return out;
}

class ge_render_target;
class ge_game_instance;
class ge_window;
class ge_directx_descriptor_heap;
class ge_directx_pso_manager;

// Used for submitting multiple frames without waiting for the GPU.
struct ge_directx_frame_resource {
    ComPtr<ID3D12CommandAllocator> dx_command_allocator;

    // fence signaled into the command queue after all frame commands submitted
    UINT64 fence_value;
};

// DirectX 12 renderer.
class ge_directx_renderer : public ge_renderer {
  public:
    ge_directx_renderer() = delete;
    ge_directx_renderer(ge_game_instance* game_instance);
    virtual ~ge_directx_renderer() override;

    ge_directx_renderer(const ge_directx_renderer&) = delete;
    ge_directx_renderer& operator=(const ge_directx_renderer&) = delete;

    // Do not delete returned pointer, always valid pointer while the renderer exists.
    ge_directx_descriptor_heap* get_rtv_descriptor_heap();
    ge_directx_descriptor_heap* get_dsv_descriptor_heap();
    ge_directx_descriptor_heap* get_cbv_srv_uav_descriptor_heap();
    ge_directx_descriptor_heap* get_sampler_descriptor_heap();

    void copy_resource(
        ID3D12Resource* src, D3D12_RESOURCE_STATES src_state, ID3D12Resource* dst, D3D12_RESOURCE_STATES dst_state);

    // Adds a new fence to the command queue and returns the value of the fence.
    UINT64 signal_fence();
    bool is_fence_completed(UINT64 fence_value);

    // Pauses the current thread and waits until all GPU commands (up to this point) are finished.
    void wait_for_gpu_to_finish_work();

    // Queues a callback that will be triggered every frame to check if a resource can be safely destroyed
    // (no longer used by the GPU).
    // The callback should return `true` if the resource was destroyed.
    void queue_resource_destruction(const std::function<bool()>& check);

    // Returns mutex that's locked while submitting a frame.
    // Can be locked externally to prevent new frame submission.
    std::mutex& get_draw_mutex();

    // Do not delete returned pointer, valid while the renderer exists.
    ID3D12Device* get_device();
    ID3D12GraphicsCommandList* get_command_list();
    D3D12MA::Allocator* get_mem_allocator();
    ge_directx_pso_manager* get_pso_manager();

  protected:
    virtual void draw_next_frame() override;
    virtual void submit_gpu_commands() override;
    virtual void on_after_new_window_created(ge_window* window) override;

  private:
    void create_video_adapter();
    std::vector<DXGI_MODE_DESC> get_supported_display_modes();
    void create_swap_chain_for_window(ge_window* window, const std::vector<DXGI_MODE_DESC>& available_display_modes);

    void wait_for_fence_value(UINT64 fence_to_wait_for);

    void set_viewport(ge_render_target* render_target);

    ComPtr<ID3D12Device> dx_device;
    ComPtr<ID3D12GraphicsCommandList> dx_command_list;
    ComPtr<ID3D12CommandQueue> dx_command_queue;
    ComPtr<IDXGIAdapter3> dx_adapter;
    ComPtr<IDXGIOutput> dx_output;
    ComPtr<IDXGIFactory4> dx_factory;
    ComPtr<ID3D12Fence> dx_fence;

    // Used for GPU resource creation.
    ComPtr<D3D12MA::Allocator> mem_allocator;

#if defined(DEBUG)
    ComPtr<ID3D12Debug> dx_debug;
    ComPtr<ID3D12InfoQueue1> dx_debug_info_queue;
#endif

    ge_directx_descriptor_heap* rtv_heap;
    ge_directx_descriptor_heap* dsv_heap;
    ge_directx_descriptor_heap* cbv_srv_uav_heap;
    ge_directx_descriptor_heap* sampler_heap;

    ge_directx_pso_manager* pso_manager;

    // Stores callbacks which we need to check every frame.
    // Each generally checks a fence value and if completed deleted a GPU resource (callback returns `true`)
    // in case the callback returned `false` there's no need to check the others (for this frame).
    std::queue<std::function<bool()>> queued_resource_destructions;

    // Locked during frame submission.
    std::mutex mtx_draw;

    std::array<ge_directx_frame_resource*, GE_RENDER_FRAMES_IN_FLIGHT_COUNT> frame_resources;
    unsigned int frame_res_idx;

    // Fence value to signal into the command queue and wait for.
    std::atomic<UINT64> next_fence_value;

    float min_depth;
    float max_depth;

    static const DXGI_FORMAT back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    static const DXGI_FORMAT depth_stencil_buffer_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Required feature level.
    static const D3D_FEATURE_LEVEL dx_feature_level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
};