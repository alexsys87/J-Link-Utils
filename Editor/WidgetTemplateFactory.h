// WidgetTemplateFactory.h
// Description of every element the XML loader (xml_window.cpp) understands: widgets,
// their properties with types and default values, and the list items some widgets hold
// (plot lines, combo entries, zones...). The XML editor builds new widgets, offers
// missing properties and picks an input control for each value from this table.
#pragma once

#include <tinyxml2/tinyxml2.h>
#include <cstdint>
#include <string>
#include <vector>

namespace WidgetTemplateFactory {

    enum class PropKind {
        Text,       // any string
        Int,        // integer
        Float,      // floating point number
        Bool,       // true / false
        Color,      // #RRGGBB or #AARRGGBB
        ColorRGB,   // #RRGGBB, alpha is ignored by the loader
        Address,    // target address: 0x20000000, #20000000 or decimal
        Enum,       // one of PropDef::options
        Point       // "x,y"
    };

    struct EnumOption {
        const char* value;   // text stored in the XML
        const char* label;   // text shown in the editor
    };

    struct PropDef {
        const char* name;
        PropKind kind;
        const char* defaultValue;
        bool inTemplate = true;             // written into newly created elements
        bool attribute = false;             // stored as an XML attribute instead of a child element
        std::vector<EnumOption> options;    // PropKind::Enum
        const char* help = nullptr;         // tooltip in the editor
    };

    // Overrides of item property values for the sample items of a new widget.
    using Sample = std::vector<std::pair<const char*, const char*>>;

    struct NodeDef {
        const char* tag;
        std::vector<PropDef> props;
        const char* listTag = nullptr;      // child element holding the list ("Items", "Zones"), if any
        const char* itemTag = nullptr;      // element type in that list; nullptr means any widget
        std::vector<Sample> samples;        // list items written into a new widget
        bool listed = true;                 // offered in the "add widget" list
    };

    // All widget types the loader supports.
    const std::vector<NodeDef>& Widgets();
    const NodeDef* FindWidget(const char* tag);
    // List item of a widget, e.g. ("PlotLines", "Line") or ("RPMIndicator", "Zone").
    const NodeDef* FindItem(const char* ownerTag, const char* itemTag);
    // The top-level <Window> element.
    const NodeDef& WindowDef();
    // Definition of an element from its place in the document: a window, a widget inside
    // a widget list, or an item inside an item list. nullptr for properties and unknown elements.
    const NodeDef* DefinitionOf(const tinyxml2::XMLElement* element);
    const PropDef* FindProp(const NodeDef* def, const char* name);

    // New elements filled with the default values.
    tinyxml2::XMLElement* CreateNode(tinyxml2::XMLDocument* doc, const NodeDef& def);
    tinyxml2::XMLElement* CreateNewWindow(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateProperty(tinyxml2::XMLDocument* doc, const PropDef& prop);
    // A new list item for the owner (a line, a combo entry, a tab...), nullptr if it has none.
    tinyxml2::XMLElement* CreateItem(tinyxml2::XMLDocument* doc, const NodeDef& owner);

    // Editing helpers shared by the editor and the tests.
    // The owner's list element (<Items>/<Zones>), created when missing.
    tinyxml2::XMLElement* EnsureList(tinyxml2::XMLElement* owner, const NodeDef& def);
    // Adds a missing property with its default value in front of the list element.
    tinyxml2::XMLElement* AddProperty(tinyxml2::XMLElement* owner, const PropDef& prop);
    // Properties of the definition the element does not have yet.
    std::vector<const PropDef*> MissingProperties(const tinyxml2::XMLElement* element, const NodeDef& def);
    // Inserts a new widget next to the selection: into a selected container, after a selected
    // widget, or at the end of the first window. Returns false if there is no place for it.
    bool InsertWidget(tinyxml2::XMLDocument* doc, tinyxml2::XMLElement* selected, tinyxml2::XMLElement* widget);

    // Value parsing as the loader does it.
    bool ParseAddress(const char* text, uint32_t* out);
    // "#RRGGBB" / "#AARRGGBB" to RGBA components 0..1 (r, g, b, a).
    bool ParseColor(const char* text, float rgba[4]);
    std::string FormatColor(const float rgba[4], bool withAlpha);
    // Shortest text that reads back as the same float.
    std::string FormatFloat(float value);

} // namespace WidgetTemplateFactory
