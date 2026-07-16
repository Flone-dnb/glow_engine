#include <render/swap_chain.h>

ge_swap_chain::ge_swap_chain(ge_renderer* renderer) { this->renderer = renderer; }

ge_renderer*
ge_swap_chain::get_renderer() {
    return renderer;
}