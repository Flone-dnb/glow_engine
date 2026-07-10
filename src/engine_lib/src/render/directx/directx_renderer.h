#pragma once

#include <array>
#include <render/renderer.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <dxgi1_4.h>
using namespace Microsoft::WRL;

class ge_game_instance;
class ge_window;

// Used for submitting multiple frames without waiting for the GPU.
struct ge_directx_frame_resource {
    ComPtr<ID3D12CommandAllocator> dx_command_allocator;
    UINT64 fence;
};

// DirectX 12 renderer.
class ge_directx_renderer : public ge_renderer {
  public:
    ge_directx_renderer() = delete;
    ge_directx_renderer(ge_game_instance* game_instance);
    virtual ~ge_directx_renderer() override;

    ge_directx_renderer(const ge_directx_renderer&) = delete;
    ge_directx_renderer& operator=(const ge_directx_renderer&) = delete;

  protected:
    virtual void draw_next_frame() override;
    virtual void on_after_new_window_created(ge_window* window) override;

  private:
    void create_video_adapter();
    std::vector<DXGI_MODE_DESC> get_supported_display_modes();
    void create_swap_chain_for_window(ge_window* window, const std::vector<DXGI_MODE_DESC>& available_display_modes);

    ComPtr<ID3D12Device> dx_device;
    ComPtr<ID3D12GraphicsCommandList> dx_command_list;
    ComPtr<ID3D12CommandQueue> dx_command_queue;
    ComPtr<IDXGIAdapter3> dx_adapter;
    ComPtr<IDXGIOutput> dx_output;
    ComPtr<IDXGIFactory4> dx_factory;

#if defined(DEBUG)
    ComPtr<ID3D12Debug> dx_debug;
    ComPtr<ID3D12InfoQueue1> dx_debug_info_queue;
#endif

    std::array<ge_directx_frame_resource*, 2> frame_resources;

    /** Back buffer fill color. */
    float backBufferFillColor[4] = {0.0F, 0.0F, 0.0F, 1.0F};

    static const DXGI_FORMAT back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    static const DXGI_FORMAT depth_stencil_buffer_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Required feature level.
    static const D3D_FEATURE_LEVEL dx_feature_level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
};