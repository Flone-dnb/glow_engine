#include <io/filesystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io/log.h>

#if defined(WIN32)
#define NOMINMAX
#include <direct.h>
#include <windows.h>
#include <tchar.h>
#define mkdir(dir, mode) _mkdir(dir)
#elif __linux__
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#else
#error "unsupported OS"
#endif

void
ge_filesystem_ensure_dirs_exist(const char* file_path) {
    const char pathSeparator = '/'; // we convert \\ to / on windows

    char* dir_path = (char*)malloc(strlen(file_path) + 1);

    const char* next_sep = strchr(file_path, pathSeparator);
    while (next_sep != nullptr) {
        const size_t dir_path_len = (size_t)(next_sep - file_path);
        memcpy(dir_path, file_path, dir_path_len);
        dir_path[dir_path_len] = 0;

        if (!ge_filesystem_does_path_exists(dir_path)) {
            mkdir(dir_path, 0755);
        }

        next_sep = strchr(next_sep + 1, pathSeparator);
    }

    free(dir_path);
}

void
ge_filesystem_create_directory(const char* path) {
    mkdir(path, 0755);
}

bool
ge_filesystem_does_path_exists(const char* path) {
#if defined(WIN32)
    FILE* fp = fopen(path, "r");
    if (fp == nullptr) {
        DWORD attrib = GetFileAttributes(path);
        return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
    } else {
        fclose(fp);
        return true;
    }
#elif __linux__
    struct stat buffer;
    return stat(path, &buffer) == 0;
#else
#error "unsupported OS"
#endif
}

bool
ge_filesystem_path_is_directory(const char* path) {
#if defined(WIN32)
    DWORD dwAttrib = GetFileAttributesA(path);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        ge_log_error_fmt("failed to get path attributes for %s", path);
        return false;
    }
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        return true;
    }

    return false;
#elif __linux__
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        ge_log_error_fmt("failed to get path attributes for %s", path);
        return false;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        return true;
    } else if (S_ISREG(path_stat.st_mode)) {
        return false;
    }

    return false;
#else
#error "unsupported OS"
#endif
}

void
ge_filesystem_remove_file(const char* path) {
#if defined(WIN32)
    DeleteFile(path);
#elif __linux__
    remove(path);
#else
#error "unsupported OS"
#endif
}

void
ge_filesystem_rename_file(const char* old_path, const char* new_path) {
    if (rename(old_path, new_path) != 0) {
        ge_log_error_fmt("failed to rename file from \"%s\" to \"%s\"", old_path, new_path);
        abort();
    }
}

void
ge_filesystem_copy_file(const char* src, const char* dst) {
#if defined(WIN32)
    CopyFile(src, dst, 0);
#else
    int input, output;
    if ((input = open(src, O_RDONLY)) == -1) {
        return;
    }
    if ((output = creat(dst, 0660)) == -1) {
        close(input);
        return;
    }

    struct stat file_stat = {0};
    int result = fstat(input, &file_stat);
    off_t copied = 0;
    while (result == 0 && copied < file_stat.st_size) {
        ssize_t written = sendfile(output, input, &copied, SSIZE_MAX);
        copied += written;
        if (written == -1) {
            result = -1;
        }
    }

    close(input);
    close(output);
#endif
}

