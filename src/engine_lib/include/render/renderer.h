#pragma once

#include <vector>

class ge_game_instance;
class ge_camera_node;
class ge_window;
class ge_mesh_renderer;

// The total number of frames that can be submitted to the GPU without waiting
// for the GPU to catch up.
#define GE_RENDER_FRAMES_IN_FLIGHT_COUNT 2

// Base class for implementing a renderer.
class ge_renderer {
    // Game instance creates renderer.
    friend class ge_game_instance;

    // Spawned cameras with render targets themselves for rendering.
    friend class ge_camera_node;

  public:
    virtual ~ge_renderer();

    ge_renderer(const ge_renderer&) = delete;
    ge_renderer& operator=(const ge_renderer&) = delete;

    // Returns always valid pointer, valid while the renderer exists.
    ge_game_instance* get_game_instance();
    ge_mesh_renderer* get_mesh_renderer();

  protected:
    // Use to create a new renderer.
    static ge_renderer* create(ge_game_instance* game_instance);
    ge_renderer() = delete;
    ge_renderer(ge_game_instance* game_instance); // <- used internally

    // Returns all cameras registered for rendering.
    const std::vector<ge_camera_node*>& get_registered_cameras() const;

    virtual void draw_next_frame() = 0;

    // Called after @ref draw_next_frame and after windows record present commands.
    virtual void submit_gpu_commands() = 0;

    virtual void on_after_new_window_created(ge_window* window) {};

  private:
    // (Un)registers the camera to be rendered to its render target.
    void register_camera(ge_camera_node* camera);
    void unregister_camera(ge_camera_node* camera);

    // Cameras registered for rendering.
    std::vector<ge_camera_node*> registered_cameras;

    ge_game_instance* game_instance;
    ge_mesh_renderer* mesh_renderer;
};
