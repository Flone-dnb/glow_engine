#pragma once

#include <string>

struct te_filesystem_entry {
    // Name of a file/directory.
    std::string name;

    // `true` if it's a directory.
    bool is_dir;
};

// ------------------------------------------------------------------------------------------------

bool ge_filesystem_does_path_exists(const char* path);
bool ge_filesystem_path_is_directory(const char* path);

// Recursively creates directories for the specified path (if directories did not existed before).
void ge_filesystem_ensure_dirs_exist(const char* path);
void ge_filesystem_create_directory(const char* path);

void ge_filesystem_remove_file(const char* path);
void ge_filesystem_rename_file(const char* old_path, const char* new_path);
void ge_filesystem_copy_file(const char* src, const char* dst);

// ------------------------------------------------------------------------------------------------

// Returns pointer to the first character of the filename from the specified path string.
// Also works for directories.
// Specify nullptr as `ret_len` to ignore returned string length.
// Returns nullptr if filename not found.
const char* ge_filesystem_find_filename(const char* path, bool include_extension, unsigned int* ret_len);

// ------------------------------------------------------------------------------------------------

std::string ge_filesystem_convert_path_to_absolute(const char* src);

// Converts the specified path to be relative to the `res` directory.
// Returns empty string if unable to convert.
std::string ge_filesystem_convert_path_to_relative(const char* src);

// For files returns path to the directory where the file is stored,
// for directories returns path to the parent directory.
// Returns empty if unable to find.
std::string ge_filesystem_get_parent_path(const std::string& path);

// Appends path to an existing one, adds a slash character between the paths if needed.
std::string ge_filesystem_append_path(const std::string& path, const std::string& add);

// ------------------------------------------------------------------------------------------------

// Returns all filesystem entries (files and directories) in the specified directory (not recursive).
// You must free returned array and entry names.
te_filesystem_entry* ge_filesystem_list_directory(const char* path_to_dir, unsigned int* entry_count);