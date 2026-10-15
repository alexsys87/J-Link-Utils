// xml_window.cpp
#include "widget_common.h"
#include "Editor/XMLSafety.h"
#include "Editor/WidgetTemplateFactory.h"
#include "Editor/DesignMode.h"
#include <cctype>
#include <cfloat>
#include <initializer_list>
#include "Preferences.h"

// All widgets
#include "TextWidget.h"
#include "TextColoredWidget.h"
#include "ButtonWidget.h"
#include "CheckboxWidget.h"
#include "GroupWidget.h"
#include "TreeNodeWidget.h"
#include "SameLineWidget.h"
#include "SpacingWidget.h"
#include "SeparatorWidget.h"
#include "DummyWidget.h"
#include "GridWidget.h"
#include "PlotLinesWidget.h"
#include "TextFormattedWidget.h"
#include "TextFormattedFWidget.h"
#include "AlignTextWidget.h"
#include "NewLineWidget.h"
#include "LabelTextWidget.h"
#include "RadioButtonGroup.h"
#include "TooltipWidget.h"
#include "PlotTooltipWidget.h"
#include "ComboBoxWidget.h"
#include "InputIntWidget.h"
#include "InputFloatWidget.h"
#include "FixedWidthWidget.h"
#include "SetCursorPosWidget.h"
#include "IndentWidget.h"
#include "HistogramWidget.h"
#include "DataLogWidget.h"
#include "ProgressBarWidget.h"
#include "SliderFloatWidget.h"
#include "SliderIntWidget.h"
#include "TabBarWidget.h"
#include "TabItemWidget.h"
#include "ColumnWidget.h"
#include "SelectableTextWidget.h"
#include "ChildWindowWidget.h"
#include "HeaderWidget.h"
#include "PopupWidget.h"
#include "HintBoxWidget.h"
#include "HelpMarkerWidget.h"
#include "WatchWidget.h"
#include "SevenSegmentWidget.h"
#include "MechanicalSwitch2Widget.h"
#include "GaugeWidget.h"
#include "GlossyDisplayWidget.h"
#include "LCD_HD44780_Widget.h"
#include "VerticalGaugeWidget.h"
#include "StyledKnobWidget.h"
#include "NeumorphicKnobWidget.h"
#include "CustomVerticalSliderWidget.h"
#include "BitControlWidget.h"
#include "BitControl8BitWidget.h"
#include "RotarySwitchWidget.h"
#include "LedIndicatorWidget.h"
#include "ImageWidget.h"
#include "GifWidget.h"
#include "RPMIndicatorWidget.h"
#include "KeypadWidget.h"
#include "EqualizerWidget.h"
#include "TankWidget.h"
#include "VUMeterWidget.h"
#include "SegmentButtonWidget.h"
#include "GlossyButtonWidget.h"
#include "MemoryViewWidget.h"
#include "MemoryEditorWidget.h"
#include "PlacedWidget.h"

#include <Main.h>

using namespace tinyxml2;

// Global buffer for the plots
float VirtualBuffer[MAX_BUFF][BUFF_SIZE] = { 0 };

std::vector<WindowData> g_Windows;
void SaveDashboardWindowVisibility();

// Document the windows were built from in design mode, and its revision at that time.
// nullptr when they were loaded from the file.
static const XMLDocument* g_WindowsDoc = nullptr;
static uint32_t g_WindowsRevision = 0;
// Set while building from the open document: the widgets keep their elements as keys.
static bool g_KeepSources = false;
static std::string g_PreferenceDocument;
static std::vector<std::string> g_WindowPreferenceKeys;

using ui_safety::XmlChildText;

// Parses "#RRGGBB" or "#AARRGGBB" (the format the XML editor writes and the other
// widgets read) and packs it as 0xRRGGBBAA, the layout ColorConverter() expects.
static bool ParseTextColor(const char* str, unsigned int* rgba) {
    if (!str || str[0] != '#') return false;
    const size_t len = strlen(str + 1);
    if (len != 6 && len != 8) return false;
    for (size_t i = 1; i <= len; ++i) {
        if (!isxdigit(static_cast<unsigned char>(str[i]))) return false;
    }
    const unsigned long value = strtoul(str + 1, nullptr, 16);
    const unsigned int alpha = (len == 8) ? ((value >> 24) & 0xFF) : 0xFF;
    *rgba = (static_cast<unsigned int>(value & 0xFFFFFF) << 8) | alpha;
    return true;
}

// ------------------------------------------------------------------
// Property readers. Values from the XML are untrusted: every number is checked and
// limited to what the widgets can handle, and a missing, malformed or non-finite
// value (tinyxml2 accepts "nan" and "inf") falls back to the default.
// ------------------------------------------------------------------
constexpr float kMaxExtent = 10000.0f;      // widths, heights, offsets in pixels
constexpr float kMaxRadius = 2000.0f;

static float ReadFloat(XMLElement* e, const char* tag, float def,
    float lo = -FLT_MAX / 2, float hi = FLT_MAX / 2) {
    XMLElement* c = e->FirstChildElement(tag);
    float v = def;
    if (!c || c->QueryFloatText(&v) != XML_SUCCESS || !std::isfinite(v)) return def;
    return std::clamp(v, lo, hi);
}

// Sizes: negative values keep their ImGui meaning (fill / align to the right edge).
static float ReadSize(XMLElement* e, const char* tag, float def) {
    return ReadFloat(e, tag, def, -kMaxExtent, kMaxExtent);
}

// Radii and scale factors: zero or negative is replaced by the default.
static float ReadPositive(XMLElement* e, const char* tag, float def, float hi) {
    const float v = ReadFloat(e, tag, def, -FLT_MAX / 2, hi);
    return v > 0.0f ? v : def;
}

static int ReadInt(XMLElement* e, const char* tag, int def,
    int lo = std::numeric_limits<int>::min(), int hi = std::numeric_limits<int>::max()) {
    XMLElement* c = e->FirstChildElement(tag);
    int v = def;
    if (!c || c->QueryIntText(&v) != XML_SUCCESS) return def;
    return std::clamp(v, lo, hi);
}

