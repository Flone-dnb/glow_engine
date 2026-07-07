#include <game_instance.h>

class ge_render_target;

class ge_editor_instance : public ge_game_instance {
  public:
    ge_editor_instance() = default;
    virtual ~ge_editor_instance() override = default;

    ge_editor_instance(const ge_editor_instance&) = delete;
    ge_editor_instance& operator=(const ge_editor_instance&) = delete;

  protected:
    virtual void on_game_started() override;
    virtual void on_game_finished() override;

  private:
    ge_window* main_window;
    ge_render_target* main_render_target;
};