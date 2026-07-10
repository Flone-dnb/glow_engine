#pragma once

// Base class for render-specific window swap chain.
class ge_swap_chain {
  public:
    ge_swap_chain() = default;
    virtual ~ge_swap_chain() = default;

    ge_swap_chain(const ge_swap_chain&) = delete;
    ge_swap_chain& operator=(const ge_swap_chain&) = delete;
};