// One of the listed values, otherwise the default (enums, data types).
static int ReadChoice(XMLElement* e, const char* tag, int def, std::initializer_list<int> allowed) {
    const int v = ReadInt(e, tag, def);
    for (int a : allowed) {
        if (a == v) return v;
    }
    return def;
}

static int ReadDataType(XMLElement* e, const char* tag, int def) {
    return ReadChoice(e, tag, def, { BOOLEAN, S08_DEC, U08_DEC, S16_DEC, U16_DEC, S32_DEC, U32_DEC,
        FLOAT, FLOAT_INV, S64_DEC, U64_DEC });
}

static bool ReadBool(XMLElement* e, const char* tag, bool def) {
    XMLElement* c = e->FirstChildElement(tag);
    bool v = def;
    if (!c || c->QueryBoolText(&v) != XML_SUCCESS) return def;
    return v;
}

static uint32_t ReadAddress(XMLElement* e, const char* tag) {
    return AddressFromHex(e->FirstChildElement(tag));
}

static const char* ReadText(XMLElement* e, const char* tag, const char* def = "") {
    return XmlChildText(e, tag, def);
}

// Preference identity is semantic, not the widget's index or a pointer into the
// document. Reordering unrelated siblings and changing layout/options preserves it.
// No sibling occurrence is included: deleting/reordering another widget must not
// transfer its settings to a survivor. Ambiguous semantic paths are not persisted.
// Length framing avoids collisions in user-controlled labels.
static std::string PreferenceIdentityPart(const std::string& value) {
    return std::to_string(value.size()) + ":" + value;
}

static std::string PreferenceElementSignature(XMLElement* elem) {
    std::string signature = PreferenceIdentityPart(elem->Name());
    for (const char* field : { "Text", "Name", "Address", "DataType", "BufferId" })
        signature += PreferenceIdentityPart(ReadText(elem, field));
    if (std::string(elem->Name()) == "PlotLines") {
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* line = items->FirstChildElement("Line"); line; line = line->NextSiblingElement("Line"))
                signature += PreferenceIdentityPart(PreferenceElementSignature(line));
        }
    }
    return signature;
}

static std::string PreferenceElementCandidateIdentity(XMLElement* elem) {
    if (!elem) return "";
    XMLElement* parent = elem->Parent() ? elem->Parent()->ToElement() : nullptr;
    // <Items> is a list, not a widget. Its first legacy <Text> widget must not
    // become an apparent title/address property of every other item in the list.
    if (strcmp(elem->Name(), "Items") == 0) return PreferenceElementCandidateIdentity(parent);
    const std::string signature = PreferenceElementSignature(elem);
    return PreferenceElementCandidateIdentity(parent) + PreferenceIdentityPart(signature);
}

static int CountPreferenceIdentityMatches(XMLElement* first, const char* tag, const std::string& candidate) {
    int matches = 0;
    for (XMLElement* elem = first; elem; elem = elem->NextSiblingElement()) {
        if (strcmp(elem->Name(), tag) == 0 && PreferenceElementCandidateIdentity(elem) == candidate) ++matches;
        if (matches > 1) return matches;
        matches += CountPreferenceIdentityMatches(elem->FirstChildElement(), tag, candidate);
        if (matches > 1) return matches;
    }
    return matches;
}

static std::string PreferenceElementIdentity(XMLElement* elem) {
    if (!elem) return "";
    const std::string candidate = PreferenceElementCandidateIdentity(elem);
    // Even distinct widgets can have indistinguishable semantic paths (for
    // example matching plots in anonymous Groups). Do not guess by list index.
    const std::string ambiguousKey = prefs::ScopedKey(g_PreferenceDocument, "Identity", candidate) + ".Ambiguous";
    if (CountPreferenceIdentityMatches(elem->GetDocument()->FirstChildElement(), elem->Name(), candidate) != 1) {
        // Remember ambiguity across rebuilds and restarts: if A had saved prefs,
        // then matching B was added and A deleted, B must not inherit A's key.
        // This only stages a UI-store tombstone; reading never changes the XML.
        prefs::WriteBool(ambiguousKey, true);
        return "";
    }
    return prefs::ReadBool(ambiguousKey, false) ? "" : candidate;
}

