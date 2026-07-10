#include <render/directx/directx_swap_chain.h>

ge_directx_swap_chain::ge_directx_swap_chain(const ComPtr<IDXGISwapChain3>& dx_swap_chain) {
    this->dx_swap_chain = dx_swap_chain;
}
