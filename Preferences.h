#pragma once

// UI preferences only. This C++17 header deliberately has no application/SDL dependency.
// Paths at the public boundary are UTF-8, including on Windows.
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace prefs {
namespace detail {
inline std::string Trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
inline std::string EncodeValue(const std::string& s) {
    if (s.find_first_of("\r\n") == std::string::npos && Trim(s) == s && (s.empty() || s[0] != '"')) return s;
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        default: out += c;
        }
    }
    return out + '"';
}
inline bool DecodeValue(const std::string& s, std::string& out) {
    if (s.empty() || s.front() != '"') { out = s; return true; }
    // Only our complete quoted representation is decoded; legacy bare values stay bare.
    if (s.size() < 2 || s.back() != '"') { out = s; return true; }
    out.clear();
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        char c = s[i];
        if (c == '\\') {
            if (++i + 1 >= s.size()) return false;
            switch (s[i]) {
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case '\\': c = '\\'; break;
            case '"': c = '"'; break;
            default: out = s; return true; // legacy quoted Windows path
            }
        }
        out += c;
    }
    return true;
}
inline bool ParseInt(const std::string& s, int& n) {
    if (s.empty()) return false;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), n, 10);
    return r.ec == std::errc{} && r.ptr == s.data() + s.size();
}
inline bool ParseUInt32(const std::string& s, uint32_t& n) {
    size_t start = s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ? 2 : 0;
    if (s.size() == start || s[start] == '-' || s[start] == '+') return false;
    const auto r = std::from_chars(s.data() + start, s.data() + s.size(), n, start ? 16 : 10);
    return r.ec == std::errc{} && r.ptr == s.data() + s.size();
}

struct Store {
    std::map<std::string, std::string> values;
    // Preserve the representation of untouched entries, including unknown quoted values.
    std::map<std::string, std::string> rawValues;
    std::filesystem::path filename;
    std::string error;
    bool dirty = false;
    bool writable = false;

