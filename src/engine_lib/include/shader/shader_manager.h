#pragma once

#include <mutex>
#include <string>
#include <memory>
#include <unordered_map>

class ge_renderer;
class ge_shader;

class ge_shader_manager {
    // Only renderer is supposed to create this manager.
    friend class ge_renderer;

    // Notifies the manager.
    friend class ge_shader;

  public:
    ge_shader_manager() = delete;
    ge_shader_manager(const ge_shader_manager&) = delete;
    ge_shader_manager& operator=(const ge_shader_manager&) = delete;

    // Compiles a new shader or retrieves one from the cache.
    // Returns `nullptr` if the compilation failed.
    std::shared_ptr<ge_shader> get_shader(const char* path_to_shader);

    ge_renderer* get_renderer();

  private:
    ge_shader_manager(ge_renderer* renderer);

    void on_before_shader_destroyed(ge_shader* shader);

    // Map where key is hash of path to shader.
    // It's safe to store weak ptr here because the shader notifies the manager in destructor.
    std::pair<std::mutex, std::unordered_map<size_t, std::weak_ptr<ge_shader>>> mtx_shaders;

    ge_renderer* renderer;
};