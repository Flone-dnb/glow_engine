#include <misc/string_funcs.h>

#include <stdlib.h>
#include <string.h>
#include <io/log.h>
#include <limits.h>

std::string
wchar_to_char(const wchar_t* src) {
    const size_t len = wcstombs(NULL, src, 0);
    if (len == (size_t)-1) {
        ge_log_error("failed to convert wchar* to char*");
        abort();
    }

    std::string dst;
    dst.resize(len);
    wcstombs((char*)dst.data(), src, len + 1);

    return dst;
}

wchar_t*
wchar_from_char(const char* src, unsigned int* dst_strlen) {
    const size_t len = mbstowcs(NULL, src, 0);
    if (len == (size_t)-1) {
        ge_log_error("failed to convert char* to wchar*");
        abort();
    }

    wchar_t* dst = (wchar_t*)malloc(sizeof(wchar_t) * (len + 1));
    mbstowcs(dst, src, len + 1);

    if (len > 0xffffffff) {
        ge_log_error("failed to convert wchar* to char* - text is too long");
        abort();
    }
    (*dst_strlen) = (unsigned int)len;

    return dst;
}

float
convert_string_to_float(const char* text, char** end) {
    if (text == nullptr) {
        (*end) = (char*)text;
        return 0.0f;
    }

    char* curr = (char*)text;
    float out = 0.0f;
    float div = 1;
    bool after_dot = false;

    if (*curr == 0) {
        (*end) = curr - 1;
        return out;
    }

    if ((*curr) == '-') {
        // We will negate later.
        curr++;
    }

    while (*curr != 0) {
        if ((*curr) >= '0' && (*curr) <= '9') {
            if (!after_dot) {
                out *= 10.0f;
                out += (float)((*curr) - '0');
            } else {
                div *= 10;
                out += (float)((*curr) - '0') / div;
            }
        } else if ((*curr) == '.' || (*curr) == ',') {
            after_dot = true;
        } else {
            break;
        }

        curr++;
    }

    if ((*text) == '-') {
        out *= -1.0f;
    }

    (*end) = curr - 1;
    return out;
}
