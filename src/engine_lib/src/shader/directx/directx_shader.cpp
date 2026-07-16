#include <shader/directx/directx_shader.h>

std::shared_ptr<ge_shader>
ge_directx_shader::compile(ge_shader_manager* shader_manager, size_t hash) {
    TODO;
}

ge_directx_shader::ge_directx_shader(ge_shader_manager* shader_manager, size_t hash)
    : ge_shader(shader_manager, hash) {}