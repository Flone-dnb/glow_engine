#include <render/directx/directx_swap_chain.h>

#include <io/log.h>
#include <render/directx/directx_resource.h>
#include <render/directx/directx_render_target.h>
#include <render/directx/directx_renderer.h>

ge_directx_swap_chain::ge_directx_swap_chain(ge_renderer* renderer, const ComPtr<IDXGISwapChain3>& dx_swap_chain)
    : ge_swap_chain(renderer) {
    this->dx_swap_chain = dx_swap_chain;
}

ge_directx_swap_chain::~ge_directx_swap_chain() {
    ((ge_directx_renderer*)get_renderer())->wait_for_gpu_to_finish_work();
    dx_swap_chain = nullptr;
}

void
ge_directx_swap_chain::copy_from_render_target(ge_render_target* render_target) {
    ge_directx_renderer* renderer = (ge_directx_renderer*) get_renderer();
    ID3D12Resource* src = ((ge_directx_render_target*)render_target)->get_resource()->get_dx_resource();

    ComPtr<ID3D12Resource> dst;
    dx_swap_chain->GetBuffer(dx_swap_chain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&dst));

    renderer->copy_resource(src, D3D12_RESOURCE_STATE_RENDER_TARGET, dst.Get(), D3D12_RESOURCE_STATE_PRESENT);
}

void
ge_directx_swap_chain::present() {
    HRESULT result = dx_swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING); // no vsync
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }
}

void
ge_directx_swap_chain::get_size(unsigned int& width, unsigned int& height) {
    DXGI_SWAP_CHAIN_DESC desc;
    HRESULT result = dx_swap_chain->GetDesc(&desc);
    if (FAILED(result)) {
        ge_log_error(hresult_to_string(result).c_str());
        abort();
    }

    width = desc.BufferDesc.Width;
    height = desc.BufferDesc.Height;
}
