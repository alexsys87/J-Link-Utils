// WidgetTemplateFactory.cpp
#include "WidgetTemplateFactory.h"
#include "XMLSafety.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace WidgetTemplateFactory {

    namespace {

        using K = PropKind;

        PropDef P(const char* name, PropKind kind, const char* value, const char* help = nullptr) {
            PropDef p;
            p.name = name;
            p.kind = kind;
            p.defaultValue = value;
            p.help = help;
            return p;
        }

        PropDef E(const char* name, const char* value, std::vector<EnumOption> options, const char* help = nullptr) {
            PropDef p = P(name, K::Enum, value, help);
            p.options = std::move(options);
            return p;
        }

        // Read by the loader, but not written into new widgets (older formats, aliases).
        PropDef Legacy(PropDef p) {
            p.inTemplate = false;
            return p;
        }

        PropDef Attr(PropDef p) {
            p.attribute = true;
            return p;
        }

        // Properties shared by many widgets
        PropDef Text(const char* value) { return P("Text", K::Text, value); }
        PropDef Addr(const char* name = "Address") { return P(name, K::Address, "0x20000000", "Target address: 0x20000000, #20000000 or decimal"); }
        PropDef LE() { return P("LittleEndian", K::Bool, "true", "false swaps the bytes (big-endian data)"); }
        PropDef Width(const char* value) { return P("Width", K::Float, value); }
        PropDef Height(const char* value) { return P("Height", K::Float, value); }
        PropDef MinF(const char* value) { return P("Min", K::Float, value); }
        PropDef MaxF(const char* value) { return P("Max", K::Float, value); }
        PropDef Format(const char* value) { return P("Format", K::Text, value, "printf format for one value, e.g. %d or %.2f"); }

        PropDef DataType(const char* value) {
            return E("DataType", value, {
                {"0", "BOOLEAN"}, {"1", "S08"}, {"2", "U08"}, {"3", "S16"}, {"4", "U16"},
                {"5", "S32"}, {"6", "U32"}, {"7", "FLOAT"}, {"8", "FLOAT_INV (byte order swapped)"},
                {"9", "S64"}, {"10", "U64"} });
        }

        PropDef WriteMode(const char* value) {
            return E("Mode", value, {
                {"0", "Set: write Value on click"}, {"1", "Toggle bit 0"},
                {"2", "Write: write Value on click"}, {"3", "Momentary: Value while held, 0 on release"} });
        }

        NodeDef N(const char* tag, std::vector<PropDef> props, const char* listTag = nullptr,
            const char* itemTag = nullptr, std::vector<Sample> samples = {}, bool listed = true) {
            NodeDef d;
            d.tag = tag;
            d.props = std::move(props);
            d.listTag = listTag;
            d.itemTag = itemTag;
            d.samples = std::move(samples);
            d.listed = listed;
            return d;
        }

        // Containers hold any widget in <Items>.
        NodeDef Container(const char* tag, std::vector<PropDef> props, bool listed = true) {
            return N(tag, std::move(props), "Items", nullptr, {}, listed);
        }

        std::vector<PropDef> CaptureProps(bool histogram) {
            std::vector<PropDef> p = {
                Text(histogram ? "Histogram" : "DataLog"), Format("%.2f"), MinF("0"), MaxF("100"),
                Width("300"), Height("150"),
                P("ScaleX", K::Float, "1", "X distance between two samples"),
                P("Size", K::Int, "100", "Number of array elements (1..4096)"),
                P("Smooth", K::Int, "0", "Moving average window, 0 = off"),
                P("DoneAddr", K::Address, "0", "Flag set by the MCU when the array is full; 0 = read continuously"),
                LE(), P("AutoSize", K::Bool, "false", "Fit the Y axis to the data") };
            if (histogram) p.push_back(P("BarWidth", K::Float, "0.8"));
            // Single-line format without <Items>
            p.push_back(Legacy(Addr()));
            p.push_back(Legacy(P("ScaleY", K::Float, "1")));
            p.push_back(Legacy(DataType("7")));
            p.push_back(Legacy(P(histogram ? "Color" : "LineColor", K::Color, "#FFFFFFFF")));
            return p;
        }

        std::vector<NodeDef> BuildWidgets() {
            std::vector<NodeDef> w;
            w.push_back(N("Text", { Text("New Text") }));
            w.push_back(N("TextColored", { Text("Colored"), P("Color", K::Color, "#FF00FF") }));
            w.push_back(N("Button", { Text("Button"), Width("80"), Height("40"), Addr(), WriteMode("0"),
                P("Value", K::Int, "1", "16-bit value written by the button"), LE() }));
            w.push_back(N("Checkbox", { Text("Checkbox"), Addr(),
                P("AddrBool", K::Bool, "false", "true: 8-bit flag, false: 16-bit value") }));
            w.push_back(Container("Group", {}));
            w.push_back(Container("TreeNode", { Text("Node") }));
            w.push_back(N("SameLine", { P("OffsetX", K::Float, "0"), P("Spacing", K::Float, "-1", "-1 = default spacing") }));
            w.push_back(N("Spacing", {}));
            w.push_back(N("Separator", {}));
            w.push_back(N("Dummy", { Width("50"), Height("50") }));
            w.push_back(Container("Grid", { P("Row", K::Int, "2"), P("Column", K::Int, "2"), Addr(),
                P("AddressInc", K::Int, "2", "Address step from one cell to the next") }));
            w.push_back(N("PlotLines", { Text("Plot"), MinF("0"), MaxF("100"), Width("-1"), Height("200"),
                P("Freq", K::Float, "100", "Samples per second per line (up to 1000)"),
                P("Size", K::Int, "100", "Points shown in the static plot (1..4096)"),
                P("Smooth", K::Int, "0", "Moving average window, 0 = off"),
                P("MaxSize", K::Int, "2000", "Minimum history length in points"),
                P("AutoSize", K::Bool, "false", "Fit the Y axis to the data"),
                // Single-line format without <Items>
                Legacy(P("Name", K::Text, "")), Legacy(Addr()), Legacy(DataType("7")),
                Legacy(P("Float", K::Bool, "true")), Legacy(LE()),
                Legacy(P("Scale", K::Float, "1")), Legacy(P("BufferId", K::Int, "0")),
                Legacy(P("LineColor", K::Color, "#FFFFFFFF")), Legacy(P("LineWeight", K::Float, "1")) },
                "Items", "Line", { { {"Name", "Line 1"} } }));
            w.push_back(N("TextFormatted", { Format("%d"), Addr(), P("Color", K::Color, "#FFFFFF") }));
            w.push_back(N("TextFormattedF", { Format("%.2f"), Addr(), LE(), P("Color", K::Color, "#FFFFFF") }));
            w.push_back(N("AlignTextToFramePadding", {}));
            w.push_back(N("NewLine", {}));
            w.push_back(N("LabelText", { P("Label", K::Text, "Label"), Format("%.2f"), Addr(), LE() }));
            w.push_back(N("RadioButton", { Addr(), P("InSameLine", K::Bool, "false") }, "Items", "Radio",
                { { {"Label", "Option 1"}, {"Value", "0"} }, { {"Label", "Option 2"}, {"Value", "1"} } }));
            w.push_back(N("Tooltip", { Text("Tooltip text") }));
            w.push_back(N("PlotTooltip", { Text("Tooltip"), Format("%.2f"), MinF("0"), MaxF("100"),
                P("Size", K::Int, "100"), P("BufferId", K::Int, "0", "Plot buffer 0..63"), LE(), Addr(),
                DataType("7"), P("AutoSize", K::Bool, "false") }));
            w.push_back(N("ComboBox", { Text("Combo"), Addr() }, "Items", "Combo",
                { { {"Label", "Item 1"}, {"Value", "0"} }, { {"Label", "Item 2"}, {"Value", "1"} } }));
            w.push_back(N("ImputInt", { Text("Input"), Addr(), P("Min", K::Int, "0"), P("Max", K::Int, "100"),
                P("Step", K::Int, "1"), P("Fast", K::Int, "10") }));
            w.push_back(N("ImputFloat", { Text("Input"), Addr(), MinF("0"), MaxF("100"),
                P("Step", K::Float, "0.1"), P("Fast", K::Float, "1"), Format("%.3f"), LE() }));
            w.push_back(Container("FixedWidth", { Width("100") }));
            w.push_back(N("SetCursorPos", { P("Width", K::Float, "0", "X position"), P("Height", K::Float, "0", "Y position") }));
            w.push_back(N("Indent", { Width("20") }));
            w.push_back(N("Histogram", CaptureProps(true), "Items", "Line", { { {"Name", "Data"} } }));
            w.push_back(N("DataLog", CaptureProps(false), "Items", "Line", { { {"Name", "Data"} } }));
            w.push_back(N("ProgressBar", { Width("200"), Height("20"), Addr(), LE() }));
            w.push_back(N("SliderFloat", { Text("Slider"), Addr(), MinF("0"), MaxF("1"),
                P("Step", K::Float, "0", "0 = continuous"), Format("%.3f"), LE() }));
            w.push_back(N("SliderInt", { Text("Slider"), Addr(), P("Min", K::Int, "0"), P("Max", K::Int, "100"), Format("%d") }));
            w.push_back(N("TabBar", { Text("TabBar") }, "Items", "Tab", { { {"Text", "Tab 1"} } }));
            w.push_back(Container("Tab", { Text("Tab") }, false));
            w.push_back(Container("Column", { P("Count", K::Int, "2", "Number of columns (1..64)") }));
            w.push_back(N("SelectableText", { Format("%d"), Addr(),
                P("AddrBool", K::Bool, "false", "true: 8-bit flag, false: 16-bit value") }));
            w.push_back(Container("ChildWindow", { Text("Child"), Width("200"), Height("150") }));
            w.push_back(Container("Header", { Text("Header") }));
            w.push_back(Container("Popup", {}));
            w.push_back(Container("HintBox", {}));
            w.push_back(N("HelpMarker", { Text("Help text") }));
            w.push_back(N("Watch", {}));
            w.push_back(N("SevenSegment", { Text("7Seg"), Addr(), Width("80"), Height("40"), LE() }));
            w.push_back(N("MechanicalSwitch2", { Text("Switch"), Addr(), Width("40"), Height("40"), LE() }));
            w.push_back(N("Gauge", { Text("Gauge"), Addr(), MinF("0"), MaxF("200"), P("Radius", K::Float, "100"),
                P("Unit", K::Text, ""), LE() }));
            w.push_back(N("GlossyDisplay", { Text("Display"), Addr(), Width("120"), Height("60"), LE(), DataType("4"),
                P("HexMode", K::Bool, "false"), P("Digits", K::Int, "0", "Leading zeros, 0 = none"),
                P("DecimalPlaces", K::Int, "2", "Digits after the point for FLOAT"),
                Legacy(P("Hex", K::Bool, "false", "Old name of HexMode")) }));
            w.push_back(N("LCD_HD44780", { Text("LCD"), Addr("Line1Address"), P("Line1Text", K::Text, "Hello"),
                Addr("Line2Address"), P("Line2Text", K::Text, "World"), P("PixelSize", K::Float, "2.5"), LE(),
                P("UseFixed", K::Bool, "false", "Show Line1Text/Line2Text instead of memory") }));
            w.push_back(N("VerticalGauge", { Text("V Gauge"), Addr(), MinF("0"), MaxF("100"), Width("120"), Height("300"),
                LE(), DataType("7") }));
            w.push_back(N("NeumorphicKnob", { Text("Knob"), Addr(), MinF("0"), MaxF("100"), P("Radius", K::Float, "50"),
                LE(), DataType("3") }));
            w.push_back(N("StyledKnob", { Text("Knob"), Addr(), MinF("0"), MaxF("100"), P("Radius", K::Float, "50"),
                LE(), DataType("3") }));
            w.push_back(N("CustomVerticalSlider", { Text("VSlider"), Addr(), MinF("0"), MaxF("1"), Width("20"), Height("100"),
                LE(), DataType("7") }));
            w.push_back(N("BitControl8Bit", { Text("Bits"), Addr(), LE(), P("Scale", K::Float, "0.55") }));
            w.push_back(N("BitControl", { Text("BitCtrl"), Addr(),
                E("Mode", "1", { {"1", "8 bits"}, {"2", "16 bits"}, {"4", "32 bits"} }), LE(),
                P("Switchable", K::Bool, "true", "Allow switching the width at run time") }));
            w.push_back(N("RotarySwitch", { Text("Rotary"), Addr(), P("Min", K::Int, "0"), P("Max", K::Int, "3"),
                P("Radius", K::Float, "40"), LE() }));
            w.push_back(N("LedIndicator", { Text("LED"), Addr(), P("Radius", K::Float, "10"),
                P("ColorOn", K::Color, "#FF0000"), P("ColorOff", K::Color, "#320000"), LE() }));
            w.push_back(N("Image", { Text("Image"), P("File", K::Text, "image.png", "File next to the program"),
                P("Scale", K::Float, "1"), P("Width", K::Float, "0", "0 = image size"), P("Height", K::Float, "0", "0 = image size"),
                P("Tint", K::Color, "#FFFFFFFF"), P("Border", K::Color, "#00000000"),
                P("UV0", K::Point, "0,0"), P("UV1", K::Point, "1,1") }));
            w.push_back(N("Gif", { Text("GIF"), P("File", K::Text, "animation.gif", "File next to the program"),
                P("Scale", K::Float, "1"), P("Width", K::Float, "0", "0 = image size"), P("Height", K::Float, "0", "0 = image size"),
                P("Loop", K::Bool, "true"), P("Speed", K::Float, "1") }));
            w.push_back(N("RPMIndicator", { Text("RPM"), Addr(), MinF("0"), MaxF("8000"), P("Radius", K::Float, "100"),
                P("Unit", K::Text, "rpm"), LE() }, "Zones", "Zone",
                { { {"start", "0"}, {"end", "4000"}, {"color", "#64C850"} },
                  { {"start", "4000"}, {"end", "6000"}, {"color", "#F0F050"} },
                  { {"start", "6000"}, {"end", "8000"}, {"color", "#E64646"} } }));
            w.push_back(N("Keypad", { Text("Keypad"), Addr(), P("Min", K::Int, "0"), P("Max", K::Int, "9999"),
                P("Digits", K::Int, "4"), LE() }));
            w.push_back(N("Equalizer", { Text("EQ"), Addr(), P("Bands", K::Int, "10", "1..64"), MinF("0"), MaxF("100"),
                P("BandWidth", K::Float, "20"), Height("120"),
                E("DataType", "float", { {"float", "float (32-bit)"}, {"int16", "int16 (16-bit)"} }), LE(),
                P("AddressStep", K::Int, "4", "Bytes between bands: 4 for float, 2 for int16") }));
            w.push_back(N("Tank", { Text("Tank"), Addr(), MinF("0"), MaxF("100"), Width("150"), Height("250"),
                P("Unit", K::Text, "l"), LE(), P("LiquidColor", K::Color, "#40A4DF"), P("MetalColor", K::Color, "#A0A0AA"),
                E("DataType", "0", { {"0", "FLOAT"}, {"1", "U16"}, {"2", "S16"}, {"3", "U32"}, {"4", "S32"} }) }));
            w.push_back(N("VUMeter", { Text("VU"), Addr(), MinF("0"), MaxF("100"),
                E("Orientation", "horizontal", { {"horizontal", "horizontal"}, {"vertical", "vertical"} }),
                Width("200"), Height("30"), P("Unit", K::Text, ""), LE(), P("ShowPeak", K::Bool, "false") }));
            w.push_back(N("SegmentButton", { Text("Segment"), Addr(), LE() }, "Items", "Segment",
                { { {"Text", "Low"}, {"Value", "0"} }, { {"Text", "Med"}, {"Value", "1"} }, { {"Text", "High"}, {"Value", "2"} } }));
            w.push_back(N("GlossyButton", { Text("Button"), Addr(), LE(), WriteMode("1"),
                P("Value", K::Int, "1", "16-bit value written by the button"), P("Radius", K::Float, "20") }));
            w.push_back(N("MemoryView", { Text("Memory"), Addr("BaseAddress"), P("NumBytes", K::Int, "256"),
                P("BytesPerLine", K::Int, "16"), LE() }));
            w.push_back(N("MemoryEditor", { Text("Memory Editor"), Addr("BaseAddress"), P("Size", K::Int, "256"),
                P("Cols", K::Int, "16"), P("ReadOnly", K::Bool, "false"), P("ShowAscii", K::Bool, "true"),
                P("ShowHexII", K::Bool, "false"), P("ShowOptions", K::Bool, "true"), P("ShowPreview", K::Bool, "true"),
                P("GreyZero", K::Bool, "true"), P("UpperHex", K::Bool, "true"), LE(),
                P("UseExternal", K::Bool, "false"), P("UseCache", K::Bool, "true") }));
            return w;
        }

        struct ItemDef {
            const char* owner;
            NodeDef node;
        };

        std::vector<ItemDef> BuildItems() {
            const NodeDef captureLine = N("Line", { P("Name", K::Text, "Data"), Addr(), P("ScaleY", K::Float, "1"),
                P("Color", K::Color, "#FFFFFFFF"), DataType("7") });
            return {
                { "PlotLines", N("Line", { P("Name", K::Text, "Line"), Addr(), DataType("7"), LE(),
                    P("Scale", K::Float, "1"), P("BufferId", K::Int, "0", "Plot buffer 0..63"),
                    P("LineColor", K::Color, "#FFFFFFFF"), P("LineWeight", K::Float, "1") }) },
                { "DataLog", captureLine },
                { "Histogram", captureLine },
                { "RadioButton", N("Radio", { P("Label", K::Text, "Option"), P("Value", K::Int, "0") }) },
                { "ComboBox", N("Combo", { P("Label", K::Text, "Item"), P("Value", K::Int, "0") }) },
                { "SegmentButton", N("Segment", { Text("Segment"), P("Value", K::Int, "0") }) },
                { "RPMIndicator", N("Zone", { Attr(P("start", K::Float, "0")), Attr(P("end", K::Float, "1000")),
                    Attr(P("color", K::ColorRGB, "#FF0000")) }) },
            };
        }

        const std::vector<ItemDef>& Items() {
            static const std::vector<ItemDef> items = BuildItems();
            return items;
        }

        bool IsTag(const tinyxml2::XMLElement* e, const char* tag) {
            return e && tag && std::strcmp(e->Name(), tag) == 0;
        }

        void SetValue(tinyxml2::XMLElement* owner, const PropDef& prop, const char* value) {
            if (prop.attribute) {
                owner->SetAttribute(prop.name, value);
            }
            else {
                tinyxml2::XMLElement* e = owner->GetDocument()->NewElement(prop.name);
                e->SetText(value);
                owner->InsertEndChild(e);
            }
        }

        // Writes the properties; values in the sample replace the defaults.
        void FillProperties(tinyxml2::XMLElement* e, const NodeDef& def, const Sample* sample) {
            for (const PropDef& prop : def.props) {
                const char* value = prop.defaultValue;
                bool found = false;
                if (sample) {
                    for (const auto& kv : *sample) {
                        if (std::strcmp(kv.first, prop.name) == 0) { value = kv.second; found = true; }
                    }
                }
                if (prop.inTemplate || found) SetValue(e, prop, value);
            }
        }

        tinyxml2::XMLElement* CreateItemFrom(tinyxml2::XMLDocument* doc, const NodeDef& owner, const Sample* sample) {
            if (!owner.listTag || !owner.itemTag) return nullptr;
            const NodeDef* item = FindItem(owner.tag, owner.itemTag);
            if (!item) return nullptr;
            tinyxml2::XMLElement* e = doc->NewElement(item->tag);
            FillProperties(e, *item, sample);
            if (item->listTag) e->InsertEndChild(doc->NewElement(item->listTag));
            return e;
        }

        bool IsWidgetList(const tinyxml2::XMLElement* e) {
            if (!e) return false;
            const tinyxml2::XMLElement* owner = e->Parent() ? e->Parent()->ToElement() : nullptr;
            const NodeDef* def = owner ? DefinitionOf(owner) : nullptr;
            return def && def->listTag && !def->itemTag && IsTag(e, def->listTag);
        }
    } // namespace

    const std::vector<NodeDef>& Widgets() {
        static const std::vector<NodeDef> widgets = BuildWidgets();
        return widgets;
    }

    const NodeDef* FindWidget(const char* tag) {
        if (!tag) return nullptr;
        for (const NodeDef& d : Widgets())
            if (std::strcmp(d.tag, tag) == 0) return &d;
        return nullptr;
    }

    const NodeDef* FindItem(const char* ownerTag, const char* itemTag) {
        if (!ownerTag || !itemTag) return nullptr;
        for (const ItemDef& d : Items())
            if (std::strcmp(d.owner, ownerTag) == 0 && std::strcmp(d.node.tag, itemTag) == 0) return &d.node;
        // TabBar holds Tab widgets
        const NodeDef* owner = FindWidget(ownerTag);
        if (owner && owner->itemTag && std::strcmp(owner->itemTag, itemTag) == 0) return FindWidget(itemTag);
        return nullptr;
    }

    const NodeDef& WindowDef() {
        static const NodeDef window = N("Window", { Text("New Window") }, "Items");
        return window;
    }

    const NodeDef* DefinitionOf(const tinyxml2::XMLElement* element) {
        if (!element) return nullptr;
        const tinyxml2::XMLNode* parentNode = element->Parent();
        const tinyxml2::XMLElement* parent = parentNode ? parentNode->ToElement() : nullptr;
        if (!parent) return IsTag(element, "Window") ? &WindowDef() : nullptr;
        const tinyxml2::XMLNode* ownerNode = parent->Parent();
        const tinyxml2::XMLElement* owner = ownerNode ? ownerNode->ToElement() : nullptr;
        const NodeDef* ownerDef = owner ? DefinitionOf(owner) : nullptr;
        if (!ownerDef || !ownerDef->listTag || !IsTag(parent, ownerDef->listTag)) return nullptr;
        if (!ownerDef->itemTag) return FindWidget(element->Name());
        return IsTag(element, ownerDef->itemTag) ? FindItem(ownerDef->tag, element->Name()) : nullptr;
    }

    const PropDef* FindProp(const NodeDef* def, const char* name) {
        if (!def || !name) return nullptr;
        for (const PropDef& p : def->props)
            if (std::strcmp(p.name, name) == 0) return &p;
        return nullptr;
    }

    tinyxml2::XMLElement* CreateNode(tinyxml2::XMLDocument* doc, const NodeDef& def) {
        tinyxml2::XMLElement* e = doc->NewElement(def.tag);
        FillProperties(e, def, nullptr);
        if (def.listTag) {
            tinyxml2::XMLElement* list = doc->NewElement(def.listTag);
            for (const Sample& sample : def.samples) {
                tinyxml2::XMLElement* item = CreateItemFrom(doc, def, &sample);
                if (item) list->InsertEndChild(item);
            }
            e->InsertEndChild(list);
        }
        return e;
    }

    tinyxml2::XMLElement* CreateNewWindow(tinyxml2::XMLDocument* doc) {
        return CreateNode(doc, WindowDef());
    }

    tinyxml2::XMLElement* CreateProperty(tinyxml2::XMLDocument* doc, const PropDef& prop) {
        tinyxml2::XMLElement* e = doc->NewElement(prop.name);
        e->SetText(prop.defaultValue);
        return e;
    }

    tinyxml2::XMLElement* CreateItem(tinyxml2::XMLDocument* doc, const NodeDef& owner) {
        return CreateItemFrom(doc, owner, nullptr);
    }

    tinyxml2::XMLElement* EnsureList(tinyxml2::XMLElement* owner, const NodeDef& def) {
        if (!owner || !def.listTag) return nullptr;
        tinyxml2::XMLElement* list = owner->FirstChildElement(def.listTag);
        if (!list) {
            list = owner->GetDocument()->NewElement(def.listTag);
            owner->InsertEndChild(list);
        }
        return list;
    }

    tinyxml2::XMLElement* AddProperty(tinyxml2::XMLElement* owner, const PropDef& prop) {
        if (!owner) return nullptr;
        if (prop.attribute) {
            owner->SetAttribute(prop.name, prop.defaultValue);
            return owner;
        }
        tinyxml2::XMLElement* e = CreateProperty(owner->GetDocument(), prop);
        // Keep the properties in front of the list, as the templates do.
        tinyxml2::XMLElement* list = nullptr;
        const NodeDef* def = DefinitionOf(owner);
        if (def && def->listTag) list = owner->FirstChildElement(def->listTag);
        tinyxml2::XMLNode* before = list ? list->PreviousSibling() : nullptr;
        if (!list) owner->InsertEndChild(e);
        else if (before) owner->InsertAfterChild(before, e);
        else owner->InsertFirstChild(e);
        return e;
    }

    std::vector<const PropDef*> MissingProperties(const tinyxml2::XMLElement* element, const NodeDef& def) {
        std::vector<const PropDef*> missing;
        if (!element) return missing;
        for (const PropDef& p : def.props) {
            const bool present = p.attribute ? element->Attribute(p.name) != nullptr
                                             : element->FirstChildElement(p.name) != nullptr;
            if (!present) missing.push_back(&p);
        }
        return missing;
    }

    bool InsertWidget(tinyxml2::XMLDocument* doc, tinyxml2::XMLElement* selected, tinyxml2::XMLElement* widget) {
        if (!doc || !widget) return false;
        tinyxml2::XMLElement* child = nullptr;   // previous element on the way up
        for (tinyxml2::XMLElement* e = selected; e; child = e, e = e->Parent() ? e->Parent()->ToElement() : nullptr) {
            // A widget list (<Items> of a container or window)
            if (IsWidgetList(e)) {
                if (child) e->InsertAfterChild(child, widget);
                else e->InsertEndChild(widget);
                return true;
            }
            const NodeDef* def = DefinitionOf(e);
            // A container or window: append to its list
            if (def && def->listTag && !def->itemTag && e == selected) {
                EnsureList(e, *def)->InsertEndChild(widget);
                return true;
            }
            // A widget inside a list: insert right after it
            tinyxml2::XMLElement* parent = e->Parent() ? e->Parent()->ToElement() : nullptr;
            if (def && parent && IsWidgetList(parent)) {
                parent->InsertAfterChild(e, widget);
                return true;
            }
        }
        tinyxml2::XMLElement* window = doc->FirstChildElement("Window");
        if (!window) return false;
        EnsureList(window, WindowDef())->InsertEndChild(widget);
        return true;
    }

    bool ParseAddress(const char* text, uint32_t* out) {
        return ui_safety::ParseAddressText(text, out);
    }

    bool ParseColor(const char* text, float rgba[4]) {
        if (!text || text[0] != '#') return false;
        const size_t len = std::strlen(text + 1);
        if (len != 6 && len != 8) return false;
        for (size_t i = 1; i <= len; ++i)
            if (!std::isxdigit(static_cast<unsigned char>(text[i]))) return false;
        const unsigned long hex = std::strtoul(text + 1, nullptr, 16);
        rgba[0] = ((hex >> 16) & 0xFF) / 255.0f;
        rgba[1] = ((hex >> 8) & 0xFF) / 255.0f;
        rgba[2] = (hex & 0xFF) / 255.0f;
        rgba[3] = (len == 8) ? ((hex >> 24) & 0xFF) / 255.0f : 1.0f;
        return true;
    }

    std::string FormatColor(const float rgba[4], bool withAlpha) {
        unsigned int c[4];
        for (int i = 0; i < 4; ++i) {
            const float v = std::isfinite(rgba[i]) ? rgba[i] : 0.0f;
            c[i] = (unsigned int)(std::fmin(std::fmax(v, 0.0f), 1.0f) * 255.0f + 0.5f);
        }
        char buf[16];
        if (withAlpha) std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c[3], c[0], c[1], c[2]);
        else std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c[0], c[1], c[2]);
        return buf;
    }

    std::string FormatFloat(float value) {
        char buf[32];
        for (int precision = 6; precision <= 9; ++precision) {
            std::snprintf(buf, sizeof(buf), "%.*g", precision, (double)value);
            if (std::strtof(buf, nullptr) == value) break;
        }
        return buf;
    }

} // namespace WidgetTemplateFactory
