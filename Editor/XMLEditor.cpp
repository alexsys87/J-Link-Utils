// XMLEditor.cpp
#include "XMLEditor.h"
#include "Localization.h"
#include "Main.h"
#include "imgui.h"
#include "tinyxml2/tinyxml2.h"
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstring>

#include "WidgetTemplateFactory.h"
#include "XMLSafety.h"

namespace wtf = WidgetTemplateFactory;
using tinyxml2::XMLElement;
using tinyxml2::XMLNode;
using tinyxml2::XMLUtil;

// ------------------------------------------------------------------
// Value guessing for elements the widget table does not describe
// (IOMap entries, misspelled or foreign tags)
// ------------------------------------------------------------------
static bool ParseColor(const char* str, ImVec4& outColor) {
    float rgba[4];
    if (!wtf::ParseColor(str, rgba)) return false;
    outColor = ImVec4(rgba[0], rgba[1], rgba[2], rgba[3]);
    return true;
}

static bool IsBooleanString(const char* str, bool& outValue) {
    if (!str) return false;
    std::string lower;
    for (const char* p = str; *p; ++p) lower.push_back((char)std::tolower(static_cast<unsigned char>(*p)));
    if (lower == "true") {
        outValue = true;
        return true;
    }
    if (lower == "false") {
        outValue = false;
        return true;
    }
    return false;
}

static bool IsNumberString(const char* str, bool& isFloat) {
    if (!str || *str == '\0') return false;
    char* endptr;
    errno = 0;
    float f = strtof(str, &endptr);
    if (errno == ERANGE || !std::isfinite(f)) return false;
    if (endptr == str) return false; // not a number
    // only whitespace may follow the number
    while (*endptr) {
        if (!std::isspace(static_cast<unsigned char>(*endptr))) return false;
        endptr++;
    }
    // a dot or an exponent means float
    isFloat = (strchr(str, '.') != nullptr || strchr(str, 'e') != nullptr || strchr(str, 'E') != nullptr);
    if (!isFloat) {
        errno = 0;
        long value = strtol(str, &endptr, 10);
        if (errno == ERANGE || endptr == str || value < INT_MIN || value > INT_MAX) return false;
        while (*endptr && std::isspace(static_cast<unsigned char>(*endptr))) ++endptr;
        if (*endptr) return false;
    }
    return true;
}

// ------------------------------------------------------------------
// Editor state
// ------------------------------------------------------------------
static XMLNode* g_selectedNode = nullptr;
static char g_newAttrName[128] = "";
static char g_newAttrValue[128] = "";
static char g_widgetFilter[64] = "";

void ResetXMLEditorState() {
    g_selectedNode = nullptr;
    g_newAttrName[0] = '\0';
    g_newAttrValue[0] = '\0';
    g_widgetFilter[0] = '\0';
}

static const ImVec4 kWarningColor(1.0f, 0.55f, 0.3f, 1.0f);

static bool HasChildElements(const XMLElement* e) {
    return e && e->FirstChildElement() != nullptr;
}

static int CountChildElements(const XMLElement* e) {
    int n = 0;
    for (const XMLElement* c = e ? e->FirstChildElement() : nullptr; c; c = c->NextSiblingElement()) ++n;
    return n;
}

static XMLElement* ParentElement(XMLNode* node) {
    return (node && node->Parent()) ? node->Parent()->ToElement() : nullptr;
}

static const XMLElement* ParentElement(const XMLNode* node) {
    return (node && node->Parent()) ? node->Parent()->ToElement() : nullptr;
}

