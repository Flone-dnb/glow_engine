#pragma once

class ge_renderer;

// Basically a texture. Cameras render to this texture and windows can display it,
// but windows are not required to display it, instead can save this to disk or do something with it.
class ge_render_target {
  public:
    // Creates a new render target with specified width and height (in pixels).
    static ge_render_target* create(ge_renderer* renderer, unsigned int width, unsigned int height);

    virtual ~ge_render_target() = default;

    ge_render_target() = delete;
    ge_render_target(const ge_render_target&) = delete;
    ge_render_target& operator=(const ge_render_target&) = delete;

    // Returns size of the render target (in pixels).
    unsigned int get_width();
    unsigned int get_height();

    void get_clear_color(float clear[4]);

  protected:
    ge_render_target(unsigned int width, unsigned int height);

  private:
    // in pixels
    unsigned int width;
    unsigned int height;

    float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};