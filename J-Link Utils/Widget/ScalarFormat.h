#pragma once
// Main.h defines snprintf as a Windows compatibility macro. This helper uses
// the standard bounded formatter, including when Main.h was included first.
#ifdef snprintf
#pragma push_macro("snprintf")
#undef snprintf
#define UI_SAFETY_RESTORE_SNPRINTF
#endif
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace ui_safety {

enum class ScalarKind { Integer, Floating };

// XML formats are allowed one correctly typed conversion (or literal text).
// Reject argument-consuming widths, positional arguments, pointer conversions,
// and length modifiers: the callers supply only one int or promoted double.
inline bool IsScalarFormat(const char* format, ScalarKind kind, char* conversion = nullptr) {
    if (!format) return false;
    unsigned conversions = 0;
    if (conversion) *conversion = '\0';
    for (const char* p = format; *p; ++p) {
        if (*p != '%') continue;
        ++p;
        if (*p == '%') continue;
        if (++conversions > 1) return false;
        while (*p && std::strchr("-+ #0", *p)) ++p;
        unsigned width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + static_cast<unsigned>(*p++ - '0');
            if (width > 1024) return false;
        }
        if (*p == '.') {
            ++p;
            unsigned precision = 0;
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + static_cast<unsigned>(*p++ - '0');
                if (precision > 1024) return false;
            }
        }
        // printf's optional 'l' on floating conversions still consumes double.
        if (kind == ScalarKind::Floating && *p == 'l') ++p;
        const char* allowed = kind == ScalarKind::Floating ? "aAeEfFgG" : "diuoxX";
        if (!*p || !std::strchr(allowed, *p)) return false;
        if (conversion) *conversion = *p;
    }
    return true;
}

inline const char* ScalarFormatOrDefault(const char* format, ScalarKind kind) {
    if (IsScalarFormat(format, kind)) return format;
    return kind == ScalarKind::Floating ? "%.3f" : "%d";
}

inline void FormatScalar(char* out, std::size_t capacity, const char* format, int value) {
    char conversion = '\0';
    if (!IsScalarFormat(format, ScalarKind::Integer, &conversion)) {
        std::snprintf(out, capacity, "%s", format ? format : "");
    } else if (conversion && std::strchr("uoxX", conversion)) {
        std::snprintf(out, capacity, format, static_cast<unsigned int>(value));
    } else {
        std::snprintf(out, capacity, format, value);
    }
}

inline void FormatScalar(char* out, std::size_t capacity, const char* format, double value) {
    if (!IsScalarFormat(format, ScalarKind::Floating)) {
        std::snprintf(out, capacity, "%s", format ? format : "");
    } else {
        std::snprintf(out, capacity, format, value);
    }
}

} // namespace ui_safety

#ifdef UI_SAFETY_RESTORE_SNPRINTF
#pragma pop_macro("snprintf")
#undef UI_SAFETY_RESTORE_SNPRINTF
#endif
