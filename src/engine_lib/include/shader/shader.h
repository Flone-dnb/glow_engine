#pragma once

#include <memory>

// Base class for render-specific shaders.
class ge_shader {
    // Only shader manager is allowed to create new shaders.
    friend class ge_shader_manager;

  public:
    virtual ~ge_shader();

    ge_shader() = delete;
    ge_shader(const ge_shader&) = delete;
    ge_shader& operator=(const ge_shader&) = delete;

    ge_shader_manager* get_shader_manager();

  protected:
    // Compiles a new render-specific shader.
    // Returns `nullptr` if compilation failed (see log for details).
    static std::shared_ptr<ge_shader> compile(ge_shader_manager* shader_manager, size_t hash);

    // Used internally.
    ge_shader(ge_shader_manager* shader_manager, size_t hash);

    ge_shader_manager* shader_manager;

    // hash from path to shader
    size_t hash;
};