// XML names: a letter or '_' first, then letters, digits, '_', '-', '.'
static bool IsValidTagName(const char* name) {
    if (!name || !(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_')) return false;
    for (const char* p = name + 1; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}

// The widget whose list this element is (<Items> of a container, <Zones> of an RPMIndicator)
static const wtf::NodeDef* ListOwner(const XMLElement* e) {
    const XMLElement* owner = ParentElement(e);
    const wtf::NodeDef* def = owner ? wtf::DefinitionOf(owner) : nullptr;
    return (def && def->listTag && std::strcmp(def->listTag, e->Name()) == 0) ? def : nullptr;
}

static void HelpTooltip(const wtf::PropDef* prop) {
    if (prop && prop->help && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", wtf::EditorText(prop->help));
}

// ------------------------------------------------------------------
// Input control for one value. Returns true with the new text in 'out'.
// ------------------------------------------------------------------
static bool EditText(const char* text, std::string& out) {
    char buf[1024];
    strncpy(buf, text ? text : "", sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (!ImGui::InputText("##value", buf, sizeof(buf))) return false;
    out = buf;
    return true;
}

static void InvalidNote(const char* what) {
    ImGui::SameLine();
    ImGui::TextColored(kWarningColor, "%s", what);
}

bool EditPropertyValue(const wtf::PropDef& prop, const char* text, std::string& out, float width) {
    ImGui::SetNextItemWidth(width);
    switch (prop.kind) {
    case wtf::PropKind::Int: {
        int value = 0;
        if (!text || !XMLUtil::ToInt(text, &value)) {
            const bool changed = EditText(text, out);
            InvalidNote(app_language::Tr("not an integer, the default is used", "не целое число; используется значение по умолчанию"));
            return changed;
        }
        if (!ImGui::InputInt("##value", &value)) return false;
        out = std::to_string(value);
        return true;
    }
    case wtf::PropKind::Float: {
        float value = 0.0f;
        if (!text || !XMLUtil::ToFloat(text, &value) || !std::isfinite(value)) {
            const bool changed = EditText(text, out);
            InvalidNote(app_language::Tr("not a number, the default is used", "не число; используется значение по умолчанию"));
            return changed;
        }
        if (!ImGui::InputFloat("##value", &value, 0.0f, 0.0f, "%g")) return false;
        if (!std::isfinite(value)) return false;
        out = wtf::FormatFloat(value);
        return true;
    }
    case wtf::PropKind::Bool: {
        bool value = false;
        if (!text || !XMLUtil::ToBool(text, &value)) {
            const bool changed = EditText(text, out);
            InvalidNote(app_language::Tr("expected true or false", "ожидается true или false"));
            return changed;
        }
        if (!ImGui::Checkbox("##value", &value)) return false;
        out = value ? "true" : "false";
        return true;
    }
    case wtf::PropKind::Color:
    case wtf::PropKind::ColorRGB: {
        float rgba[4];
        if (!wtf::ParseColor(text, rgba)) {
            const bool changed = EditText(text, out);
            InvalidNote(app_language::Tr("expected #RRGGBB or #AARRGGBB", "ожидается #RRGGBB или #AARRGGBB"));
            return changed;
        }
        const bool rgbOnly = prop.kind == wtf::PropKind::ColorRGB;
        const bool changed = rgbOnly
            ? ImGui::ColorEdit3("##value", rgba)
            : ImGui::ColorEdit4("##value", rgba, ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar);
        if (!changed) return false;
        // Keep the alpha digits once the color has them, or when it is not opaque.
        const bool withAlpha = !rgbOnly && (std::strlen(text) == 9 || rgba[3] < 0.999f);
        out = wtf::FormatColor(rgba, withAlpha);
        return true;
    }
    case wtf::PropKind::Address: {
        // Shown as written; only valid addresses are stored, so an invalid edit reverts.
        char buf[64];
        strncpy(buf, text ? text : "", sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        const bool edited = ImGui::InputText("##value", buf, sizeof(buf), ImGuiInputTextFlags_CharsNoBlank);
        uint32_t address = 0;
        if (!wtf::ParseAddress(text, &address)) InvalidNote(app_language::Tr("invalid address, 0 is used", "недопустимый адрес; используется 0"));
        else {
            ImGui::SameLine();
            ImGui::TextDisabled("= 0x%08X", address);
        }
        if (!edited || !wtf::ParseAddress(buf, nullptr)) return false;
        out = buf;
        return true;
    }
    case wtf::PropKind::Enum: {
        const char* preview = nullptr;
        std::string unknown;
        for (const wtf::EnumOption& o : prop.options) {
            if (text && std::strcmp(o.value, text) == 0) preview = wtf::EditorText(o.label);
        }
        if (!preview) {
            unknown = std::string(text ? text : "") + app_language::Tr(" (unknown)", " (неизвестно)");
            preview = unknown.c_str();
        }
        bool changed = false;
        if (ImGui::BeginCombo("##value", preview)) {
            for (const wtf::EnumOption& o : prop.options) {
                const bool selected = text && std::strcmp(o.value, text) == 0;
                std::string label = std::string(o.value) + " - " + wtf::EditorText(o.label);
                const std::string english = std::string(o.value) + " - " + o.label;
                if (ImGui::Selectable(app_language::Label(english.c_str(), label.c_str()), selected)) {
                    out = o.value;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }
    case wtf::PropKind::Point: {
        float xy[2] = { 0.0f, 0.0f };
        if (!text || sscanf(text, "%f,%f", &xy[0], &xy[1]) != 2) {
            const bool changed = EditText(text, out);
            InvalidNote(app_language::Tr("expected x,y", "ожидается x,y"));
            return changed;
        }
        if (!ImGui::InputFloat2("##value", xy, "%g")) return false;
        if (!std::isfinite(xy[0]) || !std::isfinite(xy[1])) return false;
        out = wtf::FormatFloat(xy[0]) + "," + wtf::FormatFloat(xy[1]);
        return true;
    }
    case wtf::PropKind::Text:
    default:
        return EditText(text, out);
    }
}

// Elements the table does not know: guess the control from the value.
static bool EditGuessedValue(const char* name, const char* text, std::string& out) {
    ImGui::SetNextItemWidth(220.0f);
    if (std::strstr(name, "Address") || std::strcmp(name, "DoneAddr") == 0) {
        wtf::PropDef address;
        address.name = name;
        address.kind = wtf::PropKind::Address;
        address.defaultValue = "0";
        return EditPropertyValue(address, text, out);
    }
    ImVec4 color;
    bool boolValue = false;
    bool isFloat = false;
    if (text && ParseColor(text, color)) {
        if (!ImGui::ColorEdit4("##value", &color.x, ImGuiColorEditFlags_AlphaPreviewHalf)) return false;
        const float rgba[4] = { color.x, color.y, color.z, color.w };
        out = wtf::FormatColor(rgba, std::strlen(text) == 9 || color.w < 0.999f);
        return true;
    }
    if (text && IsBooleanString(text, boolValue)) {
        if (!ImGui::Checkbox("##value", &boolValue)) return false;
        out = boolValue ? "true" : "false";
        return true;
    }
    if (text && IsNumberString(text, isFloat)) {
        // A double keeps integers integral and still accepts fractions.
        double value = std::strtod(text, nullptr);
        if (!ImGui::InputDouble("##value", &value, 0.0, 0.0, "%.10g") || !std::isfinite(value)) return false;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.10g", value);
        out = buf;
        return true;
    }
    return EditText(text, out);
}

// ------------------------------------------------------------------
// Widget picker popup: returns the chosen widget type
// ------------------------------------------------------------------
const wtf::NodeDef* WidgetPickerPopup(const char* popupId) {
    const wtf::NodeDef* chosen = nullptr;
    if (!ImGui::BeginPopup(popupId)) return nullptr;
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##filter", app_language::Tr("filter", "фильтр"), g_widgetFilter, sizeof(g_widgetFilter));
    std::string filter;
    for (const char* p = g_widgetFilter; *p; ++p) filter.push_back((char)std::tolower(static_cast<unsigned char>(*p)));
    if (ImGui::BeginChild("##widgets", ImVec2(260, 320))) {
        for (const wtf::NodeDef& def : wtf::Widgets()) {
            if (!def.listed) continue;
            std::string name;
            for (const char* p = def.tag; *p; ++p) name.push_back((char)std::tolower(static_cast<unsigned char>(*p)));
            if (!filter.empty() && name.find(filter) == std::string::npos) continue;
            const std::string description = std::string(def.tag) + " — " + wtf::WidgetDescription(def.tag);
            if (ImGui::Selectable(app_language::Label(def.tag, description.c_str()))) {
                chosen = &def;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", wtf::WidgetDescription(def.tag));
        }
    }
    ImGui::EndChild();
    ImGui::EndPopup();
    return chosen;
}

// ------------------------------------------------------------------
// Recursive XML tree view
// ------------------------------------------------------------------
static void ShowXMLNode(XMLNode* node, int depth) {
    if (!node) return;

    XMLElement* element = node->ToElement();
    tinyxml2::XMLText* text = node->ToText();
    tinyxml2::XMLComment* comment = node->ToComment();
    tinyxml2::XMLDeclaration* decl = node->ToDeclaration();

    std::string prefix(depth * 2, ' ');
    ImGui::PushID(node);

    if (element) {
        // Widgets show their caption, which tells equal widgets apart
        std::string tag = "<" + std::string(element->Name()) + ">";
        const wtf::NodeDef* def = wtf::DefinitionOf(element);
        const char* caption = nullptr;
        if (def) {
            for (const char* key : { "Text", "Label", "Name" }) {
                if (!caption && wtf::FindProp(def, key)) caption = ui_safety::XmlChildText(element, key, nullptr);
            }
        }
        if (caption && *caption) tag += std::string(" ") + caption;

        ImGuiTreeNodeFlags flags = (g_selectedNode == node) ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (!element->FirstChild() && !element->FirstAttribute()) flags |= ImGuiTreeNodeFlags_Leaf;

        bool open = ImGui::TreeNodeEx("##node", flags, "%s%s", prefix.c_str(), tag.c_str());
        if (ImGui::IsItemClicked()) {
            g_selectedNode = node;
        }

        if (open) {
            for (const tinyxml2::XMLAttribute* attr = element->FirstAttribute(); attr; attr = attr->Next()) {
                ImGui::Text("%s  @%s = \"%s\"", prefix.c_str(), attr->Name(), attr->Value());
            }
            for (XMLNode* child = element->FirstChild(); child; child = child->NextSibling()) {
                ShowXMLNode(child, depth + 1);
            }
            ImGui::TreePop();
        }
    }
    else if (text) {
        ImGui::Text("%s\"%s\"", prefix.c_str(), text->Value());
        if (ImGui::IsItemClicked()) {
            g_selectedNode = node;
        }
    }
    else if (comment) {
        std::string display = prefix + "<!-- " + comment->Value() + " -->";
        if (ImGui::Selectable(display.c_str(), g_selectedNode == node)) {
            g_selectedNode = node;
        }
    }
    else if (decl) {
        ImGui::Text("%s<?xml ...?>", prefix.c_str());
        if (ImGui::IsItemClicked()) {
            g_selectedNode = node;
        }
    }

    ImGui::PopID();
}

// ------------------------------------------------------------------
// Properties panel for an element
// ------------------------------------------------------------------
// Attributes: typed controls for the ones the table knows, free editing for the rest.
// Returns true when the panel must stop drawing this frame.
static bool ShowAttributes(XMLElement* element, const wtf::NodeDef* def, MainContext* ctx) {
    if (!ImGui::CollapsingHeader(app_language::Label("Attributes", "Атрибуты"), ImGuiTreeNodeFlags_DefaultOpen)) return false;
    std::vector<std::pair<std::string, std::string>> attrs;
    for (const tinyxml2::XMLAttribute* attr = element->FirstAttribute(); attr; attr = attr->Next()) {
        attrs.emplace_back(attr->Name(), attr->Value());
    }

    for (size_t i = 0; i < attrs.size(); ++i) {
        ImGui::PushID((int)i);
        const wtf::PropDef* prop = wtf::FindProp(def, attrs[i].first.c_str());
        if (prop && prop->attribute) {
            ImGui::Text("%s:", prop->name);
            HelpTooltip(prop);
            ImGui::SameLine(140.0f);
            std::string value;
            if (EditPropertyValue(*prop, attrs[i].second.c_str(), value)) {
                element->SetAttribute(prop->name, value.c_str());
                MarkXMLModified(ctx);
            }
        }
        else {
            char nameBuf[128], valueBuf[256];
            strncpy(nameBuf, attrs[i].first.c_str(), sizeof(nameBuf) - 1);
            nameBuf[sizeof(nameBuf) - 1] = '\0';
            strncpy(valueBuf, attrs[i].second.c_str(), sizeof(valueBuf) - 1);
            valueBuf[sizeof(valueBuf) - 1] = '\0';

            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue) &&
                IsValidTagName(nameBuf)) {
                element->DeleteAttribute(attrs[i].first.c_str());
                element->SetAttribute(nameBuf, valueBuf);
                MarkXMLModified(ctx);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::InputText("##value", valueBuf, sizeof(valueBuf))) {
                element->SetAttribute(attrs[i].first.c_str(), valueBuf);
                MarkXMLModified(ctx);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            element->DeleteAttribute(attrs[i].first.c_str());
            MarkXMLModified(ctx);
            ImGui::PopID();
            return true;
        }
        ImGui::PopID();
    }

    // New attribute
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputTextWithHint("##newAttrName", app_language::Tr("name", "имя"), g_newAttrName, sizeof(g_newAttrName));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##newAttrValue", app_language::Tr("value", "значение"), g_newAttrValue, sizeof(g_newAttrValue));
    ImGui::SameLine();
    if (ImGui::Button(app_language::Label("Add", "Добавить")) && IsValidTagName(g_newAttrName)) {
        element->SetAttribute(g_newAttrName, g_newAttrValue);
        g_newAttrName[0] = g_newAttrValue[0] = '\0';
        MarkXMLModified(ctx);
    }
    return false;
}

// One row of the child element list. Returns true when the tree changed in a way
// that invalidates the rest of this frame's list (move, delete, select).
static bool ShowChildRow(XMLElement* element, const wtf::NodeDef* def, XMLElement* child,
    size_t index, size_t count, MainContext* ctx) {
    const char* childName = child->Name();
    const wtf::PropDef* prop = wtf::FindProp(def, childName);
    if (prop && prop->attribute) prop = nullptr;

    ImGui::PushID(child);
    ImGui::AlignTextToFramePadding();
    if (def && !prop && !HasChildElements(child) && !(def->listTag && std::strcmp(def->listTag, childName) == 0)) {
        ImGui::TextColored(kWarningColor, "%s:", childName);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(app_language::Tr("<%s> does not use <%s>; the loader ignores it.", "<%s> не использует <%s>; загрузчик его игнорирует."), def->tag, childName);
    }
    else {
        ImGui::Text("%s:", childName);
        HelpTooltip(prop);
    }
    ImGui::SameLine(140.0f);

    bool restart = false;
    if (HasChildElements(child) || (def && def->listTag && std::strcmp(def->listTag, childName) == 0)) {
        // A list or a nested element: open it instead of editing text
        const wtf::NodeDef* childDef = wtf::DefinitionOf(child);
        const char* caption = childDef ? ui_safety::XmlChildText(child, "Text", nullptr) : nullptr;
        if (!caption && childDef) caption = ui_safety::XmlChildText(child, "Name", nullptr);
        if (!caption && childDef) caption = ui_safety::XmlChildText(child, "Label", nullptr);
        if (caption) ImGui::Text("%s", caption);
        else ImGui::TextDisabled(app_language::Tr("%d element(s)", "Элементов: %d"), CountChildElements(child));
        ImGui::SameLine();
        if (ImGui::SmallButton(app_language::Label("Open", "Открыть"))) {
            g_selectedNode = child;
            restart = true;
        }
    }
    else {
        std::string value;
        const bool changed = prop ? EditPropertyValue(*prop, child->GetText(), value)
                                  : EditGuessedValue(childName, child->GetText(), value);
        if (changed) {
            child->SetText(value.c_str());
            MarkXMLModified(ctx);
        }
    }

    // Move up/down and delete
    if (!restart && index > 0) {
        ImGui::SameLine();
        if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
            XMLNode* prev = child->PreviousSiblingElement();
            XMLNode* prevPrev = prev ? prev->PreviousSibling() : nullptr;
            if (prevPrev) element->InsertAfterChild(prevPrev, child);
            else element->InsertFirstChild(child);
            MarkXMLModified(ctx);
            restart = true;
        }
    }
    if (!restart && index + 1 < count) {
        ImGui::SameLine();
        if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
            XMLNode* next = child->NextSiblingElement();
            if (next) element->InsertAfterChild(next, child);
            MarkXMLModified(ctx);
            restart = true;
        }
    }
    if (!restart) {
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            element->DeleteChild(child);
            MarkXMLModified(ctx);
            restart = true;
        }
    }
    ImGui::PopID();
    return restart;
}

static void ShowNodeProperties(XMLNode* node, tinyxml2::XMLDocument* doc, MainContext* ctx) {
    if (!node) {
        ImGui::Text("%s", app_language::Tr("No node selected", "Узел не выбран"));
        return;
    }

    XMLElement* element = node->ToElement();
    tinyxml2::XMLText* textNode = node->ToText();
    tinyxml2::XMLComment* commentNode = node->ToComment();
    tinyxml2::XMLDeclaration* declNode = node->ToDeclaration();

    if (element) {
        const wtf::NodeDef* def = wtf::DefinitionOf(element);
        XMLElement* parent = ParentElement(element);
        const wtf::NodeDef* parentDef = parent ? wtf::DefinitionOf(parent) : nullptr;
        const wtf::NodeDef* listOwner = ListOwner(element);
        const wtf::PropDef* ownProp = (!def && parentDef) ? wtf::FindProp(parentDef, element->Name()) : nullptr;

        // What this element is
        if (def == &wtf::WindowDef()) ImGui::Text("%s", app_language::Tr("Window", "Окно"));
        else if (def && parent && ListOwner(parent) && ListOwner(parent)->itemTag)
            ImGui::Text(app_language::Tr("Item <%s> of <%s>", "Элемент <%s> в <%s>"), element->Name(), ListOwner(parent)->tag);
        else if (def) ImGui::Text(app_language::Tr("Widget <%s>", "Виджет <%s>"), element->Name());
        else if (listOwner) ImGui::Text(app_language::Tr("List of <%s>", "Список <%s>"), listOwner->tag);
        else if (ownProp) ImGui::Text(app_language::Tr("Property <%s> of <%s>", "Свойство <%s> в <%s>"), element->Name(), parentDef->tag);
        else ImGui::TextColored(kWarningColor, app_language::Tr("<%s> is not a known widget, list or property here", "<%s> здесь не является известным виджетом, списком или свойством"), element->Name());
        ImGui::Separator();

        // Element name
        char elemName[256];
        strncpy(elemName, element->Name(), sizeof(elemName) - 1);
        elemName[sizeof(elemName) - 1] = '\0';
        if (ImGui::InputText(app_language::Label("Tag name", "Имя тега"), elemName, sizeof(elemName), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (IsValidTagName(elemName)) {
                element->SetName(elemName);
                MarkXMLModified(ctx);
            }
        }

        if (ownProp && !HasChildElements(element) && !ownProp->attribute) {
            // A single property: the same typed control as in its widget
            ImGui::PushID(element);
            ImGui::Text("%s", app_language::Tr("Value:", "Значение:"));
            HelpTooltip(ownProp);
            ImGui::SameLine(140.0f);
            std::string value;
            if (EditPropertyValue(*ownProp, element->GetText(), value)) {
                element->SetText(value.c_str());
                MarkXMLModified(ctx);
            }
            ImGui::PopID();
        }

        if (ShowAttributes(element, def, ctx)) return;

        // Text content: for unknown elements, and for text already there (<Text>Hello</Text>
        // is a valid Text widget); widgets otherwise keep their values in child elements.
        XMLNode* first = element->FirstChild();
        tinyxml2::XMLText* textChild = first ? first->ToText() : nullptr;
        if (((!def && !listOwner && !ownProp) || textChild) && !HasChildElements(element)) {
            if (ImGui::CollapsingHeader(app_language::Label("Text content", "Текстовое содержимое"), ImGuiTreeNodeFlags_DefaultOpen)) {
                char textBuf[1024];
                strncpy(textBuf, textChild ? textChild->Value() : "", sizeof(textBuf) - 1);
                textBuf[sizeof(textBuf) - 1] = '\0';
                if (ImGui::InputTextMultiline("##text", textBuf, sizeof(textBuf), ImVec2(-1, 60))) {
                    element->SetText(textBuf);
                    MarkXMLModified(ctx);
                }
            }
        }

        // Child elements
        if (HasChildElements(element) && ImGui::CollapsingHeader(app_language::Label("Child Elements", "Дочерние элементы"), ImGuiTreeNodeFlags_DefaultOpen)) {
            std::vector<XMLElement*> children;
            for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
                children.push_back(child);
            }
            for (size_t i = 0; i < children.size(); ++i) {
                if (ShowChildRow(element, def, children[i], i, children.size(), ctx)) return;
            }
        }

        ImGui::Separator();

        // Missing properties of a widget, window or list item
        if (def) {
            const std::vector<const wtf::PropDef*> missing = wtf::MissingProperties(element, *def);
            if (!missing.empty()) {
                if (ImGui::Button(app_language::Label("Add property", "Добавить свойство"))) ImGui::OpenPopup(app_language::Label("AddPropertyPopup", "AddPropertyPopup"));
                ImGui::SameLine();
            }
            if (ImGui::BeginPopup(app_language::Label("AddPropertyPopup", "AddPropertyPopup"))) {
                for (const wtf::PropDef* prop : missing) {
                    std::string label = std::string(prop->name) + (prop->inTemplate ? "" : app_language::Tr("  (old format)", "  (старый формат)"));
                    const std::string english = std::string(prop->name) + (prop->inTemplate ? "" : "  (old format)");
                    if (ImGui::Selectable(app_language::Label(english.c_str(), label.c_str()))) {
                        wtf::AddProperty(element, *prop);
                        MarkXMLModified(ctx);
                    }
                    HelpTooltip(prop);
                }
                ImGui::EndPopup();
            }
        }

        // Lists: items of a widget, or child widgets of a container
        const wtf::NodeDef* owner = def && def->listTag ? def : listOwner;
        if (owner) {
            XMLElement* ownerElement = (owner == def) ? element : parent;
            if (owner->itemTag) {
                std::string label = std::string(app_language::Tr("Add ", "Добавить ")) + owner->itemTag;
                const std::string english = std::string("Add ") + owner->itemTag;
                if (ImGui::Button(app_language::Label(english.c_str(), label.c_str()))) {
                    XMLElement* item = wtf::CreateItem(doc, *owner);
                    if (item) {
                        wtf::EnsureList(ownerElement, *owner)->InsertEndChild(item);
                        MarkXMLModified(ctx);
                    }
                }
            }
            else {
                if (ImGui::Button(app_language::Label("Add child widget", "Добавить дочерний виджет"))) {
                    g_widgetFilter[0] = '\0';
                    ImGui::OpenPopup(app_language::Label("AddChildWidgetPopup", "AddChildWidgetPopup"));
                }
                if (const wtf::NodeDef* chosen = WidgetPickerPopup(app_language::Label("AddChildWidgetPopup", "AddChildWidgetPopup"))) {
                    XMLElement* widget = wtf::CreateNode(doc, *chosen);
                    wtf::EnsureList(ownerElement, *owner)->InsertEndChild(widget);
                    g_selectedNode = widget;
                    MarkXMLModified(ctx);
                    return;
                }
            }
            ImGui::SameLine();
        }

        if (ImGui::Button(app_language::Label("Add comment", "Добавить комментарий"))) {
            tinyxml2::XMLComment* newComment = doc->NewComment("New comment");
            XMLElement* container = (def && def->listTag) ? wtf::EnsureList(element, *def) : element;
            container->InsertEndChild(newComment);
            MarkXMLModified(ctx);
        }
        ImGui::SameLine();

        if (ImGui::Button(app_language::Label("Delete this node", "Удалить этот узел"))) {
            XMLNode* parentNode = node->Parent();
            if (parentNode)
                parentNode->DeleteChild(node);
            g_selectedNode = nullptr;
            MarkXMLModified(ctx);
            return;
        }
    }
    else if (textNode) {
        // Text node
        char textBuf[1024];
        strncpy(textBuf, textNode->Value(), sizeof(textBuf) - 1);
        textBuf[sizeof(textBuf) - 1] = '\0';
        if (ImGui::InputTextMultiline(app_language::Label("Text", "Текст"), textBuf, sizeof(textBuf))) {
            textNode->SetValue(textBuf);
            MarkXMLModified(ctx);
        }
        if (ImGui::Button(app_language::Label("Delete this node", "Удалить этот узел"))) {
            XMLNode* parentNode = node->Parent();
            if (parentNode)
                parentNode->DeleteChild(node);
            g_selectedNode = nullptr;
            MarkXMLModified(ctx);
            return;
        }
    }
    else if (commentNode) {
        // Comment
        ImGui::Text("%s", app_language::Tr("Comment", "Комментарий"));
        char commentBuf[1024];
        strncpy(commentBuf, commentNode->Value(), sizeof(commentBuf) - 1);
        commentBuf[sizeof(commentBuf) - 1] = '\0';
        if (ImGui::InputTextMultiline("##comment", commentBuf, sizeof(commentBuf))) {
            commentNode->SetValue(commentBuf);
            MarkXMLModified(ctx);
        }
        if (ImGui::Button(app_language::Label("Delete this comment", "Удалить этот комментарий"))) {
            XMLNode* parentNode = node->Parent();
            if (parentNode)
                parentNode->DeleteChild(node);
            g_selectedNode = nullptr;
            MarkXMLModified(ctx);
            return;
        }
    }
    else if (declNode) {
        ImGui::Text("%s", app_language::Tr("XML Declaration (cannot be edited)", "Объявление XML (нельзя редактировать)"));
    }
}

// ------------------------------------------------------------------
// Editor window
// ------------------------------------------------------------------
void ShowWidgetEditor(bool* p_open, MainContext* ctx) {
    if (!ctx->CurrentXMLDoc) {
        ImGui::OpenPopup(app_language::Label("No XML loaded", "XML не загружен"));
        if (ImGui::BeginPopupModal(app_language::Label("No XML loaded", "XML не загружен"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", app_language::Tr("Please open an XML file first.", "Сначала откройте XML-файл."));
            if (ImGui::Button(app_language::Label("OK", "ОК"))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        return;
    }

    if (g_selectedNode && !ui_safety::ContainsXMLNode(ctx->CurrentXMLDoc, g_selectedNode)) {
        g_selectedNode = nullptr;   // node from an old document, drop the selection
    }

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(app_language::Label("Widget Editor", "Редактор виджетов"), p_open)) {
        ImGui::End();
        return;
    }

    // Left panel: XML tree
    if (ImGui::BeginChild("Tree", ImVec2(300, -ImGui::GetFrameHeightWithSpacing() - 30), ImGuiChildFlags_Borders)) {
        XMLNode* root = ctx->CurrentXMLDoc->FirstChild();
        while (root) {
            ShowXMLNode(root, 0);
            root = root->NextSibling();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right panel: properties
    if (ImGui::BeginChild(app_language::Label("Properties", "Свойства"), ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 30), ImGuiChildFlags_Borders)) {
        ShowNodeProperties(g_selectedNode, ctx->CurrentXMLDoc, ctx);
    }
    ImGui::EndChild();

    // Bottom toolbar
    if (ImGui::Button(app_language::Label("Add Window", "Добавить окно"), ImVec2(120, 0))) {
        XMLElement* newWindow = wtf::CreateNewWindow(ctx->CurrentXMLDoc);
        ctx->CurrentXMLDoc->InsertEndChild(newWindow);
        g_selectedNode = newWindow;
        MarkXMLModified(ctx);
    }
    ImGui::SameLine();

    if (ImGui::Button(app_language::Label("Add Widget", "Добавить виджет"), ImVec2(120, 0))) {
        g_widgetFilter[0] = '\0';
        ImGui::OpenPopup(app_language::Label("AddWidgetPopup", "AddWidgetPopup"));
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", app_language::Tr("Into the selected container, after the selected widget, or into the first window", "В выбранный контейнер, после выбранного виджета или в первое окно"));
    ImGui::SameLine();

    if (ImGui::Button(app_language::Label("Save", "Сохранить"), ImVec2(120, 0))) {
        if (ctx->CurrentXMLDoc->SaveFile(ctx->File_path) == tinyxml2::XML_SUCCESS) {
            ClearAllWindows();
            LoadWindowsFromXML(ctx, ctx->File_path);
            ctx->xml_modified = false;
            ImGui::OpenPopup(app_language::Label("Saved", "Сохранено"));
        }
        else {
            ImGui::OpenPopup(app_language::Label("Save Error", "Ошибка сохранения"));
        }
    }

    if (const wtf::NodeDef* chosen = WidgetPickerPopup(app_language::Label("AddWidgetPopup", "AddWidgetPopup"))) {
        XMLElement* selected = g_selectedNode ? g_selectedNode->ToElement() : nullptr;
        if (!selected && g_selectedNode) selected = ParentElement(g_selectedNode);
        XMLElement* widget = wtf::CreateNode(ctx->CurrentXMLDoc, *chosen);
        if (wtf::InsertWidget(ctx->CurrentXMLDoc, selected, widget)) {
            g_selectedNode = widget;
            MarkXMLModified(ctx);
        }
        else {
            ctx->CurrentXMLDoc->DeleteNode(widget);
            ImGui::OpenPopup(app_language::Label("No Window", "Нет окна"));
        }
    }

    // Notification popups
    if (ImGui::BeginPopupModal(app_language::Label("No Window", "Нет окна"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", app_language::Tr("Add a window first.", "Сначала добавьте окно."));
        if (ImGui::Button(app_language::Label("OK", "ОК"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal(app_language::Label("Saved", "Сохранено"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", app_language::Tr("File saved successfully.", "Файл успешно сохранён."));
        if (ImGui::Button(app_language::Label("OK", "ОК"))) ImGui::CloseCurrentPopup();
        ctx->file_loaded = false; // reload the file the next time it is opened
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal(app_language::Label("Save Error", "Ошибка сохранения"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", app_language::Tr("Error saving file.", "Ошибка сохранения файла."));
        if (ImGui::Button(app_language::Label("OK", "ОК"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}
