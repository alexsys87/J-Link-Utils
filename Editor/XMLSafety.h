#pragma once
#include "tinyxml2/tinyxml2.h"

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

} // namespace ui_safety
