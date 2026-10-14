// XMLEditor.cpp
#include "XMLEditor.h"
#include "Main.h"
#include "imgui.h"
#include "tinyxml2/tinyxml2.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cmath>

#include "WidgetTemplateFactory.h"
#include "XMLSafety.h"

// ------------------------------------------------------------------
// Вспомогательные функции для преобразования цвета
// ------------------------------------------------------------------
static bool ParseColor(const char* str, ImVec4& outColor) {
    if (!str || str[0] != '#') return false;
    unsigned int hex = 0;
    int len = (int)strlen(str);
    for (int i = 1; i < len; ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
    }
    if (len == 7) { // #RRGGBB
        if (sscanf(str + 1, "%06x", &hex) == 1) {
            outColor.x = ((hex >> 16) & 0xFF) / 255.0f;
            outColor.y = ((hex >> 8) & 0xFF) / 255.0f;
            outColor.z = (hex & 0xFF) / 255.0f;
            outColor.w = 1.0f;
            return true;
        }
    }
    else if (len == 9) { // #AARRGGBB
        if (sscanf(str + 1, "%08x", &hex) == 1) {
            outColor.w = ((hex >> 24) & 0xFF) / 255.0f;
            outColor.x = ((hex >> 16) & 0xFF) / 255.0f;
            outColor.y = ((hex >> 8) & 0xFF) / 255.0f;
            outColor.z = (hex & 0xFF) / 255.0f;
            return true;
        }
    }
    return false;
}

static std::string FormatColor(const ImVec4& col, bool withAlpha = true) {
    unsigned int r = (unsigned int)(col.x * 255.0f + 0.5f);
    unsigned int g = (unsigned int)(col.y * 255.0f + 0.5f);
    unsigned int b = (unsigned int)(col.z * 255.0f + 0.5f);
    if (withAlpha) {
        unsigned int a = (unsigned int)(col.w * 255.0f + 0.5f);
        char buf[10];
        snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", a, r, g, b);
        return std::string(buf);
    }
    else {
        char buf[8];
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
        return std::string(buf);
    }
}

// ------------------------------------------------------------------
// Вспомогательная функция для определения, является ли строка булевой
// ------------------------------------------------------------------
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

