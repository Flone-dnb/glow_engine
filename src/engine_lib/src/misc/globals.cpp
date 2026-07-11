#include <misc/globals.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <io/log.h>

#if defined(WIN32)
#define NOMINMAX
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <time.h>
#include <errno.h>
#else
#error "unsupported OS"
#endif

static unsigned int max_app_name_len = 64;
static char cached_app_name[64] = {0};

const char*
ge_get_app_name(void) {
    if (cached_app_name[0] == 0) {
        char buffer[1024] = {0};

        // Get full path.
#if defined(WIN32)
        if (GetModuleFileNameA(NULL, buffer, 1024) == 1024) {
            ge_log_error("failed to get path to the application");
            abort();
        }
#elif __linux__
        if (readlink("/proc/self/exe", &buffer[0], 1024) == -1) {
            ge_log_error("failed to get path to the application");
            abort();
        }
#else
#error "unsupported OS"
#endif

        // Find last slash in the path.
        const size_t path_len = strlen(buffer);
        size_t last_slash_pos = path_len;
        for (size_t i = path_len - 1; i > 0; i--) {
            if (buffer[i] == '/' || buffer[i] == '\\') {
                last_slash_pos = i;
                break;
            }
        }
        if (last_slash_pos == path_len) {
            ge_log_error("unable to extract application name from the application path");
            abort();
        }

        // Save app name.
        for (size_t src = last_slash_pos + 1, dst = 0; src < path_len && dst < max_app_name_len; src++, dst++) {
#if defined(WIN32)
            if (buffer[src] == '.') {
                // don't copy ".exe"
                break;
            }
#endif
            cached_app_name[dst] = buffer[src];
        }
    }

    return &cached_app_name[0];
}

glm::vec3
ge_get_world_forward() {
    return glm::vec3(0.0f, 0.0f, 1.0f);
}

glm::vec3
ge_get_world_right() {
    return glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3
ge_get_world_up() {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

void
ge_sleep(float ms) {
#if defined(WIN32)
    timeBeginPeriod(1);
    Sleep((unsigned long)ms);
    timeEndPeriod(1);
#else
    long ns = (long)((double)ms * 1000000.0);

    struct timespec requested;
    struct timespec remaining;
    requested.tv_sec = 0;
    requested.tv_nsec = ns;
    while (nanosleep(&requested, &remaining) == -1) {
        if (errno == EINTR) {
            requested = remaining;
        } else {
            break;
        }
    }
#endif
}