// "#RRGGBB" or "#AARRGGBB" as an IM_COL32 color
static ImU32 ReadColor(XMLElement* e, const char* tag, ImU32 def) {
    unsigned int rgba = 0;
    if (!ParseTextColor(XmlChildText(e, tag, nullptr), &rgba)) return def;
    return IM_COL32((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
}

static ImVec2 ReadPoint(XMLElement* e, const char* tag, ImVec2 def) {
    const char* text = XmlChildText(e, tag, nullptr);
    ImVec2 p = def;
    if (!text || sscanf(text, "%f,%f", &p.x, &p.y) != 2 || !std::isfinite(p.x) || !std::isfinite(p.y)) return def;
    return p;
}

// Scales need min < max: a reversed range is swapped, an empty one widened.
static void OrderRange(float& lo, float& hi) {
    if (hi < lo) std::swap(lo, hi);
    if (hi == lo) hi = lo + 1.0f;
}

static void OrderRange(int& lo, int& hi) {
    if (hi < lo) std::swap(lo, hi);
}

static std::unique_ptr<UIWidget> LoadWidgetFromXML(XMLElement* elem);
static std::unique_ptr<UIWidget> LoadPlacedWidget(XMLElement* elem);

// Loads the widgets in <Items> into a container.
template <class Container>
static std::unique_ptr<Container> WithChildren(std::unique_ptr<Container> container, XMLElement* elem,
    const char* onlyTag = nullptr) {
    XMLElement* items = elem->FirstChildElement("Items");
    if (items) {
        for (XMLElement* child = items->FirstChildElement(onlyTag); child; child = child->NextSiblingElement(onlyTag)) {
            auto w = LoadPlacedWidget(child);
            if (w) container->AddChild(std::move(w));
        }
    }
    return container;
}

// Recursive widget loading from XML.
// <LittleEndian> defaults to true for every widget: Cortex-M targets are little-endian.
// tinyxml2 limits the nesting to TINYXML2_MAX_ELEMENT_DEPTH, which bounds the recursion.
static std::unique_ptr<UIWidget> LoadWidgetFromXML(XMLElement* elem) {
    if (!elem) return nullptr;
    std::string name = elem->Name();

    if (name == "Text") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : elem->GetText();
        return std::make_unique<TextWidget>(txt ? txt : "");
    }
    else if (name == "TextColored") {
        unsigned int rgba = 0xFFFFFFFF; // opaque white when <Color> is missing or invalid
        ParseTextColor(XmlChildText(elem, "Color", nullptr), &rgba);
        return std::make_unique<TextColoredWidget>(ReadText(elem, "Text"), static_cast<int>(rgba));
    }
    else if (name == "Button") {
        const int mode = ReadChoice(elem, "Mode", BUTTON_MODE_SETBIT,
            { BUTTON_MODE_SETBIT, BUTTON_MODE_TOGGLE, BUTTON_MODE_WRITE, BUTTON_MODE_MOMENTARY });
        return std::make_unique<ButtonWidget>(ReadText(elem, "Text"), ReadSize(elem, "Width", 80.0f),
            ReadSize(elem, "Height", 40.0f), ReadAddress(elem, "Address"), (ButtonMode)mode,
            (uint16_t)ReadInt(elem, "Value", 1), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "Checkbox") {
        return std::make_unique<CheckboxWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadBool(elem, "AddrBool", false));
    }
    else if (name == "Group") {
        return WithChildren(std::make_unique<GroupWidget>(), elem);
    }
    else if (name == "TreeNode") {
        return WithChildren(std::make_unique<TreeNodeWidget>(ReadText(elem, "Text")), elem);
    }
    else if (name == "SameLine") {
        return std::make_unique<SameLineWidget>(ReadFloat(elem, "OffsetX", 0.0f, 0.0f, kMaxExtent),
            ReadFloat(elem, "Spacing", -1.0f, -1.0f, kMaxExtent));
    }
    else if (name == "Spacing") {
        return std::make_unique<SpacingWidget>();
    }
    else if (name == "Separator") {
        return std::make_unique<SeparatorWidget>();
    }
    else if (name == "Dummy") {
        return std::make_unique<DummyWidget>(ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent),
            ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent));
    }
    else if (name == "Grid") {
        auto grid = std::make_unique<GridWidget>(ReadInt(elem, "Row", 0, 0, 256), ReadInt(elem, "Column", 0, 0, 256),
            ReadAddress(elem, "Address"), ReadInt(elem, "AddressInc", 1));
        return WithChildren(std::move(grid), elem);
    }
    else if (name == "PlotLines") {
        const char* txt = ReadText(elem, "Text");
        const float minV = ReadFloat(elem, "Min", 0.0f);
        const float maxV = ReadFloat(elem, "Max", 0.0f);   // Min >= Max: the Y axis fits the data
        const float w = ReadSize(elem, "Width", -1.0f);
        const float h = ReadSize(elem, "Height", -1.0f);
        const float f = ReadFloat(elem, "Freq", 1.0f, 0.1f, 1000.0f);
        const int sz = ReadInt(elem, "Size", 100, 1, BUFF_SIZE);
        const int sm = ReadInt(elem, "Smooth", 0, 0, BUFF_SIZE);
        const int maxSize = ReadInt(elem, "MaxSize", 2000, 1, 200000);
        const bool autosize = ReadBool(elem, "AutoSize", false);

        // One line from a <Line> element, or from the widget itself in the old format
        auto readLine = [](XMLElement* e, const char* defaultName, int defaultType) {
            PlotLineConfig line;
            line.name = ReadText(e, "Name", defaultName);
            line.address = ReadAddress(e, "Address");
            line.dataType = ReadDataType(e, "DataType", defaultType);
            line.littleEndian = ReadBool(e, "LittleEndian", true);
            line.scale = ReadFloat(e, "Scale", 1.0f);
            line.bufferId = ReadInt(e, "BufferId", 0, 0, MAX_BUFF - 1);
            line.color = ReadColor(e, "LineColor", IM_COL32_WHITE);
            line.weight = ReadFloat(e, "LineWeight", 1.0f, 0.0f, (float)IM_DRAWLIST_TEX_LINES_WIDTH_MAX);
            return line;
        };

        std::vector<PlotLineConfig> lines;
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* lineElem = items->FirstChildElement("Line"); lineElem; lineElem = lineElem->NextSiblingElement("Line"))
                lines.push_back(readLine(lineElem, "", FLOAT));
        }
        // No lines: one line from the element's own parameters (old format)
        if (lines.empty()) {
            const int defaultType = ReadBool(elem, "Float", true) ? FLOAT : U32_DEC;
            lines.push_back(readLine(elem, txt, defaultType));
        }
        auto plot = std::make_unique<PlotLinesWidget>(txt, minV, maxV, w, h, f, sz, sm, autosize, maxSize, lines);
        const std::string identity = PreferenceElementIdentity(elem);
        plot->SetPreferenceKey(identity.empty() ? "" : prefs::ScopedKey(g_PreferenceDocument, "PlotLines", identity));
        return plot;
    }
    else if (name == "TextFormatted") {
        unsigned int hexColor = 0;
        const bool useColor = ParseTextColor(XmlChildText(elem, "Color", nullptr), &hexColor);
        return std::make_unique<TextFormattedWidget>(ReadText(elem, "Format"), ReadAddress(elem, "Address"), hexColor, useColor);
    }
    else if (name == "TextFormattedF") {
        unsigned int hexColor = 0;
        const bool useColor = ParseTextColor(XmlChildText(elem, "Color", nullptr), &hexColor);
        return std::make_unique<TextFormattedFWidget>(ReadText(elem, "Format"), ReadAddress(elem, "Address"),
            ReadBool(elem, "LittleEndian", true), hexColor, useColor);
    }
    else if (name == "AlignTextToFramePadding") {
        return std::make_unique<AlignTextWidget>();
    }
    else if (name == "NewLine") {
        return std::make_unique<NewLineWidget>();
    }
    else if (name == "LabelText") {
        return std::make_unique<LabelTextWidget>(ReadText(elem, "Label"), ReadText(elem, "Format", "%.3f"),
            ReadAddress(elem, "Address"), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "RadioButton") {
        auto group = std::make_unique<RadioButtonGroup>(ReadAddress(elem, "Address"), ReadBool(elem, "InSameLine", false));
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* radio = items->FirstChildElement("Radio"); radio; radio = radio->NextSiblingElement("Radio"))
                group->AddItem(ReadText(radio, "Label"), ReadInt(radio, "Value", 0));
        }
        return group;
    }
    else if (name == "Tooltip") {
        return std::make_unique<TooltipWidget>(ReadText(elem, "Text"));
    }
    else if (name == "PlotTooltip") {
        return std::make_unique<PlotTooltipWidget>(ReadText(elem, "Text"), ReadText(elem, "Format"),
            ReadFloat(elem, "Min", 0.0f), ReadFloat(elem, "Max", 0.0f), ReadInt(elem, "Size", 100, 1, BUFF_SIZE),
            ReadInt(elem, "BufferId", 0, 0, MAX_BUFF - 1), ReadAddress(elem, "Address"), ReadBool(elem, "LittleEndian", true),
            ReadDataType(elem, "DataType", FLOAT), ReadBool(elem, "AutoSize", false));
    }
    else if (name == "ComboBox") {
        auto combo = std::make_unique<ComboBoxWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"));
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* item = items->FirstChildElement("Combo"); item; item = item->NextSiblingElement("Combo"))
                combo->AddItem(ReadText(item, "Label"), ReadInt(item, "Value", 0));
        }
        return combo;
    }
    else if (name == "ImputInt") {
        // Min and Max limit the value only when Max > Min (the defaults leave it free).
        return std::make_unique<InputIntWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadInt(elem, "Min", 0), ReadInt(elem, "Max", 0), ReadInt(elem, "Step", 1, 0, 65535),
            ReadInt(elem, "Fast", 100, 0, 65535));
    }
    else if (name == "ImputFloat") {
        return std::make_unique<InputFloatWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadFloat(elem, "Min", 0.0f), ReadFloat(elem, "Max", 0.0f), ReadFloat(elem, "Step", 1.0f),
            ReadFloat(elem, "Fast", 100.0f), ReadText(elem, "Format", "%.3f"), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "FixedWidth") {
        return WithChildren(std::make_unique<FixedWidthWidget>(ReadSize(elem, "Width", 0.0f)), elem);
    }
    else if (name == "SetCursorPos") {
        return std::make_unique<SetCursorPosWidget>(ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent),
            ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent));
    }
    else if (name == "Indent") {
        return std::make_unique<IndentWidget>(ReadSize(elem, "Width", 0.0f));
    }
    else if (name == "Histogram" || name == "DataLog") {
        const bool histogram = (name == "Histogram");
        const char* txt = ReadText(elem, "Text");
        const char* fmt = ReadText(elem, "Format");
        const float minV = ReadFloat(elem, "Min", 0.0f);
        const float maxV = ReadFloat(elem, "Max", 0.0f);
        const float w = ReadSize(elem, "Width", 0.0f);
        const float h = ReadSize(elem, "Height", 0.0f);
        const float xs = ReadFloat(elem, "ScaleX", 1.0f);
        const int sz = ReadInt(elem, "Size", 100, 1, BUFF_SIZE);
        const int sm = ReadInt(elem, "Smooth", 0, 0, BUFF_SIZE);
        const uint32_t done = ReadAddress(elem, "DoneAddr");
        const bool le = ReadBool(elem, "LittleEndian", true);
        const bool autoSz = ReadBool(elem, "AutoSize", false);
        const float barWidth = ReadFloat(elem, "BarWidth", 0.8f, 0.0f, 1000.0f);

        // Nested lines
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            std::vector<HistogramWidget::LineConfig> histLines;
            std::vector<DataLogWidget::LineConfig> logLines;
            for (XMLElement* lineElem = items->FirstChildElement("Line"); lineElem; lineElem = lineElem->NextSiblingElement("Line")) {
                const char* lineName = ReadText(lineElem, "Name");
                const uint32_t lineAddr = ReadAddress(lineElem, "Address");
                const float yScale = ReadFloat(lineElem, "ScaleY", 1.0f);
                const ImU32 color = ReadColor(lineElem, "Color", IM_COL32_WHITE);
                const int dtype = ReadDataType(lineElem, "DataType", FLOAT);
                if (histogram) {
                    HistogramWidget::LineConfig line;
                    line.name = lineName; line.baseAddr = lineAddr; line.yScale = yScale; line.color = color; line.dataType = dtype;
                    histLines.push_back(line);
                }
                else {
                    DataLogWidget::LineConfig line;
                    line.name = lineName; line.baseAddr = lineAddr; line.yScale = yScale; line.color = color; line.dataType = dtype;
                    logLines.push_back(line);
                }
            }
            if (histogram && !histLines.empty())
                return std::make_unique<HistogramWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm, done, le, autoSz, barWidth, histLines);
            if (!histogram && !logLines.empty())
                return std::make_unique<DataLogWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm, done, le, logLines, autoSz);
        }

        // Old format: a single line
        const uint32_t addr = ReadAddress(elem, "Address");
        const float ys = ReadFloat(elem, "ScaleY", 1.0f);
        const int dtype = ReadDataType(elem, "DataType", FLOAT);
        if (histogram) {
            return std::make_unique<HistogramWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm, done, le, autoSz, barWidth,
                addr, ys, ReadColor(elem, "Color", IM_COL32_WHITE), dtype);
        }
        return std::make_unique<DataLogWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm, addr, done, le, ys,
            ReadColor(elem, "LineColor", IM_COL32_WHITE), dtype, autoSz);
    }
    else if (name == "ProgressBar") {
        return std::make_unique<ProgressBarWidget>(ReadSize(elem, "Width", 0.0f), ReadSize(elem, "Height", 0.0f),
            ReadAddress(elem, "Address"), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "SliderFloat") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 1.0f);
        OrderRange(minV, maxV);
        return std::make_unique<SliderFloatWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadText(elem, "Format", "%.3f"), ReadFloat(elem, "Step", 0.0f, 0.0f), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "SliderInt") {
        // The value is 16-bit unsigned.
        int minV = ReadInt(elem, "Min", 0, 0, 65535), maxV = ReadInt(elem, "Max", 100, 0, 65535);
        OrderRange(minV, maxV);
        return std::make_unique<SliderIntWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadText(elem, "Format", "%d"));
    }
    else if (name == "TabBar") {
        return WithChildren(std::make_unique<TabBarWidget>(ReadText(elem, "Text")), elem, "Tab");
    }
    else if (name == "Tab") {
        return WithChildren(std::make_unique<TabItemWidget>(ReadText(elem, "Text")), elem);
    }
    else if (name == "Column") {
        return WithChildren(std::make_unique<ColumnWidget>(ReadInt(elem, "Count", 1, 1, 64)), elem);
    }
    else if (name == "SelectableText") {
        return std::make_unique<SelectableTextWidget>(ReadText(elem, "Format"), ReadAddress(elem, "Address"),
            ReadBool(elem, "AddrBool", false));
    }
    else if (name == "ChildWindow") {
        auto child = std::make_unique<ChildWindowWidget>(ReadText(elem, "Text"), ReadSize(elem, "Width", 0.0f),
            ReadSize(elem, "Height", 0.0f));
        return WithChildren(std::move(child), elem);
    }
    else if (name == "Header") {
        return WithChildren(std::make_unique<HeaderWidget>(ReadText(elem, "Text")), elem);
    }
    else if (name == "Popup") {
        return WithChildren(std::make_unique<PopupWidget>(), elem);
    }
    else if (name == "HintBox") {
        return WithChildren(std::make_unique<HintBoxWidget>(), elem);
    }
    else if (name == "HelpMarker") {
        return std::make_unique<HelpMarkerWidget>(ReadText(elem, "Text"));
    }
    else if (name == "Watch") {
        return std::make_unique<WatchWidget>();
    }
    else if (name == "SevenSegment") {
        return std::make_unique<SevenSegmentWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent), ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent),
            ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "MechanicalSwitch2") {
        return std::make_unique<MechanicalSwitch2Widget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent), ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent),
            ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "Gauge") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 200.0f);
        OrderRange(minV, maxV);
        return std::make_unique<GaugeWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadPositive(elem, "Radius", 100.0f, kMaxRadius), ReadText(elem, "Unit"), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "GlossyDisplay") {
        // <HexMode>, or the older <Hex>
        const bool hexMode = elem->FirstChildElement("HexMode") ? ReadBool(elem, "HexMode", false) : ReadBool(elem, "Hex", false);
        return std::make_unique<GlossyDisplayWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent), ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent),
            ReadBool(elem, "LittleEndian", true), ReadDataType(elem, "DataType", U16_DEC), hexMode,
            ReadInt(elem, "Digits", 0, 0, 63), ReadInt(elem, "DecimalPlaces", 2, 0, 63));
    }
    else if (name == "LCD_HD44780") {
        return std::make_unique<LCD_HD44780_Widget>(ReadText(elem, "Text"),
            ReadAddress(elem, "Line1Address"), ReadText(elem, "Line1Text"),
            ReadAddress(elem, "Line2Address"), ReadText(elem, "Line2Text"),
            ReadPositive(elem, "PixelSize", 2.5f, 20.0f), ReadBool(elem, "LittleEndian", true), ReadBool(elem, "UseFixed", false));
    }
    else if (name == "VerticalGauge") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 100.0f);
        OrderRange(minV, maxV);
        return std::make_unique<VerticalGaugeWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadPositive(elem, "Width", 120.0f, kMaxExtent), ReadPositive(elem, "Height", 300.0f, kMaxExtent),
            ReadBool(elem, "LittleEndian", true), ReadDataType(elem, "DataType", FLOAT));
    }
    else if (name == "NeumorphicKnob" || name == "StyledKnob") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 1.0f);
        OrderRange(minV, maxV);
        const char* lbl = ReadText(elem, "Text");
        const uint32_t addr = ReadAddress(elem, "Address");
        const float rad = ReadPositive(elem, "Radius", 50.0f, kMaxRadius);
        const bool le = ReadBool(elem, "LittleEndian", true);
        const int dataType = ReadDataType(elem, "DataType", S16_DEC);
        if (name == "StyledKnob")
            return std::make_unique<StyledKnobWidget>(lbl, addr, minV, maxV, rad, le, dataType);
        return std::make_unique<NeumorphicKnobWidget>(lbl, addr, minV, maxV, rad, le, dataType);
    }
    else if (name == "CustomVerticalSlider") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 1.0f);
        OrderRange(minV, maxV);
        return std::make_unique<CustomVerticalSliderWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadPositive(elem, "Width", 20.0f, kMaxExtent), ReadPositive(elem, "Height", 100.0f, kMaxExtent),
            ReadBool(elem, "LittleEndian", true), ReadDataType(elem, "DataType", FLOAT));
    }
    else if (name == "BitControl8Bit") {
        return std::make_unique<BitControl8BitWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadBool(elem, "LittleEndian", true), ReadPositive(elem, "Scale", 0.55f, 10.0f));
    }
    else if (name == "BitControl") {
        const int mode = ReadChoice(elem, "Mode", Bits8, { Bits8, Bits16, Bits32 });
        return std::make_unique<BitControlWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            static_cast<BitMode>(mode), ReadBool(elem, "LittleEndian", true), ReadBool(elem, "Switchable", true));
    }
    else if (name == "RotarySwitch") {
        int minPos = ReadInt(elem, "Min", 0, 0, 65535), maxPos = ReadInt(elem, "Max", 3, 0, 65535);
        OrderRange(minPos, maxPos);
        return std::make_unique<RotarySwitchWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minPos, maxPos,
            ReadPositive(elem, "Radius", 40.0f, kMaxRadius), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "LedIndicator") {
        return std::make_unique<LedIndicatorWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadPositive(elem, "Radius", 10.0f, kMaxRadius), ReadColor(elem, "ColorOn", IM_COL32(255, 0, 0, 255)),
            ReadColor(elem, "ColorOff", IM_COL32(50, 0, 0, 255)), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "Image") {
        return std::make_unique<ImageWidget>(ReadText(elem, "Text"), ReadText(elem, "File"),
            ReadPositive(elem, "Scale", 1.0f, 100.0f),
            ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent), ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent),
            ReadPoint(elem, "UV0", ImVec2(0, 0)), ReadPoint(elem, "UV1", ImVec2(1, 1)),
            ReadColor(elem, "Tint", IM_COL32(255, 255, 255, 255)), ReadColor(elem, "Border", 0));
    }
    else if (name == "Gif") {
        return std::make_unique<GifWidget>(ReadText(elem, "Text"), ReadText(elem, "File"),
            ReadFloat(elem, "Width", 0.0f, 0.0f, kMaxExtent), ReadFloat(elem, "Height", 0.0f, 0.0f, kMaxExtent),
            ReadPositive(elem, "Scale", 1.0f, 100.0f), ReadBool(elem, "Loop", true), ReadPositive(elem, "Speed", 1.0f, 100.0f));
    }
    else if (name == "RPMIndicator") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 8000.0f);
        OrderRange(minV, maxV);

        std::vector<RPMZone> zones;
        XMLElement* zonesElem = elem->FirstChildElement("Zones");
        if (zonesElem) {
            for (XMLElement* zoneElem = zonesElem->FirstChildElement("Zone"); zoneElem; zoneElem = zoneElem->NextSiblingElement("Zone")) {
                float start = 0, end = 0;
                unsigned int rgba = 0;
                // Zones are sorted below: non-finite bounds would break the ordering.
                if (zoneElem->QueryFloatAttribute("start", &start) != XML_SUCCESS ||
                    zoneElem->QueryFloatAttribute("end", &end) != XML_SUCCESS ||
                    !std::isfinite(start) || !std::isfinite(end) ||
                    !ParseTextColor(zoneElem->Attribute("color"), &rgba))
                    continue;
                zones.push_back({ start, end, IM_COL32((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, 200) });
            }
        }
        if (zones.empty()) {
            zones.push_back({ 0, 4000, IM_COL32(100,200,80,200) });
            zones.push_back({ 4000, 6000, IM_COL32(240,240,80,200) });
            zones.push_back({ 6000, 8000, IM_COL32(230,70,70,200) });
        }
        std::sort(zones.begin(), zones.end());

        return std::make_unique<RPMIndicatorWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadPositive(elem, "Radius", 100.0f, kMaxRadius), ReadText(elem, "Unit", "rpm"), zones, ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "Keypad") {
        int minV = ReadInt(elem, "Min", 0), maxV = ReadInt(elem, "Max", 9999);
        OrderRange(minV, maxV);
        return std::make_unique<KeypadWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadInt(elem, "Digits", 4, 1, 9), ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "Equalizer") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 100.0f);
        OrderRange(minV, maxV);
        const EqDataType dtype = (strcmp(ReadText(elem, "DataType", "float"), "int16") == 0) ? EQ_INT16 : EQ_FLOAT;
        // Byte distance between neighbouring bands: one float or one int16 by default.
        const int step = ReadInt(elem, "AddressStep", dtype == EQ_INT16 ? 2 : 4, 0, 4096);
        return std::make_unique<EqualizerWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadInt(elem, "Bands", 10, 1, 64), minV, maxV, ReadPositive(elem, "BandWidth", 20.0f, kMaxExtent),
            ReadPositive(elem, "Height", 120.0f, kMaxExtent), dtype, ReadBool(elem, "LittleEndian", true), step);
    }
    else if (name == "Tank") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 100.0f);
        OrderRange(minV, maxV);
        return std::make_unique<TankWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV,
            ReadPositive(elem, "Width", 150.0f, kMaxExtent), ReadPositive(elem, "Height", 250.0f, kMaxExtent),
            ReadText(elem, "Unit"), ReadColor(elem, "LiquidColor", IM_COL32(64, 164, 223, 255)),
            ReadColor(elem, "MetalColor", IM_COL32(160, 160, 170, 255)), ReadBool(elem, "LittleEndian", true),
            ReadChoice(elem, "DataType", TANK_FLOAT, { TANK_FLOAT, TANK_U16, TANK_S16, TANK_U32, TANK_S32 }));
    }
    else if (name == "VUMeter") {
        float minV = ReadFloat(elem, "Min", 0.0f), maxV = ReadFloat(elem, "Max", 100.0f);
        OrderRange(minV, maxV);
        const VUOrientation orient = (strcmp(ReadText(elem, "Orientation", "horizontal"), "vertical") == 0) ? VU_VERTICAL : VU_HORIZONTAL;
        return std::make_unique<VUMeterWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), minV, maxV, orient,
            ReadPositive(elem, "Width", 200.0f, kMaxExtent), ReadPositive(elem, "Height", 30.0f, kMaxExtent),
            ReadText(elem, "Unit"), ReadBool(elem, "LittleEndian", true), ReadBool(elem, "ShowPeak", false));
    }
    else if (name == "SegmentButton") {
        std::vector<SegmentItem> segments;
        XMLElement* itemsElem = elem->FirstChildElement("Items");
        if (itemsElem) {
            for (XMLElement* segElem = itemsElem->FirstChildElement("Segment"); segElem; segElem = segElem->NextSiblingElement("Segment")) {
                SegmentItem item;
                item.text = ReadText(segElem, "Text");
                item.value = ReadInt(segElem, "Value", 0);
                segments.push_back(item);
            }
        }
        if (segments.empty()) {
            segments.push_back({ "Low", 0 });
            segments.push_back({ "Med", 1 });
            segments.push_back({ "High", 2 });
        }
        return std::make_unique<SegmentButtonWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"), segments,
            ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "GlossyButton") {
        const int mode = ReadChoice(elem, "Mode", GLOSSY_BUTTON_TOGGLE,
            { GLOSSY_BUTTON_SETBIT, GLOSSY_BUTTON_TOGGLE, GLOSSY_BUTTON_WRITE, GLOSSY_BUTTON_MOMENTARY });
        return std::make_unique<GlossyButtonWidget>(ReadText(elem, "Text"), ReadAddress(elem, "Address"),
            ReadBool(elem, "LittleEndian", true), (GlossyButtonMode)mode, ReadInt(elem, "Value", 1),
            ReadPositive(elem, "Radius", 20.0f, kMaxRadius));
    }
    else if (name == "MemoryView") {
        return std::make_unique<MemoryViewWidget>(ReadText(elem, "Text"), ReadAddress(elem, "BaseAddress"),
            (uint16_t)ReadInt(elem, "NumBytes", 256, 1, 65535), ReadInt(elem, "BytesPerLine", 16, 1, 256),
            ReadBool(elem, "LittleEndian", true));
    }
    else if (name == "MemoryEditor") {
        return std::make_unique<MemoryEditorWidget>(ReadText(elem, "Text"), ReadAddress(elem, "BaseAddress"),
            (size_t)ReadInt(elem, "Size", 256, 1, IOMAP_SIZE), ReadInt(elem, "Cols", 16, 1, 64),
            ReadBool(elem, "ReadOnly", false), ReadBool(elem, "ShowAscii", true), ReadBool(elem, "ShowHexII", false),
            ReadBool(elem, "ShowOptions", true), ReadBool(elem, "ShowPreview", true),
            ReadBool(elem, "GreyZero", true), ReadBool(elem, "UpperHex", true), ReadBool(elem, "LittleEndian", true),
            ReadBool(elem, "UseExternal", false), ReadBool(elem, "UseCache", true));
    }
    return nullptr;
}

