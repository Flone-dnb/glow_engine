#pragma once

class ge_renderer;
class ge_render_target;

// Base class for render-specific window swap chain.
class ge_swap_chain {
  public:
    ge_swap_chain(ge_renderer* renderer);
    virtual ~ge_swap_chain() = default;

    ge_swap_chain() = delete;
    ge_swap_chain(const ge_swap_chain&) = delete;
    ge_swap_chain& operator=(const ge_swap_chain&) = delete;

    // Copies render target into the current swap chain buffer.
    virtual void copy_from_render_target(ge_render_target* render_target) = 0;

    virtual void present() = 0;

    // Returns size of swap chain buffers.
    virtual void get_size(unsigned int& width, unsigned int& height) = 0;

    ge_renderer* get_renderer();

  private:
    ge_renderer* renderer;
};