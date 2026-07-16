#pragma once

#include <shader/shader.h>

class ge_directx_shader : public ge_shader {
  public:
    virtual ~ge_directx_shader() override = default;

    // Compiles a new shader.
    // Returns `nullptr` if compilation failed (see log for details).
    static std::shared_ptr<ge_shader> compile(ge_shader_manager* shader_manager, size_t hash);

    ge_directx_shader() = delete;
    ge_directx_shader(const ge_directx_shader&) = delete;
    ge_directx_shader& operator=(const ge_directx_shader&) = delete;

  protected:
    ge_directx_shader(ge_shader_manager* shader_manager, size_t hash);
};