// Properties every widget shares, read around the widget itself: the position in the window
// and the width of input fields. Which widget takes which comes from the editor's table.
static std::unique_ptr<UIWidget> PlaceWidget(std::unique_ptr<UIWidget> widget, XMLElement* elem) {
    if (!widget) return nullptr;
    const WidgetTemplateFactory::NodeDef* def = WidgetTemplateFactory::FindWidget(elem->Name());
    WidgetLayout layout;
    if (def && def->placeable) {
        layout.placeable = true;
        layout.absolute = elem->FirstChildElement("PosX") || elem->FirstChildElement("PosY");
        layout.pos = ImVec2(ReadFloat(elem, "PosX", 0.0f, 0.0f, kMaxExtent), ReadFloat(elem, "PosY", 0.0f, 0.0f, kMaxExtent));
    }
    if (def && def->itemWidth) layout.itemWidth = ReadSize(elem, "ItemWidth", 0.0f);
    return std::make_unique<PlacedWidget>(std::move(widget), g_KeepSources ? elem : nullptr, layout);
}

static std::unique_ptr<UIWidget> LoadPlacedWidget(XMLElement* elem) {
    return PlaceWidget(LoadWidgetFromXML(elem), elem);
}

// Builds g_Windows from a document. keepSources: the document stays alive (ctx->CurrentXMLDoc)
// and the design mode may refer to its elements.
static void BuildWindows(MainContext* ctx, XMLDocument& doc, bool keepSources) {
    g_Windows.clear();
    g_WindowPreferenceKeys.clear();
    g_KeepSources = keepSources;
    int winCount = 0;
    for (XMLElement* win = doc.FirstChildElement("Window"); win; win = win->NextSiblingElement("Window")) {
        winCount++;
        WindowData wd;
        const char* name = XmlChildText(win, "Text", "Unnamed");
        wd.name = name;
        const std::string identity = PreferenceElementIdentity(win);
        const std::string visibilityKey = identity.empty() ? "" : prefs::ScopedKey(g_PreferenceDocument, "Windows", identity);
        g_WindowPreferenceKeys.push_back(visibilityKey);
        wd.show = visibilityKey.empty() ? true : prefs::ReadBool(visibilityKey, true);
        wd.source = keepSources ? win : nullptr;

        // Geometry, applied once; without it the window keeps the position ImGui remembers
        wd.hasPos = win->FirstChildElement("PosX") || win->FirstChildElement("PosY");
        wd.pos = ImVec2(ReadFloat(win, "PosX", 0.0f, -kMaxExtent, kMaxExtent), ReadFloat(win, "PosY", 0.0f, -kMaxExtent, kMaxExtent));
        wd.size = ImVec2(ReadFloat(win, "Width", 0.0f, 0.0f, kMaxExtent), ReadFloat(win, "Height", 0.0f, 0.0f, kMaxExtent));
        wd.hasSize = wd.size.x > 0.0f || wd.size.y > 0.0f;   // 0 on one axis fits the content
        wd.applyGeometry = wd.hasPos || wd.hasSize;

        // IOMap block, if present
        XMLElement* iomap = win->FirstChildElement("IOMap");
        if (iomap && ctx->StartComunication && ctx->Sync == SDL_FALSE) {
            XMLElement* items = iomap->FirstChildElement("Items");
            if (items) {
                SDL_LockMutex(ctx->Datalock);
                ctx->Sync = SDL_TRUE; // mark the initial read as done
                for (XMLElement* elem = items->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
                    uint32_t addr = 0;
                    int size = 0;

                    // Address
                    XMLElement* addrElem = TryFindElement(elem, "Address");
                    if (addrElem) {
                        addr = AddressFromHex(addrElem);
                    }

                    // Size
                    XMLElement* sizeElem = TryFindElement(elem, "Size");
                    if (sizeElem) {
                        sizeElem->QueryIntText(&size);
                    }

                    // Request a read if the address and size are valid
                    if (addr != 0 && size > 0) {
                        // DataReadWriteArray with data = nullptr means read
                        DataReadWriteArray(ctx, nullptr, nullptr, &addr, (uint16_t)size);
                    }
                }
                SDL_CondSignal(ctx->Datacond); // wake the communication thread
                SDL_UnlockMutex(ctx->Datalock);
            }
        }

        // Visual elements
        XMLElement* items = win->FirstChildElement("Items");
        if (items) {
            auto rootGroup = std::make_unique<GroupWidget>();
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto widget = LoadPlacedWidget(child);
                if (widget) {
                    rootGroup->AddChild(std::move(widget));
                }
                else if (!keepSources) {
                    printf("Warning: could not create widget for element <%s> in window '%s'\n",
                        child->Name(), wd.name.c_str());
                }
            }
            wd.root = std::move(rootGroup);
        }
        else if (!keepSources) {
            printf("Window '%s' has no <Items>: empty window.\n", wd.name.c_str());
        }
        g_Windows.push_back(std::move(wd));
    }
    g_KeepSources = false;
    if (!keepSources) printf("Loaded %d windows.\n", winCount);
    ctx->file_loaded = true;
}

