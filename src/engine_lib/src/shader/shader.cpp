#include <shader/shader.h>

#include <shader/shader_manager.h>

#if defined(WIN32)
#include <shader/directx/directx_shader.h>
#endif

ge_shader::ge_shader(ge_shader_manager* shader_manager, size_t hash) {
    this->shader_manager = shader_manager;
    this->hash = hash;
}

ge_shader::~ge_shader() { shader_manager->on_before_shader_destroyed(this); }

ge_shader_manager*
ge_shader::get_shader_manager() {
    return shader_manager;
}

std::shared_ptr<ge_shader>
ge_shader::compile(ge_shader_manager* shader_manager, size_t hash) {
#if defined(WIN32)
    return ge_directx_shader::compile(shader_manager, hash);
#else
    return std::make_shared<ge_shader>(shader_manager, hash);
#endif
}
