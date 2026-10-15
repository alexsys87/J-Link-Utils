// Test-only replacements for the MSVC library functions some widgets call.
#pragma once
#ifndef _WIN32
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>

template <std::size_t N>
inline int strncpy_s(char (&dest)[N], const char* src, std::size_t count) {
    std::size_t n = count < N - 1 ? count : N - 1, i = 0;
    for (; i < n && src[i]; ++i) dest[i] = src[i];
    dest[i] = '\0';
    return 0;
}
#define sscanf_s sscanf
typedef int errno_t;
inline errno_t fopen_s(FILE** file, const char* path, const char* mode) {
    *file = std::fopen(path, mode);
    return *file ? 0 : errno;
}
#endif
