#include <render/directx/directx_pso_manager.h>

#include <string>
#include <functional>
#include <material/material.h>
#include <io/log.h>

ge_directx_pso::ge_directx_pso(ge_directx_pso_manager* manager, size_t hash) {
    this->manager = manager;
    this->hash = hash;
}
ge_directx_pso::~ge_directx_pso() { manager->on_before_pso_destructed(this); }

size_t
ge_directx_pso_manager::calc_pso_hash_from_material(ge_material* material) {
    std::string src;
    src += material->get_path_to_vertex_shader();
    src += '\n';
    src += material->get_path_to_pixel_shader();
    src += '\n';

    std::hash<std::string> hasher;
    return hasher(src);
}

ge_directx_pso_manager::ge_directx_pso_manager(ge_directx_renderer* renderer) { this->renderer = renderer; }

ge_directx_pso_manager::~ge_directx_pso_manager() {
    std::lock_guard<std::mutex> guard(mtx_psos.first);
    auto& psos = mtx_psos.second;

    if (!psos.empty()) {
        ge_log_error("PSO manager is being destroyed but there are still some PSOs registered");
        abort();
    }
}

void
ge_directx_pso_manager::on_before_pso_destructed(ge_directx_pso* pso) {
    std::lock_guard<std::mutex> guard(mtx_psos.first);
    auto& psos = mtx_psos.second;

    auto pso_it = psos.find(pso->hash);
    if (pso_it == psos.end()) {
        ge_log_error(
            "a PSO notified the manager about destruction but the manager cannot find this PSO in the database");
        abort();
    }

    psos.erase(pso_it);
}

std::shared_ptr<ge_directx_pso>
ge_directx_pso_manager::get_pso_for_material(ge_material* material) {
    std::lock_guard<std::mutex> guard(mtx_psos.first);
    auto& psos = mtx_psos.second;

    auto pso_it = psos.find(calc_pso_hash_from_material(material));
    if (pso_it == psos.end()) {
        TODO; // create new
    }
    else{
        return pso_it->second.lock();
    }
}