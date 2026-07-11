#if defined(WIN32)
// Hide console on Windows.
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#endif

#if defined(DEBUG) && defined(WIN32)
#include "../mem_leak_check.hpp"
#endif

#include <io/log.h>
#include "editor_instance.h"

int
main() {
#if defined(DEBUG) && defined(WIN32)
    start_mem_leak_checks();
#endif
    ge_editor_instance* game_instance = new ge_editor_instance();

    game_instance->run_game_loop();

    delete game_instance;

#if defined(DEBUG) && defined(WIN32)
    finish_mem_leak_checks();
#endif

    if (ge_log_get_warning_count_logged() > 0 || ge_log_get_error_count_logged() > 0) {
        ge_log_info("");
        if (ge_log_get_warning_count_logged() > 0) {
            ge_log_info_fmt("total WARNINGS produced: %u", ge_log_get_warning_count_logged());
        }
        if (ge_log_get_error_count_logged() > 0) {
            ge_log_info_fmt("total ERRORS produced: %u", ge_log_get_error_count_logged());
        }
    }

    return 0;
}
