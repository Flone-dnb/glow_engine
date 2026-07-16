#include <shader/shader_manager.h>

#include <io/log.h>
#include <functional>
#include <shader/shader.h>

ge_shader_manager::ge_shader_manager(ge_renderer* renderer) { this->renderer = renderer; }

std::shared_ptr<ge_shader>
ge_shader_manager::get_shader(const char* path_to_shader) {
    std::hash<std::string> hasher;
    size_t hash = hasher(std::string(path_to_shader));

    std::lock_guard<std::mutex> guard(mtx_shaders.first);
    auto& shaders = mtx_shaders.second;

    auto shader_it = shaders.find(hash);
    if (shader_it == shaders.end()) {
        std::shared_ptr<ge_shader> shader = ge_shader::compile(this, hash);
        if (shader == nullptr) {
            return nullptr;
        }
        shaders[hash] = shader;
        return shader;
    } else {
        return shader_it->second.lock();
    }
}

void
ge_shader_manager::on_before_shader_destroyed(ge_shader* shader) {
    std::lock_guard<std::mutex> guard(mtx_shaders.first);
    auto& shaders = mtx_shaders.second;

    auto shader_it = shaders.find(shader->hash);
    if (shader_it == shaders.end()) {
        ge_log_error("failed to find shader");
        abort();
    }

    shaders.erase(shader_it);
}

ge_renderer*
ge_shader_manager::get_renderer() {
    return renderer;
}