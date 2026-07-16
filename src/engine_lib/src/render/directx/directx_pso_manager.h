#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

class ge_directx_renderer;
class ge_material;

class ge_directx_pso {
    // Only manager is allowed to create such objects.
    friend class ge_directx_pso_manager;

  public:
    // Notifies PSO manager.
    ~ge_directx_pso();

    ge_directx_pso() = delete;
    ge_directx_pso(const ge_directx_pso&) = delete;
    ge_directx_pso& operator=(const ge_directx_pso&) = delete;

  protected:
    ge_directx_pso(ge_directx_pso_manager* manager, size_t hash);

    ge_directx_pso_manager* manager;
    size_t hash;
};

// Pipeline state object (PSO) manager.
class ge_directx_pso_manager {
    // Only renderer is allowed to create this manager.
    friend class ge_directx_renderer;

    // Notifies the manager.
    friend class ge_directx_pso;

  public:
    ~ge_directx_pso_manager();

    ge_directx_pso_manager() = delete;
    ge_directx_pso_manager(const ge_directx_pso_manager&) = delete;
    ge_directx_pso_manager& operator=(const ge_directx_pso_manager&) = delete;

    // Looks for already existing (previously created) PSO or creates a new one
    // based on the parameters of the material. Note that for different material objects
    // this function can return the same PSO even if some parameters of materials are different.
    std::shared_ptr<ge_directx_pso> get_pso_for_material(ge_material* material);

  private:
    static size_t calc_pso_hash_from_material(ge_material* material);

    ge_directx_pso_manager(ge_directx_renderer* renderer);

    void on_before_pso_destructed(ge_directx_pso* pso);

    // Stores a map where key is a hash of specific material parameters.
    // It's safe to store weak pointers here because PSOs notify the manager in destructor.
    std::pair<std::mutex, std::unordered_map<size_t, std::weak_ptr<ge_directx_pso>>> mtx_psos;

    ge_directx_renderer* renderer;
};