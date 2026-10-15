#pragma once

// The language and label cache are owned by the UI thread. Transport/storage
// diagnostics stay in English internally and are translated only for display.
// Both locales use UTF-8; no process locale or XML/schema names are changed.
#include <cstring>
#include <map>
#include <string>
#include <string_view>

namespace app_language {
enum class Language { English, Russian };
inline Language current = Language::English;
inline bool IsRussian() { return current == Language::Russian; }
inline const char* Code() { return IsRussian() ? "ru" : "en"; }
inline void Set(Language language) { current = language; }
inline bool SetFromCode(std::string_view code) {
    current = code == "ru" ? Language::Russian : Language::English;
    return code == "ru" || code == "en";
}
inline const char* Tr(const char* english, const char* russian) {
    return IsRussian() ? russian : english;
}

// ImGui hashes the suffix following ###, or the entire label otherwise.
// Preserve that exact hash, including legacy ## suffixes, across language
// switches. English labels remain byte-for-byte compatible with window.ini.
// std::map keeps returned strings/pointers stable for the application's life.
inline const char* Label(const char* english, const char* russian) {
    if (!IsRussian() || !english || !russian || english[0] == '#' || !*english)
        return english;
    static std::map<std::pair<std::string, std::string>, std::string> labels;
    const auto key = std::make_pair(std::string(english), std::string(russian));
    const auto existing = labels.find(key);
    if (existing != labels.end()) return existing->second.c_str();
    const char* suffix = std::strstr(english, "###");
    // Multiple ### sequences are legal; ImGui resets its hash at each one.
    if (suffix) {
        const char* next;
        while ((next = std::strstr(suffix + 3, "###")) != nullptr) suffix = next;
        suffix += 3;
    } else suffix = english;
    std::string visible(russian);
    if (const auto hidden = visible.find("##"); hidden != std::string::npos)
        visible.resize(hidden);
    return labels.emplace(key, visible + "###" + suffix).first->second.c_str();
}
} // namespace app_language
