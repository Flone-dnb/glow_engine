#pragma once

#include <render/renderer.h>

// Renderer that does nothing (i.e. no rendering).
class ge_dummy_renderer : public ge_renderer {
  public:
    ge_dummy_renderer() = delete;
    ge_dummy_renderer(ge_game_instance* game_instance) : ge_renderer(game_instance) {}

    virtual ~ge_dummy_renderer() override = default;

    ge_dummy_renderer(const ge_dummy_renderer&) = delete;
    ge_dummy_renderer& operator=(const ge_dummy_renderer&) = delete;

  protected:
    virtual void
    draw_next_frame() override {}
};
