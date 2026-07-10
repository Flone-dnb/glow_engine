#include "directx_renderer.h"

#include <algorithm>
#include <io/log.h>
#include <misc/string_funcs.h>
#include <game_instance.h>
#include <render/directx/directx_swap_chain.h>
#include <window.h>
#include <dxgidebug.h>
#include <SDL3/SDL_video.h>
#
static void
get_screen_refresh_rate(int& numerator, int& denominator) {
    int display_count;
    SDL_DisplayID* displays = SDL_GetDisplays(&display_count);
    if (display_count == 0) {
        ge_log_error("unable to get display info");
        abort();
    }

    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[0]);
    numerator = mode->refresh_rate_numerator;
    denominator = mode->refresh_rate_denominator;

    SDL_free(displays);
}

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

// D3d Debug layer callback.
#if defined(DEBUG)
void
dx_debug_layer_msg_callback(
    D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR msg, void* context) {
    ge_log_error_fmt("[debug layer] %s", msg);
}
#endif

ge_directx_renderer::ge_directx_renderer(ge_game_instance* game_instance) : ge_renderer(game_instance) {
    // Enable debug layer in DEBUG mode.
    DWORD debugFactoryFlags = 0;
#if defined(DEBUG)
    {
        dx_debug = nullptr;
        HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&dx_debug));
        if (FAILED(result)) {
            ge_log_warn_fmt("unable to setup directx debug layer: %s", hresult_to_string(result).c_str());
        } else {
            dx_debug->EnableDebugLayer();

            ComPtr<IDXGIInfoQueue> dx_info_queue;
            result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(dx_info_queue.GetAddressOf()));
            if (FAILED(result)) {
                ge_log_error(hresult_to_string(result).c_str());
                abort();
            }

            dx_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, 1);
            dx_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, 1);
            dx_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, 1);

            debugFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            ge_log_info("D3D debug layer enabled");
        }
    }
#endif

    // Create DXGI factory.
    HRESULT result = CreateDXGIFactory2(debugFactoryFlags, IID_PPV_ARGS(&dx_factory));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    create_video_adapter();

    // Create output adapter.
    result = dx_adapter->EnumOutputs(0, dx_output.GetAddressOf());
    if (result == DXGI_ERROR_NOT_FOUND) {
        ge_log_error("no output adapter was found");
        abort();
    }
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    // Create device.
    result = D3D12CreateDevice(dx_adapter.Get(), dx_feature_level, IID_PPV_ARGS(&dx_device));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

#if defined(DEBUG)
    if (dx_debug != nullptr) {
        // Create debug message queue for message callback.
        // Apparently the ID3D12InfoQueue1 interface to register message callback is only
        // available on Windows 11 so we should just log the event here using the `info` category.
        result = dx_device->QueryInterface(IID_PPV_ARGS(&dx_debug_info_queue));
        if (FAILED(result)) {
            ge_log_info("ID3D12InfoQueue1 does not seem to be available on this system, failed to query the interface");
        } else {
            // Register debug message callback.
            DWORD unregisterCookie = 0;
            result = dx_debug_info_queue->RegisterMessageCallback(
                dx_debug_layer_msg_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &unregisterCookie);
            if (FAILED(result)) {
                ge_log_error(hresult_to_string(result).c_str());
                abort();
            }
        }
    }
#endif

    // Create command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    result = dx_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&dx_command_queue));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    // Create frame resources.
    for (ge_directx_frame_resource*& res : frame_resources) {
        res = new ge_directx_frame_resource();

        res->fence = 0;

        result = dx_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(res->dx_command_allocator.GetAddressOf()));
        if (FAILED(result)) {
            ge_log_error(hresult_to_string(result).c_str());
            abort();
        }
    }

    // Create command list.
    result = dx_device->CreateCommandList(
        0, // Create list for only one GPU. See pDevice->GetNodeCount().
        D3D12_COMMAND_LIST_TYPE_DIRECT, frame_resources[0]->dx_command_allocator.Get(), nullptr,
        IID_PPV_ARGS(dx_command_list.GetAddressOf()));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
    dx_command_list->Close(); // start in closed state because on frame draw we do reset (switching closed->open state)

    // Create swap chain.
    std::vector<DXGI_MODE_DESC> display_modes = get_supported_display_modes();
    for (ge_window* window : get_game_instance()->get_windows()) {
        create_swap_chain_for_window(window, display_modes);
    }
}

ge_directx_renderer::~ge_directx_renderer() {
    for (ge_directx_frame_resource* res : frame_resources) {
        delete res;
    }
}

void
ge_directx_renderer::draw_next_frame() {}

void
ge_directx_renderer::on_after_new_window_created(ge_window* window) {
    std::vector<DXGI_MODE_DESC> display_modes = get_supported_display_modes();
    create_swap_chain_for_window(window, display_modes);
}

