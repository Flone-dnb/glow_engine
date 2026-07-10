#pragma once

// Basically a texture. Cameras render to this texture and windows can display it,
// but windows are not required to display it, instead can save this to disk or do something with it.
class ge_render_target {
  public:
    // Creates a new render target with specified width and height (in pixels).
    ge_render_target(unsigned int width, unsigned int height);

    ge_render_target() = delete;

    ge_render_target(const ge_render_target&) = delete;
    ge_render_target& operator=(const ge_render_target&) = delete;

    // Returns size of the render target (in pixels).
    unsigned int get_width();
    unsigned int get_height();

  private:
    // in pixels
    unsigned int width;
    unsigned int height;
};