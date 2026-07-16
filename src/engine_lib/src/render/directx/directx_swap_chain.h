#pragma once

#include <vector>
#include <render/swap_chain.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <dxgi1_4.h>
using namespace Microsoft::WRL;

class ge_renderer;

class ge_directx_swap_chain : public ge_swap_chain {
  public:
    ge_directx_swap_chain(ge_renderer* renderer, const ComPtr<IDXGISwapChain3>& dx_swap_chain);
    virtual ~ge_directx_swap_chain() override;

    ge_directx_swap_chain() = delete;
    ge_directx_swap_chain(const ge_directx_swap_chain&) = delete;
    ge_directx_swap_chain& operator=(const ge_directx_swap_chain&) = delete;

    virtual void copy_from_render_target(ge_render_target* render_target) override;
    virtual void present() override;
    virtual void get_size(unsigned int& width, unsigned int& height) override;

  private:
    ComPtr<IDXGISwapChain3> dx_swap_chain;
};