// xml_window.cpp
#include "widget_common.h"

// Включаем все виджеты
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

#include <Main.h>

using namespace tinyxml2;

// Глобальный буфер для графиков
float VirtualBuffer[MAX_BUFF][BUFF_SIZE] = { 0 };

std::vector<WindowData> g_Windows;

static const char* XmlChildText(XMLElement* elem, const char* name, const char* def = "") {
    if (!elem) return def;
    XMLElement* child = elem->FirstChildElement(name);
    if (!child) return def;
    const char* txt = child->GetText();
    return txt ? txt : def;
}

// Рекурсивная загрузка виджетов из XML
static std::unique_ptr<UIWidget> LoadWidgetFromXML(XMLElement* elem) {
    if (!elem) return nullptr;
    std::string name = elem->Name();

    if (name == "Text") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : elem->GetText();
        return std::make_unique<TextWidget>(txt ? txt : "");
    }
    else if (name == "TextColored") {
        const char* txt = XmlChildText(elem, "Text");
        const char* colorStr = XmlChildText(elem, "Color", nullptr);
        int hex = 0;
        if (colorStr) { int ret = sscanf(colorStr, "#%08x", &hex); }
        return std::make_unique<TextColoredWidget>(txt, hex);
    }
    else if (name == "Button") {
        const char* txt = XmlChildText(elem, "Text");
        float w = 80.0f, h = 40.0f;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height"))
            elem->FirstChildElement("Height")->QueryFloatText(&h);
        int addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int mode = 0;
        if (elem->FirstChildElement("Mode"))
            elem->FirstChildElement("Mode")->QueryIntText(&mode);
        int val = 1;
        if (elem->FirstChildElement("Value"))
            elem->FirstChildElement("Value")->QueryIntText(&val);
        bool le = true;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        return std::make_unique<ButtonWidget>(txt, w, h, addr, (ButtonMode)mode, (uint16_t)val, le);
    }
    else if (name == "Checkbox") {
        const char* txt = XmlChildText(elem, "Text");
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool useBool = false;
        if (elem->FirstChildElement("AddrBool"))
            elem->FirstChildElement("AddrBool")->QueryBoolText(&useBool);
        return std::make_unique<CheckboxWidget>(txt, addr, useBool);
    }
    else if (name == "Group") {
        auto group = std::make_unique<GroupWidget>();
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) group->AddChild(std::move(w));
            }
        }
        return group;
    }
    else if (name == "TreeNode") {
        const char* txt = XmlChildText(elem, "Text");
        auto node = std::make_unique<TreeNodeWidget>(txt);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) node->AddChild(std::move(w));
            }
        }
        return node;
    }
    else if (name == "SameLine") {
        float off = 0, sp = -1;
        if (elem->FirstChildElement("OffsetX"))
            elem->FirstChildElement("OffsetX")->QueryFloatText(&off);
        if (elem->FirstChildElement("Spacing"))
            elem->FirstChildElement("Spacing")->QueryFloatText(&sp);
        return std::make_unique<SameLineWidget>(off, sp);
    }
    else if (name == "Spacing") {
        return std::make_unique<SpacingWidget>();
    }
    else if (name == "Separator") {
        return std::make_unique<SeparatorWidget>();
    }
    else if (name == "Dummy") {
        float w = 0, h = 0;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height"))
            elem->FirstChildElement("Height")->QueryFloatText(&h);
        return std::make_unique<DummyWidget>(w, h);
    }
    else if (name == "Grid") {
        int rows = 0, cols = 0, inc = 1;
        uint32_t addr = 0;
        if (elem->FirstChildElement("Row"))
            elem->FirstChildElement("Row")->QueryIntText(&rows);
        if (elem->FirstChildElement("Column"))
            elem->FirstChildElement("Column")->QueryIntText(&cols);
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("AddressInc"))
            elem->FirstChildElement("AddressInc")->QueryIntText(&inc);
        auto grid = std::make_unique<GridWidget>(rows, cols, addr, inc);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) grid->AddChild(std::move(w));
            }
        }
        return grid;
    }
    else if (name == "PlotLines") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        float minV = 0, maxV = 0, w = -1, h = -1, f = 1;
        int sz = 100, sm = 0, maxSize = 2000;
        bool autosize = false;

        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height")) elem->FirstChildElement("Height")->QueryFloatText(&h);
        if (elem->FirstChildElement("Freq")) elem->FirstChildElement("Freq")->QueryFloatText(&f);
        if (elem->FirstChildElement("Size")) elem->FirstChildElement("Size")->QueryIntText(&sz);
        if (elem->FirstChildElement("Smooth")) elem->FirstChildElement("Smooth")->QueryIntText(&sm);
        if (elem->FirstChildElement("MaxSize")) elem->FirstChildElement("MaxSize")->QueryIntText(&maxSize);
        if (elem->FirstChildElement("AutoSize")) elem->FirstChildElement("AutoSize")->QueryBoolText(&autosize);

        std::vector<PlotLineConfig> lines;

        // Читаем дочерние линии из Items
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* lineElem = items->FirstChildElement("Line"); lineElem; lineElem = lineElem->NextSiblingElement()) {
                PlotLineConfig line;
                line.name = lineElem->FirstChildElement("Name") ? lineElem->FirstChildElement("Name")->GetText() : "";
                if (lineElem->FirstChildElement("Address"))
                    line.address = AddressFromHex(lineElem->FirstChildElement("Address"));
                if (lineElem->FirstChildElement("DataType"))
                    lineElem->FirstChildElement("DataType")->QueryIntText(&line.dataType);
                bool le = false;
                if (lineElem->FirstChildElement("LittleEndian"))
                    lineElem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
                line.littleEndian = le;
                float sc = 1.0f;
                if (lineElem->FirstChildElement("Scale"))
                    lineElem->FirstChildElement("Scale")->QueryFloatText(&sc);
                line.scale = sc;
                int bufId = 0;
                if (lineElem->FirstChildElement("BufferId"))
                    lineElem->FirstChildElement("BufferId")->QueryIntText(&bufId);
                line.bufferId = bufId;

                // Цвет
                ImU32 col = IM_COL32_WHITE;
                if (lineElem->FirstChildElement("LineColor")) {
                    const char* colStr = lineElem->FirstChildElement("LineColor")->GetText();
                    if (colStr && colStr[0] == '#') {
                        unsigned int hex = 0;
                        sscanf(colStr + 1, "%x", &hex);
                        if (strlen(colStr) == 7)
                            col = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                        else if (strlen(colStr) == 9)
                            col = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                    }
                }
                line.color = col;

                float weight = 1.0f;
                if (lineElem->FirstChildElement("LineWeight"))
                    lineElem->FirstChildElement("LineWeight")->QueryFloatText(&weight);
                line.weight = weight;

                lines.push_back(line);
            }
        }

        // Если линий нет, создаём одну из корневых параметров (старый формат)
        if (lines.empty()) {
            // Читаем старые параметры: Address, DataType, Float, LittleEndian, Scale, BufferId, LineColor, LineWeight
            PlotLineConfig line;
            line.name = txt;
            if (elem->FirstChildElement("Address"))
                line.address = AddressFromHex(elem->FirstChildElement("Address"));

            int dtype = U32_DEC;
            if (elem->FirstChildElement("DataType")) {
                elem->FirstChildElement("DataType")->QueryIntText(&dtype);
            }
            else {
                bool flt = true;
                if (elem->FirstChildElement("Float")) elem->FirstChildElement("Float")->QueryBoolText(&flt);
                dtype = flt ? FLOAT : U32_DEC;
            }
            line.dataType = dtype;

            bool le = false;
            if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
            line.littleEndian = le;

            float sc = 1.0f;
            if (elem->FirstChildElement("Scale")) elem->FirstChildElement("Scale")->QueryFloatText(&sc);
            line.scale = sc;

            int bufId = 0;
            if (elem->FirstChildElement("BufferId")) elem->FirstChildElement("BufferId")->QueryIntText(&bufId);
            line.bufferId = bufId;

            // Цвет
            ImU32 col = IM_COL32_WHITE;
            if (elem->FirstChildElement("LineColor")) {
                const char* colStr = elem->FirstChildElement("LineColor")->GetText();
                if (colStr && colStr[0] == '#') {
                    unsigned int hex = 0;
                    sscanf(colStr + 1, "%x", &hex);
                    if (strlen(colStr) == 7)
                        col = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                    else if (strlen(colStr) == 9)
                        col = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
            line.color = col;

            float weight = 1.0f;
            if (elem->FirstChildElement("LineWeight")) elem->FirstChildElement("LineWeight")->QueryFloatText(&weight);
            line.weight = weight;

            lines.push_back(line);
        }

        // Создаём виджет с несколькими линиями
        return std::make_unique<PlotLinesWidget>(
            txt, minV, maxV, w, h, f, sz, sm, autosize, maxSize, lines
        );
    }
    else if (name == "TextFormatted") {
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int hexColor = 0;
        bool useColor = false;
        if (elem->FirstChildElement("Color")) {
            const char* colorStr = elem->FirstChildElement("Color")->GetText();
            if (colorStr) { int ret = sscanf(colorStr, "#%08x", &hexColor); useColor = true; }
        }
        return std::make_unique<TextFormattedWidget>(fmt, addr, hexColor, useColor);
    }
    else if (name == "TextFormattedF") {
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool le = false;
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int hexColor = 0;
        bool useColor = false;
        if (elem->FirstChildElement("Color")) {
            const char* colorStr = elem->FirstChildElement("Color")->GetText();
            if (colorStr) { int ret = sscanf(colorStr, "#%08x", &hexColor); useColor = true; }
        }
        return std::make_unique<TextFormattedFWidget>(fmt, addr, le, hexColor, useColor);
    }
    else if (name == "AlignTextToFramePadding") {
        return std::make_unique<AlignTextWidget>();
    }
    else if (name == "NewLine") {
        return std::make_unique<NewLineWidget>();
    }
    else if (name == "LabelText") {
        const char* lbl = elem->FirstChildElement("Label") ? elem->FirstChildElement("Label")->GetText() : "";
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool le = false;
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<LabelTextWidget>(lbl, fmt, addr, le);
    }
    else if (name == "RadioButton") {
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool sameLine = false;
        if (elem->FirstChildElement("InSameLine")) elem->FirstChildElement("InSameLine")->QueryBoolText(&sameLine);
        auto group = std::make_unique<RadioButtonGroup>(addr, sameLine);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* radio = items->FirstChildElement("Radio"); radio; radio = radio->NextSiblingElement()) {
                const char* lbl = radio->FirstChildElement("Label") ? radio->FirstChildElement("Label")->GetText() : "";
                int val = 0;
                if (radio->FirstChildElement("Value")) radio->FirstChildElement("Value")->QueryIntText(&val);
                group->AddItem(lbl, val);
            }
        }
        return group;
    }
    else if (name == "Tooltip") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        return std::make_unique<TooltipWidget>(txt);
    }
    else if (name == "PlotTooltip") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        float minV = 0, maxV = 0;
        int sz = 100, id = 0;
        bool le = false;
        uint32_t addr = 0;
        int dtype = FLOAT;
        bool autoSz = false;

        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Size")) elem->FirstChildElement("Size")->QueryIntText(&sz);
        if (elem->FirstChildElement("BufferId")) elem->FirstChildElement("BufferId")->QueryIntText(&id);
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        if (elem->FirstChildElement("Address")) addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("DataType")) elem->FirstChildElement("DataType")->QueryIntText(&dtype);
        if (elem->FirstChildElement("AutoSize")) elem->FirstChildElement("AutoSize")->QueryBoolText(&autoSz);

        return std::make_unique<PlotTooltipWidget>(txt, fmt, minV, maxV, sz, id, addr, le, dtype, autoSz);
    }
    else if (name == "ComboBox") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        auto combo = std::make_unique<ComboBoxWidget>(lbl, addr);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* item = items->FirstChildElement("Combo"); item; item = item->NextSiblingElement()) {
                const char* label = item->FirstChildElement("Label") ? item->FirstChildElement("Label")->GetText() : "";
                int val = 0;
                if (item->FirstChildElement("Value")) item->FirstChildElement("Value")->QueryIntText(&val);
                combo->AddItem(label, val);
            }
        }
        return combo;
    }
    else if (name == "ImputInt") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int minV = 0, maxV = 0, step = 1, fast = 100;
        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryIntText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryIntText(&maxV);
        if (elem->FirstChildElement("Step")) elem->FirstChildElement("Step")->QueryIntText(&step);
        if (elem->FirstChildElement("Fast")) elem->FirstChildElement("Fast")->QueryIntText(&fast);
        return std::make_unique<InputIntWidget>(lbl, addr, minV, maxV, step, fast);
    }
    else if (name == "ImputFloat") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0, maxV = 0, step = 1, fast = 100;
        const char* fmt = "%.3f";
        bool le = false;
        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Step")) elem->FirstChildElement("Step")->QueryFloatText(&step);
        if (elem->FirstChildElement("Fast")) elem->FirstChildElement("Fast")->QueryFloatText(&fast);
        if (elem->FirstChildElement("Format")) fmt = elem->FirstChildElement("Format")->GetText();
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<InputFloatWidget>(lbl, addr, minV, maxV, step, fast, fmt, le);
    }
    else if (name == "FixedWidth") {
        float w = 0;
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        auto fw = std::make_unique<FixedWidthWidget>(w);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto widget = LoadWidgetFromXML(child);
                if (widget) fw->AddChild(std::move(widget));
            }
        }
        return fw;
    }
    else if (name == "SetCursorPos") {
        float x = 0, y = 0;
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&x);
        if (elem->FirstChildElement("Height")) elem->FirstChildElement("Height")->QueryFloatText(&y);
        return std::make_unique<SetCursorPosWidget>(x, y);
    }
    else if (name == "Indent") {
        float w = 0;
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        return std::make_unique<IndentWidget>(w);
    }
    else if (name == "Histogram") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        float minV = 0, maxV = 0, w = 0, h = 0, xs = 1.0f;
        int sz = 100, sm = 0;
        uint32_t done = 0;
        bool le = false;
        bool autoSz = false;
        float barWidth = 0.8f; // по умолчанию

        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height")) elem->FirstChildElement("Height")->QueryFloatText(&h);
        if (elem->FirstChildElement("ScaleX")) elem->FirstChildElement("ScaleX")->QueryFloatText(&xs);
        if (elem->FirstChildElement("Size")) elem->FirstChildElement("Size")->QueryIntText(&sz);
        if (elem->FirstChildElement("Smooth")) elem->FirstChildElement("Smooth")->QueryIntText(&sm);
        if (elem->FirstChildElement("DoneAddr")) done = AddressFromHex(elem->FirstChildElement("DoneAddr"));
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        if (elem->FirstChildElement("AutoSize")) elem->FirstChildElement("AutoSize")->QueryBoolText(&autoSz);
        if (elem->FirstChildElement("BarWidth")) elem->FirstChildElement("BarWidth")->QueryFloatText(&barWidth);

        // Проверяем наличие вложенных линий
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            std::vector<HistogramWidget::LineConfig> lineList;
            for (XMLElement* lineElem = items->FirstChildElement("Line"); lineElem; lineElem = lineElem->NextSiblingElement()) {
                HistogramWidget::LineConfig line;
                line.name = lineElem->FirstChildElement("Name") ? lineElem->FirstChildElement("Name")->GetText() : "";
                if (lineElem->FirstChildElement("Address"))
                    line.baseAddr = AddressFromHex(lineElem->FirstChildElement("Address"));
                line.yScale = 1.0f;
                if (lineElem->FirstChildElement("ScaleY"))
                    lineElem->FirstChildElement("ScaleY")->QueryFloatText(&line.yScale);
                line.color = IM_COL32_WHITE;
                line.dataType = FLOAT;
                if (lineElem->FirstChildElement("DataType"))
                    lineElem->FirstChildElement("DataType")->QueryIntText(&line.dataType);

                if (lineElem->FirstChildElement("Color")) {
                    const char* colStr = lineElem->FirstChildElement("Color")->GetText();
                    if (colStr && colStr[0] == '#') {
                        unsigned int hex = 0;
                        sscanf(colStr + 1, "%x", &hex);
                        if (strlen(colStr) == 7) {
                            line.color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                        }
                        else if (strlen(colStr) == 9) {
                            line.color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                        }
                    }
                }
                lineList.push_back(line);
            }
            if (!lineList.empty()) {
                return std::make_unique<HistogramWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm,
                    done, le, autoSz, barWidth, lineList);
            }
        }

        // Старый формат: одна линия
        uint32_t addr = 0;
        float ys = 1.0f;
        ImU32 color = IM_COL32_WHITE;
        int dtype = FLOAT;

        if (elem->FirstChildElement("Address")) addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("ScaleY")) elem->FirstChildElement("ScaleY")->QueryFloatText(&ys);
        if (elem->FirstChildElement("DataType")) elem->FirstChildElement("DataType")->QueryIntText(&dtype);
        if (elem->FirstChildElement("Color")) {
            const char* colStr = elem->FirstChildElement("Color")->GetText();
            if (colStr && colStr[0] == '#') {
                unsigned int hex = 0;
                sscanf(colStr + 1, "%x", &hex);
                if (strlen(colStr) == 7) {
                    color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                }
                else if (strlen(colStr) == 9) {
                    color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
        }

        return std::make_unique<HistogramWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm,
            done, le, autoSz, barWidth,
            addr, ys, color, dtype);
    }
    else if (name == "DataLog") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        float minV = 0, maxV = 0, w = 0, h = 0, xs = 1;
        int sz = 100, sm = 0;
        uint32_t done = 0;
        bool le = false;
        bool autoSz = false;

        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height")) elem->FirstChildElement("Height")->QueryFloatText(&h);
        if (elem->FirstChildElement("ScaleX")) elem->FirstChildElement("ScaleX")->QueryFloatText(&xs);
        if (elem->FirstChildElement("Size")) elem->FirstChildElement("Size")->QueryIntText(&sz);
        if (elem->FirstChildElement("Smooth")) elem->FirstChildElement("Smooth")->QueryIntText(&sm);
        if (elem->FirstChildElement("DoneAddr")) done = AddressFromHex(elem->FirstChildElement("DoneAddr"));
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        if (elem->FirstChildElement("AutoSize")) elem->FirstChildElement("AutoSize")->QueryBoolText(&autoSz);

        // Проверяем, есть ли вложенные линии
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            std::vector<DataLogWidget::LineConfig> lineList;
            for (XMLElement* lineElem = items->FirstChildElement("Line"); lineElem; lineElem = lineElem->NextSiblingElement()) {
                DataLogWidget::LineConfig line;
                line.name = lineElem->FirstChildElement("Name") ? lineElem->FirstChildElement("Name")->GetText() : "";
                if (lineElem->FirstChildElement("Address"))
                    line.baseAddr = AddressFromHex(lineElem->FirstChildElement("Address"));
                line.yScale = 1.0f;
                if (lineElem->FirstChildElement("ScaleY"))
                    lineElem->FirstChildElement("ScaleY")->QueryFloatText(&line.yScale);
                line.color = IM_COL32_WHITE;
                line.dataType = FLOAT; // по умолчанию
                if (lineElem->FirstChildElement("DataType"))
                    lineElem->FirstChildElement("DataType")->QueryIntText(&line.dataType);

                if (lineElem->FirstChildElement("Color")) {
                    const char* colStr = lineElem->FirstChildElement("Color")->GetText();
                    if (colStr && colStr[0] == '#') {
                        unsigned int hex = 0;
                        sscanf(colStr + 1, "%x", &hex);
                        if (strlen(colStr) == 7) {
                            line.color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                        }
                        else if (strlen(colStr) == 9) {
                            line.color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                        }
                    }
                }
                lineList.push_back(line);
            }
            if (!lineList.empty()) {
                return std::make_unique<DataLogWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm, done, le, lineList, autoSz);
            }
        }

        // Старый формат: одна линия
        uint32_t addr = 0;
        float ys = 1.0f;
        ImU32 color = IM_COL32_WHITE;
        int dtype = FLOAT;

        if (elem->FirstChildElement("Address")) addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("ScaleY")) elem->FirstChildElement("ScaleY")->QueryFloatText(&ys);
        if (elem->FirstChildElement("DataType")) elem->FirstChildElement("DataType")->QueryIntText(&dtype);
        if (elem->FirstChildElement("LineColor")) {
            const char* colStr = elem->FirstChildElement("LineColor")->GetText();
            if (colStr && colStr[0] == '#') {
                unsigned int hex = 0;
                sscanf(colStr + 1, "%x", &hex);
                if (strlen(colStr) == 7) {
                    color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                }
                else if (strlen(colStr) == 9) {
                    color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
        }

        return std::make_unique<DataLogWidget>(txt, fmt, minV, maxV, w, h, xs, sz, sm, addr, done, le, ys, color, dtype, autoSz);
    }
    else if (name == "ProgressBar") {
        float w = 0, h = 0;
        uint32_t addr = 0;
        bool le = false;
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height")) elem->FirstChildElement("Height")->QueryFloatText(&h);
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<ProgressBarWidget>(w, h, addr, le);
    }
    else if (name == "SliderFloat") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        float minV = 0, maxV = 0, step = 0;
        const char* fmt = "%.3f";
        bool le = false;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Step")) elem->FirstChildElement("Step")->QueryFloatText(&step);
        if (elem->FirstChildElement("Format")) fmt = elem->FirstChildElement("Format")->GetText();
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<SliderFloatWidget>(lbl, addr, minV, maxV, fmt, step, le);
    }
    else if (name == "SliderInt") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        int minV = 0, maxV = 0;
        const char* fmt = "%d";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryIntText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryIntText(&maxV);
        if (elem->FirstChildElement("Format")) fmt = elem->FirstChildElement("Format")->GetText();
        return std::make_unique<SliderIntWidget>(lbl, addr, minV, maxV, fmt);
    }
    else if (name == "TabBar") {
        const char* name = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        auto tabBar = std::make_unique<TabBarWidget>(name);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement("Tab"); child; child = child->NextSiblingElement()) {
                auto tabItem = LoadWidgetFromXML(child);
                if (tabItem) tabBar->AddChild(std::move(tabItem));
            }
        }
        return tabBar;
    }
    else if (name == "Tab") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        auto tab = std::make_unique<TabItemWidget>(lbl);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) tab->AddChild(std::move(w));
            }
        }
        return tab;
    }
    else if (name == "Column") {
        int cnt = 0;
        if (elem->FirstChildElement("Count")) elem->FirstChildElement("Count")->QueryIntText(&cnt);
        auto col = std::make_unique<ColumnWidget>(cnt);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) col->AddChild(std::move(w));
            }
        }
        return col;
    }
    else if (name == "SelectableText") {
        const char* fmt = elem->FirstChildElement("Format") ? elem->FirstChildElement("Format")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool useBool = false;
        if (elem->FirstChildElement("AddrBool")) elem->FirstChildElement("AddrBool")->QueryBoolText(&useBool);
        return std::make_unique<SelectableTextWidget>(fmt, addr, useBool);
    }
    else if (name == "ChildWindow") {
        const char* name = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        float w = 0, h = 0;
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        if (elem->FirstChildElement("Height")) elem->FirstChildElement("Height")->QueryFloatText(&h);
        auto child = std::make_unique<ChildWindowWidget>(name, w, h);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* childElem = items->FirstChildElement(); childElem; childElem = childElem->NextSiblingElement()) {
                auto wgt = LoadWidgetFromXML(childElem);
                if (wgt) child->AddChild(std::move(wgt));
            }
        }
        return child;
    }
    else if (name == "Header") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        auto header = std::make_unique<HeaderWidget>(lbl);
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) header->AddChild(std::move(w));
            }
        }
        return header;
    }
    else if (name == "Popup") {
        auto popup = std::make_unique<PopupWidget>();
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) popup->AddChild(std::move(w));
            }
        }
        return popup;
    }
    else if (name == "HintBox") {
        auto hint = std::make_unique<HintBoxWidget>();
        XMLElement* items = elem->FirstChildElement("Items");
        if (items) {
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto w = LoadWidgetFromXML(child);
                if (w) hint->AddChild(std::move(w));
            }
        }
        return hint;
    }
    else if (name == "HelpMarker") {
        const char* txt = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        return std::make_unique<HelpMarkerWidget>(txt);
    }
    else if (name == "Watch") {
        return std::make_unique<WatchWidget>();
    }
    else if (name == "SevenSegment") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float w = 0, h = 0;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<SevenSegmentWidget>(lbl, addr, w, h, le);
    }
    else if (name == "MechanicalSwitch2") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float w = 0, h = 0;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<MechanicalSwitch2Widget>(lbl, addr, w, h, le);
    }
    else if (name == "Gauge") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 200.0f, rad = 100.0f;
        const char* unit = "";
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&rad);
        if (elem->FirstChildElement("Unit"))
            unit = elem->FirstChildElement("Unit")->GetText();
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<GaugeWidget>(lbl, addr, minV, maxV, rad, unit, le);
    }
    else if (name == "GlossyDisplay") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float w = 0, h = 0;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        int dataType = U16_DEC;
        if (elem->FirstChildElement("DataType"))
            elem->FirstChildElement("DataType")->QueryIntText(&dataType);

        bool hexMode = false;
        if (elem->FirstChildElement("HexMode"))
            elem->FirstChildElement("HexMode")->QueryBoolText(&hexMode);
        else if (elem->FirstChildElement("Hex"))
            elem->FirstChildElement("Hex")->QueryBoolText(&hexMode);

        int digits = 0;
        if (elem->FirstChildElement("Digits"))
            elem->FirstChildElement("Digits")->QueryIntText(&digits);

        int decimalPlaces = 2;
        if (elem->FirstChildElement("DecimalPlaces"))
            elem->FirstChildElement("DecimalPlaces")->QueryIntText(&decimalPlaces);

        return std::make_unique<GlossyDisplayWidget>(
            lbl, (uint16_t)addr, w, h, le,
            dataType, hexMode, digits, decimalPlaces
        );
    }
    else if (name == "LCD_HD44780") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr1 = 0, addr2 = 0;
        const char* line1 = "";
        const char* line2 = "";

        if (elem->FirstChildElement("Line1Address"))
            addr1 = AddressFromHex(elem->FirstChildElement("Line1Address"));
        if (elem->FirstChildElement("Line2Address"))
            addr2 = AddressFromHex(elem->FirstChildElement("Line2Address"));
        if (elem->FirstChildElement("Line1Text"))
            line1 = elem->FirstChildElement("Line1Text")->GetText();
        if (elem->FirstChildElement("Line2Text"))
            line2 = elem->FirstChildElement("Line2Text")->GetText();

        float pSize = 2.5f;
        if (elem->FirstChildElement("PixelSize"))
            elem->FirstChildElement("PixelSize")->QueryFloatText(&pSize);

        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        bool useFixed = false;
        if (elem->FirstChildElement("UseFixed"))
            elem->FirstChildElement("UseFixed")->QueryBoolText(&useFixed);

        return std::make_unique<LCD_HD44780_Widget>(lbl, addr1, line1, addr2, line2, pSize, le, useFixed);
    }
    else if (name == "VerticalGauge") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 100.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        float w = 120.0f, h = 300.0f;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int dataType = FLOAT;
        if (elem->FirstChildElement("DataType"))
            elem->FirstChildElement("DataType")->QueryIntText(&dataType);
        return std::make_unique<VerticalGaugeWidget>(lbl, addr, minV, maxV, w, h, le, dataType);
    }
    else if (name == "NeumorphicKnob") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 1.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        float rad = 50.0f;
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&rad);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int dataType = S16_DEC;
        if (elem->FirstChildElement("DataType"))
            elem->FirstChildElement("DataType")->QueryIntText(&dataType);
        return std::make_unique<NeumorphicKnobWidget>(lbl, addr, minV, maxV, rad, le, dataType);
    }
    else if (name == "StyledKnob") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 1.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        float radius = 50.0f;
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&radius);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int dataType = S16_DEC;
        if (elem->FirstChildElement("DataType"))
            elem->FirstChildElement("DataType")->QueryIntText(&dataType);
        return std::make_unique<StyledKnobWidget>(lbl, addr, minV, maxV, radius, le, dataType);
    }
    else if (name == "CustomVerticalSlider") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 1.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        float w = 20.0f, h = 100.0f;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int dataType = FLOAT;
        if (elem->FirstChildElement("DataType"))
            elem->FirstChildElement("DataType")->QueryIntText(&dataType);
        return std::make_unique<CustomVerticalSliderWidget>(lbl, addr, minV, maxV, w, h, le, dataType);
    }
    else if (name == "BitControl8Bit") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        float scale = 0.55f;
        if (elem->FirstChildElement("Scale"))
            elem->FirstChildElement("Scale")->QueryFloatText(&scale);

        return std::make_unique<BitControl8BitWidget>(lbl, addr, le, scale);
    }
    else if (name == "BitControl") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int modeInt = Bits8;
        if (elem->FirstChildElement("Mode"))
            elem->FirstChildElement("Mode")->QueryIntText(&modeInt);
        BitMode mode = static_cast<BitMode>(modeInt);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        bool switchable = true;
        if (elem->FirstChildElement("Switchable"))
            elem->FirstChildElement("Switchable")->QueryBoolText(&switchable);
        return std::make_unique<BitControlWidget>(lbl, addr, mode, le, switchable);
    }
    else if (name == "RotarySwitch") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int minPos = 0, maxPos = 3;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryIntText(&minPos);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryIntText(&maxPos);
        float rad = 40.0f;
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&rad);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<RotarySwitchWidget>(lbl, addr, minPos, maxPos, rad, le);
    }
    else if (name == "LedIndicator") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float rad = 10.0f;
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&rad);

        ImU32 colorOn = IM_COL32(255, 0, 0, 255);
        ImU32 colorOff = IM_COL32(50, 0, 0, 255);

        if (elem->FirstChildElement("ColorOn")) {
            const char* colStr = elem->FirstChildElement("ColorOn")->GetText();
            if (colStr && colStr[0] == '#') {
                unsigned int hex = 0;
                int ret = sscanf(colStr + 1, "%x", &hex);
                if (strlen(colStr) == 7) {
                    colorOn = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                }
                else if (strlen(colStr) == 9) {
                    colorOn = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
        }
        if (elem->FirstChildElement("ColorOff")) {
            const char* colStr = elem->FirstChildElement("ColorOff")->GetText();
            if (colStr && colStr[0] == '#') {
                unsigned int hex = 0;
                int ret = sscanf(colStr + 1, "%x", &hex);
                if (strlen(colStr) == 7) {
                    colorOff = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                }
                else if (strlen(colStr) == 9) {
                    colorOff = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
        }

        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        return std::make_unique<LedIndicatorWidget>(lbl, addr, rad, colorOn, colorOff, le);
    }
    else if (name == "Image") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        const char* file = elem->FirstChildElement("File") ? elem->FirstChildElement("File")->GetText() : "";
        float scale = 1.0f;
        if (elem->FirstChildElement("Scale"))
            elem->FirstChildElement("Scale")->QueryFloatText(&scale);

        float fixW = 0, fixH = 0;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&fixW);
        if (elem->FirstChildElement("Height"))
            elem->FirstChildElement("Height")->QueryFloatText(&fixH);

        ImU32 tint = IM_COL32(255, 255, 255, 255);
        if (elem->FirstChildElement("Tint")) {
            const char* tintStr = elem->FirstChildElement("Tint")->GetText();
            if (tintStr && tintStr[0] == '#') {
                unsigned int hex = 0;
                int ret = sscanf(tintStr + 1, "%x", &hex);
                if (strlen(tintStr) == 7) {
                    tint = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                }
                else if (strlen(tintStr) == 9) {
                    tint = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
        }

        ImU32 border = 0;
        if (elem->FirstChildElement("Border")) {
            const char* borderStr = elem->FirstChildElement("Border")->GetText();
            if (borderStr && borderStr[0] == '#') {
                unsigned int hex = 0;
                int ret = sscanf(borderStr + 1, "%x", &hex);
                if (strlen(borderStr) == 7) {
                    border = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                }
                else if (strlen(borderStr) == 9) {
                    border = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
                }
            }
        }

        ImVec2 uv0(0, 0), uv1(1, 1);
        if (elem->FirstChildElement("UV0")) {
            const char* uv0Str = elem->FirstChildElement("UV0")->GetText();
            if (uv0Str) { int ret = sscanf(uv0Str, "%f,%f", &uv0.x, &uv0.y); }
        }
        if (elem->FirstChildElement("UV1")) {
            const char* uv1Str = elem->FirstChildElement("UV1")->GetText();
            if (uv1Str) { int ret = sscanf(uv1Str, "%f,%f", &uv1.x, &uv1.y); }
        }

        return std::make_unique<ImageWidget>(lbl, file, scale, fixW, fixH, uv0, uv1, tint, border);
    }
    else if (name == "Gif") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        const char* file = elem->FirstChildElement("File") ? elem->FirstChildElement("File")->GetText() : "";
        float scale = 1.0f;
        if (elem->FirstChildElement("Scale"))
            elem->FirstChildElement("Scale")->QueryFloatText(&scale);
        float fixW = 0, fixH = 0;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&fixW);
        if (elem->FirstChildElement("Height"))
            elem->FirstChildElement("Height")->QueryFloatText(&fixH);
        bool loop = true;
        if (elem->FirstChildElement("Loop"))
            elem->FirstChildElement("Loop")->QueryBoolText(&loop);
        float speed = 1.0f;
        if (elem->FirstChildElement("Speed"))
            elem->FirstChildElement("Speed")->QueryFloatText(&speed);
        return std::make_unique<GifWidget>(lbl, file, fixW, fixH, scale, loop, speed);
    }
    else if (name == "RPMIndicator") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 8000.0f, rad = 100.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&rad);
        const char* unit = "rpm";
        if (elem->FirstChildElement("Unit"))
            unit = elem->FirstChildElement("Unit")->GetText();
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        std::vector<RPMZone> zones;
        XMLElement* zonesElem = elem->FirstChildElement("Zones");
        if (zonesElem) {
            for (XMLElement* zoneElem = zonesElem->FirstChildElement("Zone"); zoneElem; zoneElem = zoneElem->NextSiblingElement()) {
                RPMZone zone;
                float start = 0, end = 0;
                const char* colorStr = nullptr;
                zoneElem->QueryFloatAttribute("start", &start);
                zoneElem->QueryFloatAttribute("end", &end);
                colorStr = zoneElem->Attribute("color");
                if (colorStr && colorStr[0] == '#') {
                    unsigned int hex = 0;
                    int ret = sscanf(colorStr + 1, "%x", &hex);
                    zone.start = start;
                    zone.end = end;
                    zone.color = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 200);
                    zones.push_back(zone);
                }
            }
        }
        if (zones.empty()) {
            zones.push_back({ 0, 4000, IM_COL32(100,200,80,200) });
            zones.push_back({ 4000, 6000, IM_COL32(240,240,80,200) });
            zones.push_back({ 6000, 8000, IM_COL32(230,70,70,200) });
        }
        std::sort(zones.begin(), zones.end());

        return std::make_unique<RPMIndicatorWidget>(lbl, addr, minV, maxV, rad, unit, zones, le);
    }
    else if (name == "Keypad") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int minV = 0, maxV = 9999;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryIntText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryIntText(&maxV);
        int digits = 4;
        if (elem->FirstChildElement("Digits"))
            elem->FirstChildElement("Digits")->QueryIntText(&digits);
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<KeypadWidget>(lbl, addr, minV, maxV, digits, le);
    }
    else if (name == "Equalizer") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        int bands = 10;
        if (elem->FirstChildElement("Bands"))
            elem->FirstChildElement("Bands")->QueryIntText(&bands);
        float minV = 0.0f, maxV = 100.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        float bandWidth = 20.0f, height = 120.0f;
        if (elem->FirstChildElement("BandWidth"))
            elem->FirstChildElement("BandWidth")->QueryFloatText(&bandWidth);
        if (elem->FirstChildElement("Height"))
            elem->FirstChildElement("Height")->QueryFloatText(&height);
        const char* typeStr = elem->FirstChildElement("DataType") ? elem->FirstChildElement("DataType")->GetText() : "float";
        EqDataType dtype = EQ_FLOAT;
        if (strcmp(typeStr, "int16") == 0) dtype = EQ_INT16;
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int step = 2;
        if (dtype == EQ_INT16) step = 1;
        if (elem->FirstChildElement("AddressStep"))
            elem->FirstChildElement("AddressStep")->QueryIntText(&step);
        return std::make_unique<EqualizerWidget>(lbl, addr, bands, minV, maxV, bandWidth, height, dtype, le, step);
    }
    else if (name == "Tank") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 100.0f;
        if (elem->FirstChildElement("Min")) elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max")) elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        float w = 150.0f, h = 250.0f;
        if (elem->FirstChildElement("Width")) elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        const char* unit = "";
        if (elem->FirstChildElement("Unit")) unit = elem->FirstChildElement("Unit")->GetText();
        bool le = true;
        if (elem->FirstChildElement("LittleEndian")) elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        ImU32 liquidColor = IM_COL32(64, 164, 223, 255);
        if (elem->FirstChildElement("LiquidColor")) {
            const char* colStr = elem->FirstChildElement("LiquidColor")->GetText();
            if (colStr && colStr[0] == '#') {
                unsigned int hex = 0;
                sscanf(colStr + 1, "%x", &hex);
                if (strlen(colStr) == 7)
                    liquidColor = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                else if (strlen(colStr) == 9)
                    liquidColor = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
            }
        }
        ImU32 metalColor = IM_COL32(160, 160, 170, 255);
        if (elem->FirstChildElement("MetalColor")) {
            const char* colStr = elem->FirstChildElement("MetalColor")->GetText();
            if (colStr && colStr[0] == '#') {
                unsigned int hex = 0;
                sscanf(colStr + 1, "%x", &hex);
                if (strlen(colStr) == 7)
                    metalColor = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
                else if (strlen(colStr) == 9)
                    metalColor = IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, (hex >> 24) & 0xFF);
            }
        }
        int dataType = 0;
        if (elem->FirstChildElement("DataType")) elem->FirstChildElement("DataType")->QueryIntText(&dataType);
        return std::make_unique<TankWidget>(lbl, addr, minV, maxV, w, h, unit, liquidColor, metalColor, le, dataType);
    }
    else if (name == "VUMeter") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        float minV = 0.0f, maxV = 100.0f;
        if (elem->FirstChildElement("Min"))
            elem->FirstChildElement("Min")->QueryFloatText(&minV);
        if (elem->FirstChildElement("Max"))
            elem->FirstChildElement("Max")->QueryFloatText(&maxV);
        const char* orientStr = elem->FirstChildElement("Orientation") ? elem->FirstChildElement("Orientation")->GetText() : "horizontal";
        VUOrientation orient = VU_HORIZONTAL;
        if (strcmp(orientStr, "vertical") == 0) orient = VU_VERTICAL;
        float w = 200.0f, h = 30.0f;
        if (elem->FirstChildElement("Width"))
            elem->FirstChildElement("Width")->QueryFloatText(&w);
        XMLElement* heightElem = elem->FirstChildElement("Height");
        if (!heightElem) heightElem = elem->FirstChildElement("Height");
        if (heightElem) heightElem->QueryFloatText(&h);
        const char* unit = "";
        if (elem->FirstChildElement("Unit"))
            unit = elem->FirstChildElement("Unit")->GetText();
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        bool peak = false;
        if (elem->FirstChildElement("ShowPeak"))
            elem->FirstChildElement("ShowPeak")->QueryBoolText(&peak);
        return std::make_unique<VUMeterWidget>(lbl, addr, minV, maxV, orient, w, h, unit, le, peak);
    }
    else if (name == "SegmentButton") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool le = false;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);

        std::vector<SegmentItem> segments;
        XMLElement* itemsElem = elem->FirstChildElement("Items");
        if (itemsElem) {
            for (XMLElement* segElem = itemsElem->FirstChildElement("Segment"); segElem; segElem = segElem->NextSiblingElement()) {
                SegmentItem item;
                const char* text = segElem->FirstChildElement("Text") ? segElem->FirstChildElement("Text")->GetText() : "";
                int val = 0;
                if (segElem->FirstChildElement("Value"))
                    segElem->FirstChildElement("Value")->QueryIntText(&val);
                item.text = text;
                item.value = val;
                segments.push_back(item);
            }
        }

        if (segments.empty()) {
            segments.push_back({ "Low", 0 });
            segments.push_back({ "Med", 1 });
            segments.push_back({ "High", 2 });
        }

        return std::make_unique<SegmentButtonWidget>(lbl, addr, segments, le);
    }
    else if (name == "GlossyButton") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t addr = 0;
        if (elem->FirstChildElement("Address"))
            addr = AddressFromHex(elem->FirstChildElement("Address"));
        bool le = true;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        int mode = 1;
        if (elem->FirstChildElement("Mode"))
            elem->FirstChildElement("Mode")->QueryIntText(&mode);
        int writeVal = 1;
        if (elem->FirstChildElement("Value"))
            elem->FirstChildElement("Value")->QueryIntText(&writeVal);
        float radius = 20.0f;
        if (elem->FirstChildElement("Radius"))
            elem->FirstChildElement("Radius")->QueryFloatText(&radius);
        return std::make_unique<GlossyButtonWidget>(lbl, addr, le, (GlossyButtonMode)mode, writeVal, radius);
    }
    else if (name == "MemoryView") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t base = 0;
        if (elem->FirstChildElement("BaseAddress"))
            base = AddressFromHex(elem->FirstChildElement("BaseAddress"));
        int bytes = 256;
        if (elem->FirstChildElement("NumBytes"))
            elem->FirstChildElement("NumBytes")->QueryIntText(&bytes);
        int bpl = 16;
        if (elem->FirstChildElement("BytesPerLine"))
            elem->FirstChildElement("BytesPerLine")->QueryIntText(&bpl);
        bool le = true;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        return std::make_unique<MemoryViewWidget>(lbl, base, bytes, bpl, le);
    }
    else if (name == "MemoryEditor") {
        const char* lbl = elem->FirstChildElement("Text") ? elem->FirstChildElement("Text")->GetText() : "";
        uint32_t base = 0;
        if (elem->FirstChildElement("BaseAddress"))
            base = AddressFromHex(elem->FirstChildElement("BaseAddress"));
        int size = 256;
        if (elem->FirstChildElement("Size"))
            elem->FirstChildElement("Size")->QueryIntText(&size);
        int cols = 16;
        if (elem->FirstChildElement("Cols"))
            elem->FirstChildElement("Cols")->QueryIntText(&cols);
        bool readOnly = false;
        if (elem->FirstChildElement("ReadOnly"))
            elem->FirstChildElement("ReadOnly")->QueryBoolText(&readOnly);
        bool showAscii = true;
        if (elem->FirstChildElement("ShowAscii"))
            elem->FirstChildElement("ShowAscii")->QueryBoolText(&showAscii);
        bool showHexII = false;
        if (elem->FirstChildElement("ShowHexII"))
            elem->FirstChildElement("ShowHexII")->QueryBoolText(&showHexII);
        bool showOptions = true;
        if (elem->FirstChildElement("ShowOptions"))
            elem->FirstChildElement("ShowOptions")->QueryBoolText(&showOptions);
        bool showPreview = true;
        if (elem->FirstChildElement("ShowPreview"))
            elem->FirstChildElement("ShowPreview")->QueryBoolText(&showPreview);
        bool greyZero = true;
        if (elem->FirstChildElement("GreyZero"))
            elem->FirstChildElement("GreyZero")->QueryBoolText(&greyZero);
        bool upperHex = true;
        if (elem->FirstChildElement("UpperHex"))
            elem->FirstChildElement("UpperHex")->QueryBoolText(&upperHex);
        bool le = true;
        if (elem->FirstChildElement("LittleEndian"))
            elem->FirstChildElement("LittleEndian")->QueryBoolText(&le);
        bool useExternal = false;
        if (elem->FirstChildElement("UseExternal"))
            elem->FirstChildElement("UseExternal")->QueryBoolText(&useExternal);
        bool useCache = true; // по умолчанию true
        if (elem->FirstChildElement("UseCache"))
            elem->FirstChildElement("UseCache")->QueryBoolText(&useCache);

        return std::make_unique<MemoryEditorWidget>(lbl, base, (size_t)size, cols,
            readOnly, showAscii, showHexII,
            showOptions, showPreview,
            greyZero, upperHex, le, useExternal, useCache);
            }
    return nullptr;
}

