#pragma once
#include "tinyxml2/tinyxml2.h"
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>

namespace ui_safety {

inline const char* XmlChildText(tinyxml2::XMLElement* element, const char* name,
                               const char* fallback = "") {
    if (!element) return fallback;
    tinyxml2::XMLElement* child = element->FirstChildElement(name);
    const char* text = child ? child->GetText() : nullptr;
    return text ? text : fallback;
}

// The selection may point into a deleted document. Compare addresses without
// dereferencing it, traversing only nodes belonging to the live document.
inline bool ContainsXMLNode(const tinyxml2::XMLNode* root, const tinyxml2::XMLNode* target) {
    if (!root) return false;
    if (root == target) return true;
    for (const tinyxml2::XMLNode* child = root->FirstChild(); child; child = child->NextSibling()) {
        if (ContainsXMLNode(child, target)) return true;
    }
    return false;
}

// Target address as the XML loader reads it: "#20000000" is hexadecimal, otherwise the C
// rules apply ("0x" hexadecimal, a leading 0 octal, decimal). Surrounding spaces are allowed;
// negative, out-of-range and malformed values are rejected.
inline bool ParseAddressText(const char* text, uint32_t* out) {
    if (!text) return false;
    while (std::isspace(static_cast<unsigned char>(*text))) ++text;
    int base = 0;
    if (*text == '#') { ++text; base = 16; }
    if (*text == '-' || *text == '+' || std::isspace(static_cast<unsigned char>(*text))) return false;
    errno = 0;
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, base);
    if (end == text || errno == ERANGE || value > UINT32_MAX) return false;
    while (std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (*end != '\0') return false;
    if (out) *out = static_cast<uint32_t>(value);
    return true;
}

} // namespace ui_safety
