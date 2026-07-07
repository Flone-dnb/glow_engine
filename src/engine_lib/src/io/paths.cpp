#include <io/paths.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <io/log.h>
#include <io/filesystem.h>
#include <misc/globals.h>

#if defined(WIN32)
#define NOMINMAX
#include <Windows.h>
#include <KnownFolders.h>
#include <Shlobj.h>
#endif

static char cached_path_to_config_dir[2048] = {0};
static char cached_path_to_log_file[2048] = {0};

const char*
ge_get_config_dir(void) {
    if (cached_path_to_config_dir[0] == 0) {
#if defined(WIN32)
        PWSTR path_tmp = NULL;
        const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path_tmp);
        if (result != S_OK) {
            CoTaskMemFree(path_tmp);
            ge_log_error("failed to query config dir path");
            abort();
        }

        // Copy path and replace slashes.
        char path_buff[256] = {0};
        const size_t path_len = wcslen(path_tmp);
        if (path_len > 256) {
            ge_log_error("path to AppData folder is too long");
            abort();
        }
        for (size_t i = 0; i < path_len; i++) {
            if (path_tmp[i] == '\\') {
                path_buff[i] = '/';
            } else {
                path_buff[i] = (char)path_tmp[i];
            }
        }
        CoTaskMemFree(path_tmp);

        sprintf(cached_path_to_config_dir, "%s/glow_engine/%s/config/", &path_buff[0], ge_get_app_name());
#elif __linux__

#if defined(__aarch64__)
        // On ARM64 linux devices I've decided to store configs near the binary so that it will be easier to find them.
        sprintf(cached_path_to_config_dir, "config/");
#else
        char* home_path = getenv("HOME");
        if (home_path == NULL) {
            ge_log_error("unable to query environment variable HOME");
            abort();
        }
        sprintf(cached_path_to_config_dir, "%s/.config/glow_engine/%s/config/", home_path, ge_get_app_name());
#endif

#else
#error "unsupported OS"
#endif
    }

    char* path = &cached_path_to_config_dir[0];
    ge_filesystem_ensure_dirs_exist(path);
    return path;
}

const char*
ge_get_log_file_path(void) {
    if (cached_path_to_log_file[0] == 0) {
#if defined(WIN32)
        PWSTR path_tmp = NULL;
        const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path_tmp);
        if (result != S_OK) {
            CoTaskMemFree(path_tmp);
            ge_log_error("failed to query config dir path");
            abort();
        }

        // Copy path and replace slashes.
        char path_buff[256] = {0};
        const size_t path_len = wcslen(path_tmp);
        if (path_len > 256) {
            ge_log_error("path to AppData folder is too long");
            abort();
        }
        for (size_t i = 0; i < path_len; i++) {
            if (path_tmp[i] == '\\') {
                path_buff[i] = '/';
            } else {
                path_buff[i] = (char)path_tmp[i];
            }
        }
        CoTaskMemFree(path_tmp);

        sprintf(cached_path_to_log_file, "%s/glow_engine/%s/log.txt", &path_buff[0], ge_get_app_name());
#elif __linux__

#if defined(__aarch64__)
        // On ARM64 linux devices I've decided to store logs near the binary so that it will be easier to find them.
        sprintf(cached_path_to_log_file, "log.txt");
#else
        char* home_path = getenv("HOME");
        if (home_path == NULL) {
            ge_log_error("unable to query environment variable HOME");
            abort();
        }
        sprintf(cached_path_to_log_file, "%s/.config/glow_engine/%s/log.txt", home_path, ge_get_app_name());
#endif

#else
#error "unsupported OS"
#endif
    }

    char* path = &cached_path_to_log_file[0];
    ge_filesystem_ensure_dirs_exist(path);
    return path;
}