    void Fail(const std::string& message) { error = message; }
    bool Load(const std::string& path) {
        values.clear(); rawValues.clear(); dirty = false; writable = false; error.clear();
        try {
            filename = std::filesystem::absolute(std::filesystem::u8path(path)).lexically_normal();
            std::error_code ec;
            const bool exists = std::filesystem::exists(filename, ec);
            if (ec) { Fail("Cannot inspect settings file: " + ec.message()); return false; }
            if (!exists) { writable = true; return true; }
            if (!std::filesystem::is_regular_file(filename, ec) || ec) {
                Fail("Settings path is not a readable regular file: " + path); return false;
            }
            std::ifstream file(filename, std::ios::binary);
            if (!file) { Fail("Cannot read settings file: " + path); return false; }
            std::string line, section;
            size_t lineNumber = 0;
            while (std::getline(file, line)) {
                if (++lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
                line = Trim(line);
                if (line.empty() || line[0] == ';' || line[0] == '#') continue;
                if (line.front() == '[' && line.back() == ']') {
                    section = Trim(line.substr(1, line.size() - 2)); continue;
                }
                const auto eq = line.find('=');
                if (eq == std::string::npos || Trim(line.substr(0, eq)).empty()) {
                    Fail("Malformed settings line " + std::to_string(lineNumber)); continue;
                }
                std::string value;
                if (!DecodeValue(Trim(line.substr(eq + 1)), value)) {
                    Fail("Malformed settings value on line " + std::to_string(lineNumber)); continue;
                }
                const auto key = (section.empty() ? "" : section + '.') + Trim(line.substr(0, eq));
                values[key] = value;
                rawValues[key] = Trim(line.substr(eq + 1));
            }
            if (file.bad() || (!file.eof() && file.fail())) {
                values.clear(); rawValues.clear(); Fail("Failed reading settings file: " + path); return false;
            }
            writable = true; return true;
        } catch (const std::exception& e) { Fail(std::string("Cannot load settings: ") + e.what()); return false; }
    }
    void Write(const std::string& key, const std::string& value) {
        if (key.empty() || key.find_first_of("\r\n=[]") != std::string::npos) {
            Fail("Invalid settings key"); return;
        }
        const auto it = values.find(key);
        if (it == values.end() || it->second != value) { values[key] = value; rawValues.erase(key); dirty = true; }
    }
    bool Flush() {
        if (!writable) { if (error.empty()) Fail("Settings have not been loaded safely; refusing to overwrite"); return false; }
        if (!dirty) return true;
        std::filesystem::path temp;
        try {
            std::map<std::string, std::map<std::string, std::string>> sections;
            for (const auto& entry : values) {
                const auto dot = entry.first.find_last_of('.');
                sections[dot == std::string::npos ? "" : entry.first.substr(0, dot)]
                    [dot == std::string::npos ? entry.first : entry.first.substr(dot + 1)] = entry.second;
            }
            std::ostringstream text;
            for (const auto& section : sections) {
                if (!section.first.empty()) text << '[' << section.first << "]\n";
                for (const auto& entry : section.second) {
                    const auto fullKey = (section.first.empty() ? "" : section.first + '.') + entry.first;
                    const auto raw = rawValues.find(fullKey);
                    text << entry.first << '=' << (raw == rawValues.end() ? EncodeValue(entry.second) : raw->second) << '\n';
                }
                text << '\n';
            }
            const auto bytes = text.str();
            static std::atomic<unsigned long> sequence{0};
            bool saved = false;
            for (int attempt = 0; attempt < 100; ++attempt) {
                temp = filename;
#ifdef _WIN32
                temp += L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(++sequence);
                HANDLE h = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (h == INVALID_HANDLE_VALUE) {
                    if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS) continue;
                    temp.clear(); throw std::runtime_error("cannot create temporary file (Windows error " + std::to_string(GetLastError()) + ")");
                }
                bool ok = true;
                size_t offset = 0;
                while (offset < bytes.size()) {
                    DWORD count = 0;
                    const DWORD chunk = static_cast<DWORD>((std::min)(bytes.size() - offset, size_t(1 << 20)));
                    if (!WriteFile(h, bytes.data() + offset, chunk, &count, nullptr) || !count) { ok = false; break; }
                    offset += count;
                }
                if (ok) ok = FlushFileBuffers(h) != 0;
                if (!CloseHandle(h)) ok = false;
                if (!ok) throw std::runtime_error("temporary file write/flush failed");
                if (!MoveFileExW(temp.c_str(), filename.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                    throw std::runtime_error("atomic replacement failed (Windows error " + std::to_string(GetLastError()) + ")");
#else
                temp += ".tmp." + std::to_string(getpid()) + "." + std::to_string(++sequence);
                int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
                if (fd < 0) {
                    if (errno == EEXIST) continue;
                    temp.clear(); throw std::system_error(errno, std::generic_category(), "cannot create temporary file");
                }
                bool ok = true;
                size_t offset = 0;
                while (offset < bytes.size()) {
                    const auto n = ::write(fd, bytes.data() + offset, bytes.size() - offset);
                    if (n < 0 && errno == EINTR) continue;
                    if (n <= 0) { ok = false; break; }
                    offset += static_cast<size_t>(n);
                }
                if (ok && ::fsync(fd) != 0) ok = false;
                if (::close(fd) != 0) ok = false;
                if (!ok) throw std::runtime_error("temporary file write/flush failed");
                std::filesystem::rename(temp, filename);
#endif
                saved = true; temp.clear(); break;
            }
            if (!saved) { temp.clear(); throw std::runtime_error("cannot allocate unique temporary file"); }
            dirty = false; error.clear(); return true;
        } catch (const std::exception& e) {
            if (!temp.empty()) { std::error_code ignored; std::filesystem::remove(temp, ignored); }
            Fail(std::string("Cannot save settings: ") + e.what()); return false;
        }
    }
};
inline Store& GetStore() { static Store store; return store; }
inline void Invalid(const std::string& key) { GetStore().Fail("Invalid setting: " + key + " (using default)"); }
inline std::string Hex(const std::string& s) {
    const char digits[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : s) { result += digits[c >> 4]; result += digits[c & 15]; }
    return result;
}
} // namespace detail

inline bool Load(const std::string& filePath) { return detail::GetStore().Load(filePath); }
inline bool Flush() { return detail::GetStore().Flush(); }
inline bool Dirty() { return detail::GetStore().dirty; }
inline const std::string& Error() { return detail::GetStore().error; }
inline std::string ReadString(const std::string& key, const std::string& fallback = "") {
    const auto& values = detail::GetStore().values;
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}
inline int ReadInt(const std::string& key, int fallback, int minimum, int maximum) {
    if (minimum > maximum) { detail::Invalid(key); return fallback; }
    fallback = (std::max)(minimum, (std::min)(maximum, fallback));
    const auto& values = detail::GetStore().values;
    const auto it = values.find(key);
    if (it == values.end()) return fallback;
    int n;
    if (!detail::ParseInt(it->second, n) || n < minimum || n > maximum) { detail::Invalid(key); return fallback; }
    return n;
}
inline double ReadDouble(const std::string& key, double fallback, double minimum, double maximum) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum) { detail::Invalid(key); return fallback; }
    if (!std::isfinite(fallback)) fallback = minimum;
    fallback = (std::max)(minimum, (std::min)(maximum, fallback));
    const auto& values = detail::GetStore().values;
    const auto it = values.find(key);
    if (it == values.end()) return fallback;
    std::istringstream stream(it->second);
    stream.imbue(std::locale::classic());
    double n;
    if (!(stream >> std::noskipws >> n) || !stream.eof() || !std::isfinite(n) || n < minimum || n > maximum) {
        detail::Invalid(key); return fallback;
    }
    return n;
}
inline bool ReadBool(const std::string& key, bool fallback) {
    const auto& values = detail::GetStore().values;
    const auto it = values.find(key);
    if (it == values.end()) return fallback;
    if (it->second == "1" || it->second == "true") return true;
    if (it->second == "0" || it->second == "false") return false;
    detail::Invalid(key); return fallback;
}
inline void WriteString(const std::string& key, const std::string& value) { detail::GetStore().Write(key, value); }
inline void WriteInt(const std::string& key, int value) { WriteString(key, std::to_string(value)); }
inline void WriteDouble(const std::string& key, double value) {
    if (!std::isfinite(value)) { detail::Invalid(key); return; }
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    WriteString(key, stream.str());
}
inline void WriteBool(const std::string& key, bool value) { WriteString(key, value ? "1" : "0"); }
inline std::string ScopedKey(const std::string& documentPath, const std::string& category, const std::string& identity) {
    std::string normalized = documentPath;
    try {
        std::error_code ec;
        auto path = std::filesystem::absolute(std::filesystem::u8path(documentPath), ec);
        if (!ec) {
            const auto canonical = std::filesystem::weakly_canonical(path, ec);
            normalized = (ec ? path.lexically_normal() : canonical).generic_u8string();
        }
#ifdef _WIN32
        // ASCII case-insensitivity covers usual drive letters and file names without a locale dependency.
        for (char& c : normalized) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
#endif
    } catch (...) { /* The original path remains a stable, distinct fallback identity. */ }
    return "DASHBOARD." + detail::Hex(normalized) + '_' + detail::Hex(category) + '_' + detail::Hex(identity);
}
} // namespace prefs