const char*
ge_filesystem_find_filename(const char* path, bool include_extension, unsigned int* ret_len) {
    const size_t len = strlen(path);

    size_t idx = len;
    for (size_t i = len - 1; i > 0; i--) {
#if defined(WIN32)
        if (path[i] == '/' || path[i] == '\\') {
#else
        if (path[i] == '/') {
#endif
            idx = i + 1;
            break;
        }
    }
    if (idx >= len) {
        if (ret_len != nullptr) {
            (*ret_len) = 0;
        }
        return nullptr;
    }

    if (include_extension) {
        if (ret_len != nullptr) {
            (*ret_len) = (unsigned int)(len - idx);
        }
        return path + idx;
    }

    for (size_t i = idx; i < len; i++) {
        if (path[i] == '.') {
            if (ret_len != nullptr) {
                (*ret_len) = (unsigned int)(i - idx);
            }
            break;
        }
    }

    return path + idx;
}

std::string
ge_filesystem_convert_path_to_absolute(const char* src) {
#if defined(__linux__)
    return realpath(src, nullptr);
#elif defined(WIN32)
    return _fullpath(nullptr, src, 0);
#else
#error "unsupported OS"
#endif
}

std::string
ge_filesystem_convert_path_to_relative(const char* src) {
    // Find `res/` in the path.
    const size_t len = strlen(src);
    size_t start_pos = 0xFFFFFFFF;

#if defined(__linux__)
    if (strncmp(src, "res/", 4) == 0)
#else
    if (strncmp(src, "res\\", 4) == 0 || strncmp(src, "res/", 4) == 0)
#endif
    {
        start_pos = 4;
    } else {
        for (size_t i = 1; i < len; i++) {
#if defined(__linux__)
            if (strncmp(src + i, "/res/", 5) == 0)
#else
            if (strncmp(src + i, "\\res\\", 5) == 0 || strncmp(src + i, "/res/", 5) == 0)
#endif
            {
                start_pos = i + 5;
                break;
            }
        }
    }
    if (start_pos == 0xFFFFFFFF) {
        return std::string();
    }

    return src + start_pos;
}

std::string
ge_filesystem_get_parent_path(const std::string& path) {
    if (path.size() <= 1) {
        return nullptr;
    }

    unsigned int pos = path.size() - 1;
#if defined(WIN32)
    if (path[pos] == '\\' || path[pos] == '/')
#else
    if (path[pos] == '/')
#endif
    {
        pos -= 1;
    }

    for (; pos > 0; pos--) {
#if defined(WIN32)
        if (path[pos] == '\\' || path[pos] == '/')
#else
        if (path[pos] == '/')
#endif
        {
            break;
        }
    }
    if (pos == 0) {
        return std::string();
    }

    std::string out;
    out.resize(pos);
    memcpy((char*)out.data(), path.data(), sizeof(char) * pos);

    return out;
}

std::string filesystem_append_path(const std::string& path, const std::string& add) {
    const bool have_slash = path[path.size() - 1] == '/' || path[path.size() - 1] == '\\';

    std::string out;
    out.reserve(path.size() + !have_slash + add.size());

    out += path;
    if (!have_slash){
#if defined(WIN32)
        out += "\\";
#else
        out += "/";
#endif
    }
    out += add;

    return out;
}

te_filesystem_entry*
ge_filesystem_list_directory(const char* path_to_dir, unsigned int* entry_count) {
    (*entry_count) = 0;

#if defined(__linux__)
    // Count number of entries.
    DIR* dir = opendir(path_to_dir);
    if (dir == nullptr) {
        log_error_fmt("unable to open the directory \"%s\" (does path exist?)", path_to_dir);
        abort();
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        (*entry_count) += 1;
    }
    closedir(dir);

    if ((*entry_count) == 0) {
        return nullptr;
    }
    te_filesystem_entry* entries = malloc(sizeof(te_filesystem_entry) * (*entry_count));

    // Save entries.
    dir = opendir(path_to_dir);
    if (dir == nullptr) {
        log_error_fmt("unable to open the directory \"%s\" (does path exist?)", path_to_dir);
        abort();
    }
    unsigned int i = 0;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        entries[i].name = entry->d_name;
        entries[i].is_dir = entry->d_type == DT_DIR;

        i++;
    }
    closedir(dir);

    return entries;
#elif defined(WIN32)
    char abs_path[MAX_PATH * 2 + 2] = {0};
    _fullpath(abs_path, path_to_dir, MAX_PATH * 2);

    // Prepare path for FindFirstFile.
    {
        const size_t len = strlen(abs_path);
        if (abs_path[len - 1] == '\\' || abs_path[len - 1] == '/') {
            abs_path[len] = '*';
        } else {
            abs_path[len] = '\\';
            abs_path[len + 1] = '*';
        }
    }

    // Count entries.
    {
        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFileA(abs_path, &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            ge_log_error_fmt("unable to open the directory \"%s\" (does path exist?)", abs_path);
            abort();
        }
        (*entry_count) = 0;
        do {
            if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
                continue;
            }
            (*entry_count) += 1;
        } while (FindNextFile(hFind, &ffd) != 0);
        FindClose(hFind);
    }

    if ((*entry_count) == 0) {
        return nullptr;
    }
    te_filesystem_entry* entries = (te_filesystem_entry*)malloc(sizeof(te_filesystem_entry) * (*entry_count));

    // Save entries.
    {
        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFileA(abs_path, &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            ge_log_error_fmt("unable to open the directory \"%s\" (does path exist?)", abs_path);
            abort();
        }
        unsigned int i = 0;
        do {
            if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
                continue;
            }

            entries[i].name = ffd.cFileName;
            entries[i].is_dir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

            i += 1;
        } while (FindNextFile(hFind, &ffd) != 0);
        FindClose(hFind);
    }

    return entries;
#else
#error "unsupported OS"
#endif
}