// Загрузка всех окон из XML
void LoadWindowsFromXML(MainContext* ctx, const char* filename) {
    XMLDocument doc;
    if (doc.LoadFile(filename) != XML_SUCCESS) {
        printf("ERROR: Failed to load XML file: %s\n", filename);
        return;
    }
    printf("XML loaded successfully: %s\n", filename);
    g_Windows.clear();
    int winCount = 0;
    for (XMLElement* win = doc.FirstChildElement("Window"); win; win = win->NextSiblingElement()) {
        winCount++;
        WindowData wd;
        const char* name = win->FirstChildElement("Text") ? win->FirstChildElement("Text")->GetText() : "Unnamed";
        wd.name = name;
        wd.show = true;
        //ctx->w[winCount].name = name;
        //ctx->w[winCount].show = true;

        // Обработка IOMap (если есть)
        XMLElement* iomap = win->FirstChildElement("IOMap");
        if (iomap && ctx->StartComunication && ctx->Sync == SDL_FALSE) {
            XMLElement* items = iomap->FirstChildElement("Items");
            if (items) {
                SDL_LockMutex(ctx->Datalock);
                ctx->Sync = SDL_TRUE; // помечаем, что синхронизация для этого окна выполнена
                for (XMLElement* elem = items->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
                    uint32_t addr = 0;
                    int size = 0;

                    // Читаем адрес
                    XMLElement* addrElem = TryFindElement(elem, "Address");
                    if (addrElem) {
                        addr = AddressFromHex(addrElem);
                    }

                    // Читаем размер
                    XMLElement* sizeElem = TryFindElement(elem, "Size");
                    if (sizeElem) {
                        sizeElem->QueryIntText(&size);
                    }

                    // Если адрес и размер корректны, ставим задачу на чтение
                    if (addr != 0 && size > 0) {
                        // Вызов DataReadWriteArray с data = nullptr означает чтение
                        DataReadWriteArray(ctx, nullptr, nullptr, &addr, (uint16_t)size);
                    }
                }
                SDL_CondSignal(ctx->Datacond); // будим поток обработки очередей
                SDL_UnlockMutex(ctx->Datalock);
            }
        }

        // Загрузка визуальных элементов
        XMLElement* items = win->FirstChildElement("Items");
        if (items) {
            auto rootGroup = std::make_unique<GroupWidget>();
            for (XMLElement* child = items->FirstChildElement(); child; child = child->NextSiblingElement()) {
                auto widget = LoadWidgetFromXML(child);
                if (widget) {
                    rootGroup->AddChild(std::move(widget));
                }
                else {
                    printf("Warning: could not create widget for element <%s> in window '%s'\n",
                        child->Name(), wd.name.c_str());
                }
            }
            wd.root = std::move(rootGroup);
        }
        else {
            printf("Window '%s' has no <Items> – empty window.\n", wd.name.c_str());
        }
        g_Windows.push_back(std::move(wd));
    }
    printf("Loaded %d windows.\n", winCount);
    ctx->file_loaded = true;
}

// Функция отрисовки всех окон
int Draw_Window(MainContext* ctx) {

    if (!ctx) return -1;
    if (g_Windows.empty()) {
        if (ctx->File_path[0] != '\0' && !ctx->file_loaded) {
            LoadWindowsFromXML(ctx, ctx->File_path);
        }
        if (g_Windows.empty()) {
            ImGui::Text("No windows loaded. Check XML file.");
            return -1;
        }
    }

    for (auto& win : g_Windows) {
        if (!win.show) continue;
        ImGui::SetNextWindowBgAlpha(0.9f);
        if (ImGui::Begin(win.name.c_str(), &win.show, ImGuiWindowFlags_None)) {
            if (win.root) {
                win.root->Draw(ctx, nullptr);
            }
        }
        ImGui::End();
    }
    return 0;
}

// Очистка всех окон
extern "C" void ClearAllWindows(void) {
    g_Windows.clear();
}