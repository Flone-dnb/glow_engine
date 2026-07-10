#pragma once

#include <render/swap_chain.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <dxgi1_4.h>
using namespace Microsoft::WRL;

class ge_directx_swap_chain : public ge_swap_chain {
  public:
    ge_directx_swap_chain() = delete;
    ge_directx_swap_chain(const ComPtr<IDXGISwapChain3>& dx_swap_chain);
    virtual ~ge_directx_swap_chain() override = default;

    ge_directx_swap_chain(const ge_directx_swap_chain&) = delete;
    ge_directx_swap_chain& operator=(const ge_directx_swap_chain&) = delete;

    ComPtr<IDXGISwapChain3> dx_swap_chain;
};