std::vector<DXGI_MODE_DESC>
ge_directx_renderer::get_supported_display_modes() {
    int refresh_rate_numerator, refresh_rate_denominator;
    get_screen_refresh_rate(refresh_rate_numerator, refresh_rate_denominator);

    UINT display_mode_count;
    HRESULT result = dx_output->GetDisplayModeList(back_buffer_format, 0, &display_mode_count, nullptr);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
    std::vector<DXGI_MODE_DESC> display_modes(display_mode_count);
    result = dx_output->GetDisplayModeList(back_buffer_format, 0, &display_mode_count, display_modes.data());
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    std::vector<DXGI_MODE_DESC> filtered_display_modes;
    filtered_display_modes.reserve(display_mode_count);
    for (const DXGI_MODE_DESC& mode : display_modes) {
        if (mode.Scaling != DXGI_MODE_SCALING_UNSPECIFIED) {
            continue;
        }
        if (mode.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE) {
            continue;
        }
        if (refresh_rate_numerator != mode.RefreshRate.Numerator) {
            continue;
        }
        if (refresh_rate_denominator != mode.RefreshRate.Denominator) {
            continue;
        }
        filtered_display_modes.push_back(mode);
    }
    if (filtered_display_modes.empty()) {
        ge_log_error("unable to find a single suitable display mode");
        abort();
    }

    return filtered_display_modes;
}

void
ge_directx_renderer::create_video_adapter() {
    // Prepare struct to store GPU info.
    struct GpuInfo {
        ComPtr<IDXGIAdapter3> adapter;
        DXGI_ADAPTER_DESC1 desc;
    };
    std::vector<GpuInfo> adapters;

    // Get information about the GPUs.
    for (UINT adapter_idx = 0;; adapter_idx++) {
        ComPtr<IDXGIAdapter3> test_adapter;

        if (dx_factory->EnumAdapters(adapter_idx, reinterpret_cast<IDXGIAdapter**>(test_adapter.GetAddressOf()))
            == DXGI_ERROR_NOT_FOUND) {
            // No more adapters to enumerate.
            break;
        }

        // Check if the adapter supports used D3D version, but don't create the actual device yet.
        HRESULT result = D3D12CreateDevice(test_adapter.Get(), dx_feature_level, _uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(result)) {
            // Found supported adapter.

            GpuInfo info;
            result = test_adapter->GetDesc1(&info.desc);
            if (FAILED(result)) {
                ge_log_error(hresult_to_string(result).c_str());
                abort();
            }
            info.adapter = test_adapter;
            adapters.push_back(std::move(info));
        }
    }

    if (adapters.empty()) {
        ge_log_error("could not find a GPU that supports required DirectX feature level");
        abort();
    }

    // Prepare a struct for GPU suitability score.
    struct GpuScore {
        ComPtr<IDXGIAdapter3> adapter;
        size_t score = 0;
        std::string name;
    };
    std::vector<GpuScore> scores;

    // Rate all GPUs.
    for (const auto& info : adapters) {
        GpuScore score;

        score.score = 0;

        if ((info.desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
            // Skip software adapters.
            continue;
        }
        score.score += info.desc.DedicatedVideoMemory;
        score.score += info.desc.SharedSystemMemory / 1024 / 1024; // divide because it's less important

        if (score.score == 0) {
            // Skip not suitable GPUs.
            continue;
        }

        score.name = wchar_to_char(info.desc.Description);
        score.adapter = info.adapter;

        scores.push_back(std::move(score));
    }

    if (scores.empty()) {
        ge_log_error("unable to find a suitable GPU");
        abort();
    }

    if (scores.size() > 1) {
        std::sort(scores.begin(), scores.end(), [](const GpuScore& scoreA, const GpuScore& scoreB) -> bool {
            return scoreA.score > scoreB.score;
        });
        ge_log_info("found multiple suitable GPUs:");
        for (unsigned int i = 0; i < scores.size(); i++) {
            ge_log_info_fmt("%u. %s (rating: %zu)", i, scores[i].name.c_str(), scores[i].score);
        }
    }

    ge_log_info_fmt("using the following GPU: %s", scores[0].name.c_str());

    dx_adapter = scores[0].adapter;
}

void
ge_directx_renderer::create_swap_chain_for_window(
    ge_window* window, const std::vector<DXGI_MODE_DESC>& available_display_modes) {
    // TODO: just pick the highest resolution for now (until we have a properly configurable render settings).
    const DXGI_MODE_DESC& display_mode = available_display_modes[available_display_modes.size() - 1];

    DXGI_SWAP_CHAIN_DESC1 desc;
    desc.Width = display_mode.Width;
    desc.Height = display_mode.Height;
    desc.Format = back_buffer_format;
    desc.Stereo = 0;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // D3D12 apps must use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD for
    // better performance (from the docs).
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    // Allow tearing because we don't use VSync.
    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    // Flip model swapchains (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD)
    // do not support multisampling.
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;

    int refresh_rate_numerator, refresh_rate_denominator;
    get_screen_refresh_rate(refresh_rate_numerator, refresh_rate_denominator);

    DXGI_RATIONAL refreshRateData;
    refreshRateData.Numerator = refresh_rate_numerator;
    refreshRateData.Denominator = refresh_rate_denominator;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
    fullscreenDesc.RefreshRate = refreshRateData;
    fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    fullscreenDesc.Windowed = 1;

    ComPtr<IDXGISwapChain3> created_swap_chain;
    HRESULT result = dx_factory->CreateSwapChainForHwnd(
        dx_command_queue.Get(), window->get_win32_handle(), &desc, &fullscreenDesc, dx_output.Get(),
        reinterpret_cast<IDXGISwapChain1**>(created_swap_chain.GetAddressOf()));
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    window->get_swap_chain() = new ge_directx_swap_chain(created_swap_chain);

    ge_log_info_fmt("created a swap chain with display mode %ux%u", display_mode.Width, display_mode.Height);

    // Disable Alt + Enter in order to avoid switching to fullscreen exclusive mode
    // because we don't support it and use windowed fullscreen instead.
    result = dx_factory->MakeWindowAssociation(window->get_win32_handle(), DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
}
