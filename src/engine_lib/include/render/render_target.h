#pragma once

// Basically a texture. Cameras render to this texture and windows can display it,
// but windows are not required to display it, instead can save this to disk or do something with it.
class ge_render_target {
  public:
    ge_render_target() = default;

    ge_render_target(const ge_render_target&) = delete;
    ge_render_target& operator=(const ge_render_target&) = delete;
};