// ------------------------------------------------------------------
// Вспомогательная функция для определения, является ли строка числом (int/float)
// ------------------------------------------------------------------
static bool IsNumberString(const char* str, bool& isFloat) {
    if (!str || *str == '\0') return false;
    char* endptr;
    errno = 0;
    float f = strtof(str, &endptr);
    if (errno == ERANGE || !std::isfinite(f)) return false;
    if (endptr == str) return false; // не удалось преобразовать
    // проверяем, остались ли после числа только пробелы
    while (*endptr) {
        if (!std::isspace(static_cast<unsigned char>(*endptr))) return false;
        endptr++;
    }
    // определяем тип: если есть точка или экспонента, то float
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
// Функция для определения, может ли элемент иметь дочерние элементы
// ------------------------------------------------------------------
static bool CanHaveChildren(const char* elementName) {
    // Список универсальных контейнеров, которые могут содержать любые виджеты
    static const std::set<std::string> containers = {
        "Group", "TreeNode", "TabBar", "Tab", "ChildWindow", "Header", "Popup", "HintBox",
        "FixedWidth", "Column", "Grid", "Window"
    };
    if (containers.find(elementName) != containers.end()) {
        return true;
    }
    // Проверяем, есть ли запись в childMap для этого элемента
    const auto& childMap = WidgetTemplateFactory::GetChildCreators();
    if (childMap.find(elementName) != childMap.end()) {
        return true;
    }
    return false;
}

// ------------------------------------------------------------------
// Вспомогательные функции и глобальные переменные редактора
// ------------------------------------------------------------------
static tinyxml2::XMLNode* g_selectedNode = nullptr;
static char g_newAttrName[128] = "";
static char g_newAttrValue[128] = "";

void ResetXMLEditorState() {
    g_selectedNode = nullptr;
    g_newAttrName[0] = '\0';
    g_newAttrValue[0] = '\0';
}

// ------------------------------------------------------------------
// Функция для поиска контейнера, в который следует вставлять дочерние виджеты
// ------------------------------------------------------------------
static tinyxml2::XMLElement* GetContainerForChildren(tinyxml2::XMLElement* parent, tinyxml2::XMLDocument* doc, const char* childName = nullptr) {
    const char* name = parent->Name();

    // DataLog, Histogram, PlotLines: параметры (не Line) идут в сам элемент, Line идёт в <Items>
    if (strcmp(name, "DataLog") == 0 || strcmp(name, "Histogram") == 0 || strcmp(name, "PlotLines") == 0) {
        if (childName && strcmp(childName, "Line") == 0) {
            tinyxml2::XMLElement* items = parent->FirstChildElement("Items");
            if (!items) {
                items = doc->NewElement("Items");
                parent->InsertEndChild(items);
            }
            return items;
        }
        return parent; // параметры (Text, Format и др.) идут в сам элемент
    }

    // Специализированные контейнеры
    if (strcmp(name, "ComboBox") == 0 || strcmp(name, "RadioButton") == 0 ||
        strcmp(name, "SegmentButton") == 0) {
        tinyxml2::XMLElement* items = parent->FirstChildElement("Items");
        if (!items) {
            items = doc->NewElement("Items");
            parent->InsertEndChild(items);
        }
        return items;
    }
    if (strcmp(name, "RPMIndicator") == 0) {
        tinyxml2::XMLElement* zones = parent->FirstChildElement("Zones");
        if (!zones) {
            zones = doc->NewElement("Zones");
            parent->InsertEndChild(zones);
        }
        return zones;
    }

    // Для универсальных контейнеров (Group, Tab, ChildWindow, Header, Popup, HintBox и т.п.) ищем <Items>
    tinyxml2::XMLElement* items = parent->FirstChildElement("Items");
    if (items) return items;

    // Если ничего не найдено, возвращаем самого родителя (вставка прямо в него)
    return parent;
}

// ------------------------------------------------------------------
// Функция для поиска родительского элемента для вставки нового виджета
// (используется кнопкой "Add Widget" в главном окне)
// ------------------------------------------------------------------
static tinyxml2::XMLElement* FindInsertionParent(tinyxml2::XMLDocument* doc, const char* widgetType = nullptr) {
    // Если выбран какой-то элемент, используем его как родителя
    if (g_selectedNode && g_selectedNode->ToElement()) {
        return GetContainerForChildren(g_selectedNode->ToElement(), doc, widgetType);
    }
    // Иначе ищем первый элемент Window
    tinyxml2::XMLElement* root = doc->FirstChildElement("Window");
    if (root) {
        // Убедимся, что у окна есть <Items>
        tinyxml2::XMLElement* items = root->FirstChildElement("Items");
        if (!items) {
            items = doc->NewElement("Items");
            root->InsertEndChild(items);
        }
        return items;
    }
    return nullptr;
}

// ------------------------------------------------------------------
// Рекурсивное отображение дерева XML
// ------------------------------------------------------------------
static void ShowXMLNode(tinyxml2::XMLNode* node, int depth) {
    if (!node) return;

    tinyxml2::XMLElement* element = node->ToElement();
    tinyxml2::XMLText* text = node->ToText();
    tinyxml2::XMLComment* comment = node->ToComment();
    tinyxml2::XMLDeclaration* decl = node->ToDeclaration();

    std::string prefix(depth * 2, ' ');
    ImGui::PushID(node);

    if (element) {
        std::string tag = "<" + std::string(element->Name()) + ">";
        ImGuiTreeNodeFlags flags = (g_selectedNode == node) ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        bool open = ImGui::TreeNodeEx(element->Name(), flags, "%s%s", prefix.c_str(), tag.c_str());
        if (ImGui::IsItemClicked()) {
            g_selectedNode = node;
        }

        if (open) {
            for (const tinyxml2::XMLAttribute* attr = element->FirstAttribute(); attr; attr = attr->Next()) {
                ImGui::Text("%s  @%s = \"%s\"", prefix.c_str(), attr->Name(), attr->Value());
            }
            for (tinyxml2::XMLNode* child = element->FirstChild(); child; child = child->NextSibling()) {
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
// Панель свойств для выбранного узла
// ------------------------------------------------------------------
static void ShowNodeProperties(tinyxml2::XMLNode* node, tinyxml2::XMLDocument* doc, MainContext* ctx) {
    if (!node) {
        ImGui::Text("No node selected");
        return;
    }

    tinyxml2::XMLElement* element = node->ToElement();
    tinyxml2::XMLText* textNode = node->ToText();
    tinyxml2::XMLComment* commentNode = node->ToComment();
    tinyxml2::XMLDeclaration* declNode = node->ToDeclaration();

    // Список имён, которые могут повторяться
    static const std::set<std::string> multipleElements = {
        "Line", "Zone", "Combo", "Radio", "Segment", "SameLine", "Spacing", "Separator"
    };

    if (element) {
        // Редактирование имени элемента
        static char elemName[256];
        strncpy(elemName, element->Name(), sizeof(elemName));
        elemName[sizeof(elemName) - 1] = '\0';
        if (ImGui::InputText("Tag name", elemName, sizeof(elemName))) {
            element->SetName(elemName);
            ctx->xml_modified = true;
        }

        // Таблица атрибутов
        if (ImGui::CollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::vector<std::pair<std::string, std::string>> attrs;
            for (const tinyxml2::XMLAttribute* attr = element->FirstAttribute(); attr; attr = attr->Next()) {
                attrs.emplace_back(attr->Name(), attr->Value());
            }

            for (size_t i = 0; i < attrs.size(); ++i) {
                ImGui::PushID((int)i);
                char nameBuf[128], valueBuf[256];
                strncpy(nameBuf, attrs[i].first.c_str(), sizeof(nameBuf));
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                strncpy(valueBuf, attrs[i].second.c_str(), sizeof(valueBuf));
                valueBuf[sizeof(valueBuf) - 1] = '\0';

                if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    element->DeleteAttribute(attrs[i].first.c_str());
                    element->SetAttribute(nameBuf, valueBuf);
                    ctx->xml_modified = true;
                }
                ImGui::SameLine();
                if (ImGui::InputText("##value", valueBuf, sizeof(valueBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    element->SetAttribute(attrs[i].first.c_str(), valueBuf);
                    ctx->xml_modified = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    element->DeleteAttribute(attrs[i].first.c_str());
                    ctx->xml_modified = true;
                }
                ImGui::PopID();
            }

            // Добавление нового атрибута
            ImGui::InputTextWithHint("##newAttrName", "name", g_newAttrName, sizeof(g_newAttrName));
            ImGui::SameLine();
            ImGui::InputTextWithHint("##newAttrValue", "value", g_newAttrValue, sizeof(g_newAttrValue));
            ImGui::SameLine();
            if (ImGui::Button("Add")) {
                if (g_newAttrName[0] && g_newAttrValue[0]) {
                    element->SetAttribute(g_newAttrName, g_newAttrValue);
                    g_newAttrName[0] = g_newAttrValue[0] = '\0';
                    ctx->xml_modified = true;
                }
            }
        }

        // Текстовое содержимое элемента (первый текстовый узел)
        tinyxml2::XMLText* textChild = nullptr;
        for (tinyxml2::XMLNode* child = element->FirstChild(); child; child = child->NextSibling()) {
            if (child->ToText()) {
                textChild = child->ToText();
                break;
            }
        }
        if (ImGui::CollapsingHeader("Text content", ImGuiTreeNodeFlags_DefaultOpen)) {
            static char textBuf[1024];
            if (textChild) {
                strncpy(textBuf, textChild->Value(), sizeof(textBuf));
                textBuf[sizeof(textBuf) - 1] = '\0';
                if (ImGui::InputTextMultiline("##text", textBuf, sizeof(textBuf))) {
                    textChild->SetValue(textBuf);
                    ctx->xml_modified = true;
                }
            }
            else {
                ImGui::Text("(no text)");
                if (ImGui::Button("Add text")) {
                    element->InsertEndChild(doc->NewText(""));
                    ctx->xml_modified = true;
                }
            }
        }

        // Секция дочерних элементов (только элементы, не текстовые узлы)
        if (ImGui::CollapsingHeader("Child Elements", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::vector<tinyxml2::XMLElement*> children;
            for (tinyxml2::XMLNode* child = element->FirstChild(); child; child = child->NextSibling()) {
                if (child->ToElement()) {
                    children.push_back(child->ToElement());
                }
            }

            if (children.empty()) {
                ImGui::Text("No child elements.");
            }
            else {
                for (size_t i = 0; i < children.size(); ++i) {
                    tinyxml2::XMLElement* childElem = children[i];
                    const char* childName = childElem->Name();
                    const char* childText = childElem->GetText(); // может быть nullptr

                    ImGui::PushID(childElem);

                    // Отображаем имя тега слева
                    ImGui::Text("%s:", childName);
                    ImGui::SameLine();

                    // Определяем тип и создаем соответствующий контрол
                    ImVec4 color;
                    bool boolVal;
                    bool isFloat;
                    bool isAddress = (strcmp(childName, "Address") == 0 ||
                        strcmp(childName, "BaseAddress") == 0 ||
                        strcmp(childName, "Line1Address") == 0 ||
                        strcmp(childName, "Line2Address") == 0 ||
                        strcmp(childName, "DoneAddr") == 0);

                    if (childText && isAddress) {
                        // Редактируем адрес в hex
                        unsigned int addr = 0;
                        const char* text = childText;
                        if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
                            sscanf(text + 2, "%x", &addr);
                        }
                        else if (text[0] == '#') {
                            sscanf(text + 1, "%x", &addr);
                        }
                        else {
                            sscanf(text, "%x", &addr);
                        }
                        char hexBuf[32];
                        snprintf(hexBuf, sizeof(hexBuf), "0x%08X", addr);
                        if (ImGui::InputText("##value", hexBuf, sizeof(hexBuf))) {
                            unsigned int newAddr;
                            if (sscanf(hexBuf, "%x", &newAddr) == 1) {
                                char out[16];
                                snprintf(out, sizeof(out), "0x%08X", newAddr);
                                childElem->SetText(out);
                                ctx->xml_modified = true;
                            }
                        }
                    }
                    else if (childText && ParseColor(childText, color)) {
                        // Цвет
                        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs;
                        if (ImGui::ColorEdit4("##value", (float*)&color, flags)) {
                            bool hasAlpha = (color.w < 0.999f);
                            std::string newText = FormatColor(color, hasAlpha);
                            childElem->SetText(newText.c_str());
                            ctx->xml_modified = true;
                        }
                    }
                    else if (childText && IsBooleanString(childText, boolVal)) {
                        bool current = boolVal;
                        if (ImGui::Checkbox("##value", &current)) {
                            childElem->SetText(current ? "true" : "false");
                            ctx->xml_modified = true;
                        }
                    }
                    else if (childText && IsNumberString(childText, isFloat)) {
                        if (isFloat) {
                            float f = (float)atof(childText);
                            if (ImGui::InputFloat("##value", &f, 0.0f, 0.0f, "%.3f")) {
                                char buf[64];
                                snprintf(buf, sizeof(buf), "%g", f);
                                childElem->SetText(buf);
                                ctx->xml_modified = true;
                            }
                        }
                        else {
                            int i = atoi(childText);
                            if (ImGui::InputInt("##value", &i)) {
                                char buf[32];
                                snprintf(buf, sizeof(buf), "%d", i);
                                childElem->SetText(buf);
                                ctx->xml_modified = true;
                            }
                        }
                    }
                    else {
                        // Обычный текст
                        static char textBuf[1024];
                        strncpy(textBuf, childText ? childText : "", sizeof(textBuf));
                        textBuf[sizeof(textBuf) - 1] = '\0';
                        if (ImGui::InputText("##value", textBuf, sizeof(textBuf))) {
                            childElem->SetText(textBuf);
                            ctx->xml_modified = true;
                        }
                    }

                    // Кнопки перемещения вверх/вниз
                    ImGui::SameLine();
                    if (i > 0) {
                        if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                            tinyxml2::XMLNode* parent = childElem->Parent();
                            tinyxml2::XMLNode* prev = childElem->PreviousSibling();
                            tinyxml2::XMLNode* prevPrev = prev ? prev->PreviousSibling() : nullptr;
                            if (prevPrev) {
                                parent->InsertAfterChild(prevPrev, childElem);
                            }
                            else {
                                parent->InsertFirstChild(childElem);
                            }
                            ctx->xml_modified = true;
                            ImGui::PopID();
                            return;
                        }
                        ImGui::SameLine();
                    }
                    if (i < children.size() - 1) {
                        if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
                            tinyxml2::XMLNode* parent = childElem->Parent();
                            tinyxml2::XMLNode* next = childElem->NextSibling();
                            if (next) {
                                parent->InsertAfterChild(next, childElem);
                            }
                            ctx->xml_modified = true;
                            ImGui::PopID();
                            return;
                        }
                        ImGui::SameLine();
                    }

                    if (ImGui::Button("X")) {
                        element->DeleteChild(childElem);
                        ctx->xml_modified = true;
                        ImGui::PopID();
                        return;
                    }
                    ImGui::PopID();
                }
            }
        }

        // Кнопки управления узлом
        bool canHaveChildren = CanHaveChildren(element->Name());
        if (canHaveChildren) {
            if (ImGui::Button("Add child widget")) {
                ImGui::OpenPopup("AddChildWidgetPopup");
            }
            ImGui::SameLine();
        }

        if (ImGui::Button("Add comment")) {
            tinyxml2::XMLComment* newComment = doc->NewComment("New comment");
            tinyxml2::XMLElement* container = GetContainerForChildren(element, doc, nullptr);
            container->InsertEndChild(newComment);
            ctx->xml_modified = true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Delete this node")) {
            tinyxml2::XMLNode* parent = node->Parent();
            if (parent)
                parent->DeleteChild(node);
            g_selectedNode = nullptr;
            ctx->xml_modified = true;
            return;
        }

        // Попап для выбора типа дочернего виджета
        if (canHaveChildren && ImGui::BeginPopup("AddChildWidgetPopup")) {
            ImGui::Text("Select child widget type:");
            ImGui::Separator();

            const char* parentType = element->Name();
            const auto& childMap = WidgetTemplateFactory::GetChildCreators();
            auto it = childMap.find(parentType);

            // Собираем имена уже существующих дочерних элементов
            std::set<std::string> existingNames;
            for (tinyxml2::XMLNode* child = element->FirstChild(); child; child = child->NextSibling()) {
                if (child->ToElement()) {
                    existingNames.insert(child->ToElement()->Name());
                }
            }

            if (it != childMap.end()) {
                bool anyAvailable = false;
                for (const auto& pair : it->second) {
                    bool isMultiple = (multipleElements.find(pair.first) != multipleElements.end());
                    if (isMultiple || existingNames.find(pair.first) == existingNames.end()) {
                        anyAvailable = true;
                        if (ImGui::Selectable(pair.first.c_str())) {
                            tinyxml2::XMLElement* newElem = pair.second(doc);
                            if (newElem) {
                                tinyxml2::XMLElement* container = GetContainerForChildren(element, doc, pair.first.c_str());
                                container->InsertEndChild(newElem);
                                ctx->xml_modified = true;
                            }
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                if (!anyAvailable) {
                    ImGui::Text("All available elements already added.");
                }
            }
            else {
                for (const auto& pair : WidgetTemplateFactory::GetRootCreators()) {
                    bool isMultiple = (multipleElements.find(pair.first) != multipleElements.end());
                    if (isMultiple || existingNames.find(pair.first) == existingNames.end()) {
                        if (ImGui::Selectable(pair.first.c_str())) {
                            tinyxml2::XMLElement* newElem = pair.second(doc);
                            if (newElem) {
                                tinyxml2::XMLElement* container = GetContainerForChildren(element, doc, pair.first.c_str());
                                container->InsertEndChild(newElem);
                                ctx->xml_modified = true;
                            }
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            ImGui::EndPopup();
        }
    }
    else if (textNode) {
        // Редактирование текстового узла
        static char textBuf[1024];
        strncpy(textBuf, textNode->Value(), sizeof(textBuf));
        textBuf[sizeof(textBuf) - 1] = '\0';
        if (ImGui::InputTextMultiline("Text", textBuf, sizeof(textBuf))) {
            textNode->SetValue(textBuf);
            ctx->xml_modified = true;
        }
        if (ImGui::Button("Delete this node")) {
            tinyxml2::XMLNode* parent = node->Parent();
            if (parent)
                parent->DeleteChild(node);
            g_selectedNode = nullptr;
            ctx->xml_modified = true;
            return;
        }
    }
    else if (commentNode) {
        // Редактирование комментария
        ImGui::Text("Comment");
        static char commentBuf[1024];
        strncpy(commentBuf, commentNode->Value(), sizeof(commentBuf));
        commentBuf[sizeof(commentBuf) - 1] = '\0';
        if (ImGui::InputTextMultiline("##comment", commentBuf, sizeof(commentBuf))) {
            commentNode->SetValue(commentBuf);
            ctx->xml_modified = true;
        }
        if (ImGui::Button("Delete this comment")) {
            tinyxml2::XMLNode* parent = node->Parent();
            if (parent)
                parent->DeleteChild(node);
            g_selectedNode = nullptr;
            ctx->xml_modified = true;
            return;
        }
    }
    else if (declNode) {
        ImGui::Text("XML Declaration (cannot be edited)");
    }
}

// ------------------------------------------------------------------
// Главная функция редактора
// ------------------------------------------------------------------
void ShowWidgetEditor(bool* p_open, MainContext* ctx) {
    if (!ctx->CurrentXMLDoc) {
        ImGui::OpenPopup("No XML loaded");
        if (ImGui::BeginPopupModal("No XML loaded", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Please open an XML file first.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        return;
    }

    if (g_selectedNode && !ui_safety::ContainsXMLNode(ctx->CurrentXMLDoc, g_selectedNode)) {
        g_selectedNode = nullptr;   // узел из старого документа, сбрасываем выбор
    }

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Widget Editor", p_open)) {
        ImGui::End();
        return;
    }

    // Левая панель: дерево XML
    if (ImGui::BeginChild("Tree", ImVec2(300, -ImGui::GetFrameHeightWithSpacing() - 30), true)) {
        tinyxml2::XMLNode* root = ctx->CurrentXMLDoc->FirstChild();
        while (root) {
            ShowXMLNode(root, 0);
            root = root->NextSibling();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Правая панель: свойства
    if (ImGui::BeginChild("Properties", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 30), true)) {
        ShowNodeProperties(g_selectedNode, ctx->CurrentXMLDoc, ctx);
    }
    ImGui::EndChild();

    // Панель инструментов снизу
    if (ImGui::Button("Add Window", ImVec2(120, 0))) {
        tinyxml2::XMLElement* newWindow = WidgetTemplateFactory::CreateNewWindow(ctx->CurrentXMLDoc);
        if (newWindow) {
            ctx->CurrentXMLDoc->InsertEndChild(newWindow);
            ctx->xml_modified = true;
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Widget", ImVec2(120, 0))) {
        ImGui::OpenPopup("AddWidgetPopup");
    }
    ImGui::SameLine();

    if (ImGui::Button("Save", ImVec2(120, 0))) {
        if (ctx->CurrentXMLDoc->SaveFile(ctx->File_path) == tinyxml2::XML_SUCCESS) {
            ClearAllWindows();
            LoadWindowsFromXML(ctx, ctx->File_path);
            ctx->xml_modified = false;
            ImGui::OpenPopup("Saved");
        }
        else {
            ImGui::OpenPopup("Save Error");
        }
    }

    // Попап для добавления нового корневого виджета (в выбранный узел или в Items окна)
    if (ImGui::BeginPopup("AddWidgetPopup")) {
        ImGui::Text("Select widget type:");
        ImGui::Separator();

        static const std::set<std::string> multipleElements = {
            "Line", "Zone", "Combo", "Radio", "Segment", "SameLine", "Spacing", "Separator"
        };

        for (const auto& pair : WidgetTemplateFactory::GetRootCreators()) {
            // Находим родителя, куда будет вставлен виджет
            tinyxml2::XMLElement* parent = FindInsertionParent(ctx->CurrentXMLDoc, pair.first.c_str());
            if (parent) {
                // Собираем имена уже существующих дочерних элементов
                std::set<std::string> existingNames;
                for (tinyxml2::XMLNode* child = parent->FirstChild(); child; child = child->NextSibling()) {
                    if (child->ToElement()) {
                        existingNames.insert(child->ToElement()->Name());
                    }
                }

                bool isMultiple = (multipleElements.find(pair.first) != multipleElements.end());
                if (isMultiple || existingNames.find(pair.first) == existingNames.end()) {
                    if (ImGui::Selectable(pair.first.c_str())) {
                        tinyxml2::XMLElement* newElem = pair.second(ctx->CurrentXMLDoc);
                        if (newElem) {
                            // Определяем правильный контейнер внутри родителя
                            tinyxml2::XMLElement* container = GetContainerForChildren(parent, ctx->CurrentXMLDoc, pair.first.c_str());
                            container->InsertEndChild(newElem);
                            ctx->xml_modified = true;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    // Всплывающие уведомления
    if (ImGui::BeginPopupModal("Saved", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File saved successfully.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ctx->file_loaded = false; // сбрасываем флаг, чтобы при следующем открытии файла загрузить его заново
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Save Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Error saving file.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}