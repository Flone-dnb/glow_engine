#pragma once

#include <render/render_target.h>

class ge_renderer;
class ge_directx_resource;

class ge_directx_render_target : public ge_render_target {
  public:
    ge_directx_render_target(
        ge_renderer* renderer, unsigned int width, unsigned int height, const glm::vec4& clear_color);

    virtual ~ge_directx_render_target() override;

    ge_directx_render_target() = delete;
    ge_directx_render_target(const ge_directx_render_target&) = delete;
    ge_directx_render_target& operator=(const ge_directx_render_target&) = delete;

    ge_directx_resource* get_resource();

  private:
    ge_directx_resource* resource;
};