// Loads all windows from the XML file
void LoadWindowsFromXML(MainContext* ctx, const char* filename) {
    XMLDocument doc;
    if (doc.LoadFile(filename) != XML_SUCCESS) {
        printf("ERROR: Failed to load XML file: %s\n", filename);
        // Do not retry on every frame; opening or saving the file resets the flag.
        ctx->file_loaded = true;
        return;
    }
    printf("XML loaded successfully: %s\n", filename);
    g_PreferenceDocument = filename;
    BuildWindows(ctx, doc, false);
    g_WindowsDoc = nullptr;
}

// Design mode: the windows show the open document, with its unsaved changes.
static void LoadWindowsFromDocument(MainContext* ctx) {
    g_PreferenceDocument = ctx->File_path;
    BuildWindows(ctx, *ctx->CurrentXMLDoc, true);
    g_WindowsDoc = ctx->CurrentXMLDoc;
    g_WindowsRevision = ctx->xml_revision;
}

// Design mode: a window moved or resized with the mouse is written back to its <Window>.
static void TrackWindowGeometry(MainContext* ctx, WindowData& win) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 pos = window->Pos;
    const ImVec2 size = window->SizeFull;   // not the title bar of a collapsed window
    const bool dragging = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (win.seen && dragging && (pos.x != win.seenPos.x || pos.y != win.seenPos.y ||
        size.x != win.seenSize.x || size.y != win.seenSize.y))
        win.geometryChanged = true;
    win.seen = true;
    win.seenPos = pos;
    win.seenSize = size;
    if (win.geometryChanged && !dragging) {
        win.geometryChanged = false;
        DesignMode::CommitWindowGeometry(ctx, win.source, pos, size);
    }
}

