#if defined(WIN32)
// Hide console on Windows.
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#endif

#include "editor_instance.h"

int main() {
    ge_editor_instance* game_instance = new ge_editor_instance();

    game_instance->run_game_loop();

    delete game_instance;

    return 0;
}