// Draws all windows
int Draw_Window(MainContext* ctx) {

    if (!ctx) return -1;
    if (ctx->design_mode && !ctx->CurrentXMLDoc) ctx->design_mode = false;   // nothing to edit
    const bool design = ctx->design_mode;

    if (design) {
        // Every change of the document (here, in the XML editor, undo) rebuilds the windows.
        if (g_WindowsDoc != ctx->CurrentXMLDoc || g_WindowsRevision != ctx->xml_revision)
            LoadWindowsFromDocument(ctx);
    }
    else if (g_Windows.empty()) {
        if (ctx->File_path[0] != '\0' && !ctx->file_loaded) {
            LoadWindowsFromXML(ctx, ctx->File_path);
        }
        // Nothing to draw: the main window shows the hint instead of an implicit "Debug" window.
        if (g_Windows.empty()) return -1;
    }

    DesignMode::BeginFrame(design);
    for (auto& win : g_Windows) {
        if (!win.show) continue;
        if (win.applyGeometry) {
            if (win.hasPos) ImGui::SetNextWindowPos(win.pos, ImGuiCond_Always);
            if (win.hasSize) ImGui::SetNextWindowSize(win.size, ImGuiCond_Always);
            win.applyGeometry = false;
        }
        ImGui::SetNextWindowBgAlpha(0.9f);
        const bool visible = ImGui::Begin(win.name.c_str(), &win.show, ImGuiWindowFlags_None);
        if (design) DesignMode::BeginWindow(win.source);
        if (visible && win.root) {
            win.root->Draw(ctx, nullptr);
        }
        if (design) TrackWindowGeometry(ctx, win);
        ImGui::End();
    }
    SaveDashboardWindowVisibility(); // title-bar close buttons, including collapsed windows
    if (design) DesignMode::EndFrame(ctx);
    return 0;
}

// Main-menu visibility toggles call this immediately after changing win.show.
// Read-before-write avoids staging defaults simply because a dashboard was drawn.
void SaveDashboardWindowVisibility() {
    for (size_t i = 0; i < g_Windows.size() && i < g_WindowPreferenceKeys.size(); ++i) {
        const std::string& key = g_WindowPreferenceKeys[i];
        if (key.empty()) continue; // ambiguous windows deliberately have no persisted identity
        if (prefs::ReadBool(key, true) != g_Windows[i].show)
            prefs::WriteBool(key, g_Windows[i].show);
    }
}

// Removes all windows
extern "C" void ClearAllWindows(void) {
    g_Windows.clear();
    g_WindowPreferenceKeys.clear();
    g_WindowsDoc = nullptr;
}