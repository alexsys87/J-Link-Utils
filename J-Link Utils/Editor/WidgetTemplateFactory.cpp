// WidgetTemplateFactory.cpp
#include "WidgetTemplateFactory.h"
#include <tinyxml2/tinyxml2.h>

namespace WidgetTemplateFactory {

    // ==================================================================
    // Функции создания корневых виджетов
    // ==================================================================
    tinyxml2::XMLElement* CreateNewWindow(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Window");
        auto text = doc->NewElement("Text"); text->SetText("New Window"); elem->InsertEndChild(text);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateText(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Text");
        auto text = doc->NewElement("Text");
        text->SetText("New Text");
        elem->InsertEndChild(text);
        return elem;
    }

    tinyxml2::XMLElement* CreateTextColored(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("TextColored");
        auto text = doc->NewElement("Text"); text->SetText("Colored"); elem->InsertEndChild(text);
        auto color = doc->NewElement("Color"); color->SetText("#FF00FF"); elem->InsertEndChild(color);
        return elem;
    }

    // Button
    tinyxml2::XMLElement* CreateButton(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Button");
        auto text = doc->NewElement("Text"); text->SetText("Button"); elem->InsertEndChild(text);
        auto w = doc->NewElement("Width"); w->SetText("80"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("30"); elem->InsertEndChild(h);
        auto mode = doc->NewElement("Mode"); mode->SetText("0"); elem->InsertEndChild(mode);
        auto val = doc->NewElement("Value"); val->SetText("1"); elem->InsertEndChild(val);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateCheckbox(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Checkbox");
        auto text = doc->NewElement("Text"); text->SetText("Checkbox"); elem->InsertEndChild(text);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); elem->InsertEndChild(addr);
        auto useBool = doc->NewElement("AddrBool"); useBool->SetText("false"); elem->InsertEndChild(useBool);
        return elem;
    }

    tinyxml2::XMLElement* CreateGroup(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Group");
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateTreeNode(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("TreeNode");
        auto text = doc->NewElement("Text"); text->SetText("Node"); elem->InsertEndChild(text);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateSameLine(tinyxml2::XMLDocument* doc) {
        return doc->NewElement("SameLine");
    }

    tinyxml2::XMLElement* CreateSpacing(tinyxml2::XMLDocument* doc) {
        return doc->NewElement("Spacing");
    }

    tinyxml2::XMLElement* CreateSeparator(tinyxml2::XMLDocument* doc) {
        return doc->NewElement("Separator");
    }

    tinyxml2::XMLElement* CreateDummy(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Dummy");
        auto w = doc->NewElement("Width"); w->SetText("50"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("50"); elem->InsertEndChild(h);
        return elem;
    }

    tinyxml2::XMLElement* CreateGrid(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Grid");
        auto rows = doc->NewElement("Row"); rows->SetText("2"); elem->InsertEndChild(rows);
        auto cols = doc->NewElement("Column"); cols->SetText("2"); elem->InsertEndChild(cols);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); elem->InsertEndChild(addr);
        auto inc = doc->NewElement("AddressInc"); inc->SetText("1"); elem->InsertEndChild(inc);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    // PlotLines с одной линией по умолчанию
    tinyxml2::XMLElement* CreatePlotLines(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("PlotLines");
        auto text = doc->NewElement("Text"); text->SetText("Plot"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto w = doc->NewElement("Width"); w->SetText("-1"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("-1"); elem->InsertEndChild(h);
        auto f = doc->NewElement("Freq"); f->SetText("1"); elem->InsertEndChild(f);
        auto sz = doc->NewElement("Size"); sz->SetText("100"); elem->InsertEndChild(sz);
        auto sm = doc->NewElement("Smooth"); sm->SetText("0"); elem->InsertEndChild(sm);
        auto maxSize = doc->NewElement("MaxSize"); maxSize->SetText("2000"); elem->InsertEndChild(maxSize);
        auto autosize = doc->NewElement("AutoSize"); autosize->SetText("false"); elem->InsertEndChild(autosize);

        auto items = doc->NewElement("Items");
        auto line = doc->NewElement("Line");
        auto name = doc->NewElement("Name"); name->SetText("Line"); line->InsertEndChild(name);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); line->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); line->InsertEndChild(dtype); // FLOAT
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); line->InsertEndChild(le);
        auto scale = doc->NewElement("Scale"); scale->SetText("1.0"); line->InsertEndChild(scale);
        auto bufId = doc->NewElement("BufferId"); bufId->SetText("0"); line->InsertEndChild(bufId);
        auto lineColor = doc->NewElement("LineColor"); lineColor->SetText("#FFFFFFFF"); line->InsertEndChild(lineColor);
        auto weight = doc->NewElement("LineWeight"); weight->SetText("1.0"); line->InsertEndChild(weight);
        items->InsertEndChild(line);
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateTextFormatted(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("TextFormatted");
        auto fmt = doc->NewElement("Format"); fmt->SetText("%d"); elem->InsertEndChild(fmt);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto color = doc->NewElement("Color"); color->SetText("0xCC000000"); elem->InsertEndChild(color);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateTextFormattedF(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("TextFormattedF");
        auto fmt = doc->NewElement("Format"); fmt->SetText("%.2f"); elem->InsertEndChild(fmt);
        auto color = doc->NewElement("Color"); color->SetText("0xCC000000"); elem->InsertEndChild(color);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateAlignText(tinyxml2::XMLDocument* doc) {
        return doc->NewElement("AlignTextToFramePadding");
    }

    tinyxml2::XMLElement* CreateNewLine(tinyxml2::XMLDocument* doc) {
        return doc->NewElement("NewLine");
    }

    tinyxml2::XMLElement* CreateLabelText(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("LabelText");
        auto lbl = doc->NewElement("Label"); lbl->SetText("Label"); elem->InsertEndChild(lbl);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%d"); elem->InsertEndChild(fmt);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateRadioButton(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("RadioButton");
        auto sameLine = doc->NewElement("InSameLine"); sameLine->SetText("false"); elem->InsertEndChild(sameLine);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto items = doc->NewElement("Items");
        // Добавим два примера
        auto radio1 = doc->NewElement("Radio");
        auto lbl1 = doc->NewElement("Label"); lbl1->SetText("Option 1"); radio1->InsertEndChild(lbl1);
        auto val1 = doc->NewElement("Value"); val1->SetText("0"); radio1->InsertEndChild(val1);
        items->InsertEndChild(radio1);
        auto radio2 = doc->NewElement("Radio");
        auto lbl2 = doc->NewElement("Label"); lbl2->SetText("Option 2"); radio2->InsertEndChild(lbl2);
        auto val2 = doc->NewElement("Value"); val2->SetText("1"); radio2->InsertEndChild(val2);
        items->InsertEndChild(radio2);
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateTooltip(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Tooltip");
        auto text = doc->NewElement("Text"); text->SetText("Tooltip text"); elem->InsertEndChild(text);
        return elem;
    }

    tinyxml2::XMLElement* CreatePlotTooltip(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("PlotTooltip");
        auto text = doc->NewElement("Text"); text->SetText("Tooltip"); elem->InsertEndChild(text);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%.2f"); elem->InsertEndChild(fmt);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto size = doc->NewElement("Size"); size->SetText("100"); elem->InsertEndChild(size);
        auto id = doc->NewElement("BufferId"); id->SetText("0"); elem->InsertEndChild(id);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); elem->InsertEndChild(dtype); // FLOAT
        auto autoSz = doc->NewElement("AutoSize"); autoSz->SetText("false"); elem->InsertEndChild(autoSz);
        return elem;
    }

    tinyxml2::XMLElement* CreateComboBox(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ComboBox");
        auto text = doc->NewElement("Text"); text->SetText("Combo"); elem->InsertEndChild(text);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto items = doc->NewElement("Items");  elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateInputInt(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ImputInt"); // опечатка сохранена для совместимости
        auto text = doc->NewElement("Text"); text->SetText("Input"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto step = doc->NewElement("Step"); step->SetText("1"); elem->InsertEndChild(step);
        auto fast = doc->NewElement("Fast"); fast->SetText("10"); elem->InsertEndChild(fast);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateInputFloat(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ImputFloat");
        auto text = doc->NewElement("Text"); text->SetText("Input"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0.0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("1.0"); elem->InsertEndChild(max);
        auto step = doc->NewElement("Step"); step->SetText("0.1"); elem->InsertEndChild(step);
        auto fast = doc->NewElement("Fast"); fast->SetText("0.5"); elem->InsertEndChild(fast);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%.3f"); elem->InsertEndChild(fmt);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateFixedWidth(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("FixedWidth");
        auto w = doc->NewElement("Width"); w->SetText("100"); elem->InsertEndChild(w);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateSetCursorPos(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("SetCursorPos");
        auto x = doc->NewElement("Width"); x->SetText("0"); elem->InsertEndChild(x);
        auto y = doc->NewElement("Height"); y->SetText("0"); elem->InsertEndChild(y);
        return elem;
    }

    tinyxml2::XMLElement* CreateIndent(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Indent");
        auto w = doc->NewElement("Width"); w->SetText("20"); elem->InsertEndChild(w);
        return elem;
    }

    // Histogram с поддержкой ScaleX, Smooth, DoneAddr, AutoSize, BarWidth и Items
    tinyxml2::XMLElement* CreateHistogram(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Histogram");
        auto text = doc->NewElement("Text"); text->SetText("Histogram"); elem->InsertEndChild(text);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%.2f"); elem->InsertEndChild(fmt);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto w = doc->NewElement("Width"); w->SetText("200"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("80"); elem->InsertEndChild(h);
        auto xs = doc->NewElement("ScaleX"); xs->SetText("1.0"); elem->InsertEndChild(xs);
        auto sz = doc->NewElement("Size"); sz->SetText("100"); elem->InsertEndChild(sz);
        auto sm = doc->NewElement("Smooth"); sm->SetText("0"); elem->InsertEndChild(sm);
        auto done = doc->NewElement("DoneAddr"); done->SetText("0"); elem->InsertEndChild(done);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto autoSz = doc->NewElement("AutoSize"); autoSz->SetText("false"); elem->InsertEndChild(autoSz);
        auto barWidth = doc->NewElement("BarWidth"); barWidth->SetText("0.8"); elem->InsertEndChild(barWidth);

        auto items = doc->NewElement("Items");
        auto line = doc->NewElement("Line");
        auto name = doc->NewElement("Name"); name->SetText("Data"); line->InsertEndChild(name);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); line->InsertEndChild(addr);
        auto yScale = doc->NewElement("ScaleY"); yScale->SetText("1.0"); line->InsertEndChild(yScale);
        auto color = doc->NewElement("Color"); color->SetText("#FFFFFFFF"); line->InsertEndChild(color);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); line->InsertEndChild(dtype); // FLOAT
        items->InsertEndChild(line);
        elem->InsertEndChild(items);
        return elem;
    }

    // DataLog с поддержкой ScaleX, Smooth, DoneAddr, AutoSize и Items
    tinyxml2::XMLElement* CreateDataLog(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("DataLog");
        auto text = doc->NewElement("Text"); text->SetText("DataLog"); elem->InsertEndChild(text);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%.2f"); elem->InsertEndChild(fmt);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto w = doc->NewElement("Width"); w->SetText("200"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("150"); elem->InsertEndChild(h);
        auto xs = doc->NewElement("ScaleX"); xs->SetText("1.0"); elem->InsertEndChild(xs);
        auto sz = doc->NewElement("Size"); sz->SetText("100"); elem->InsertEndChild(sz);
        auto sm = doc->NewElement("Smooth"); sm->SetText("0"); elem->InsertEndChild(sm);
        auto done = doc->NewElement("DoneAddr"); done->SetText("0"); elem->InsertEndChild(done);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto autoSz = doc->NewElement("AutoSize"); autoSz->SetText("false"); elem->InsertEndChild(autoSz);

        auto items = doc->NewElement("Items");
        auto line = doc->NewElement("Line");
        auto name = doc->NewElement("Name"); name->SetText("Data"); line->InsertEndChild(name);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); line->InsertEndChild(addr);
        auto yScale = doc->NewElement("ScaleY"); yScale->SetText("1.0"); line->InsertEndChild(yScale);
        auto color = doc->NewElement("Color"); color->SetText("#FFFFFFFF"); line->InsertEndChild(color);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); line->InsertEndChild(dtype);
        items->InsertEndChild(line);
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateProgressBar(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ProgressBar");
        auto w = doc->NewElement("Width"); w->SetText("200"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("20"); elem->InsertEndChild(h);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateSliderFloat(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("SliderFloat");
        auto text = doc->NewElement("Text"); text->SetText("Slider"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0.0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("1.0"); elem->InsertEndChild(max);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%.3f"); elem->InsertEndChild(fmt);
        auto step = doc->NewElement("Step"); step->SetText("0.01"); elem->InsertEndChild(step);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateSliderInt(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("SliderInt");
        auto text = doc->NewElement("Text"); text->SetText("Slider"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto fmt = doc->NewElement("Format"); fmt->SetText("%d"); elem->InsertEndChild(fmt);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateTabBar(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("TabBar");
        auto text = doc->NewElement("Text"); text->SetText("TabBar"); elem->InsertEndChild(text);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateTab(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Tab");
        auto text = doc->NewElement("Text"); text->SetText("Tab"); elem->InsertEndChild(text);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateColumn(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Column");
        auto cnt = doc->NewElement("Count"); cnt->SetText("2"); elem->InsertEndChild(cnt);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateSelectableText(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("SelectableText");
        auto fmt = doc->NewElement("Format"); fmt->SetText("%d"); elem->InsertEndChild(fmt);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateChildWindow(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ChildWindow");
        auto text = doc->NewElement("Text"); text->SetText("Child"); elem->InsertEndChild(text);
        auto w = doc->NewElement("Width"); w->SetText("200"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("150"); elem->InsertEndChild(h);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateHeader(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Header");
        auto text = doc->NewElement("Text"); text->SetText("Header"); elem->InsertEndChild(text);
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreatePopup(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Popup");
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateHintBox(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("HintBox");
        auto items = doc->NewElement("Items");
        elem->InsertEndChild(items);
        return elem;
    }

    tinyxml2::XMLElement* CreateHelpMarker(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("HelpMarker");
        auto text = doc->NewElement("Text"); text->SetText("Help text"); elem->InsertEndChild(text);
        return elem;
    }

    tinyxml2::XMLElement* CreateWatch(tinyxml2::XMLDocument* doc) {
        return doc->NewElement("Watch");
    }

    tinyxml2::XMLElement* CreateSevenSegment(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("SevenSegment");
        auto text = doc->NewElement("Text"); text->SetText("7Seg"); elem->InsertEndChild(text);
        auto w = doc->NewElement("Width"); w->SetText("80"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("40"); elem->InsertEndChild(h);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateMechanicalSwitch2(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("MechanicalSwitch2");
        auto text = doc->NewElement("Text"); text->SetText("Switch"); elem->InsertEndChild(text);
        auto w = doc->NewElement("Width"); w->SetText("40"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("40"); elem->InsertEndChild(h);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateGauge(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Gauge");
        auto text = doc->NewElement("Text"); text->SetText("Gauge"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("200"); elem->InsertEndChild(max);
        auto rad = doc->NewElement("Radius"); rad->SetText("100"); elem->InsertEndChild(rad);
        auto unit = doc->NewElement("Unit"); unit->SetText("rpm"); elem->InsertEndChild(unit);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    // GlossyDisplay с DataType и DecimalPlaces
    tinyxml2::XMLElement* CreateGlossyDisplay(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("GlossyDisplay");
        auto text = doc->NewElement("Text"); text->SetText("Display"); elem->InsertEndChild(text);
        auto w = doc->NewElement("Width"); w->SetText("120"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("60"); elem->InsertEndChild(h);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("4"); elem->InsertEndChild(dtype); // U16_DEC
        auto hex = doc->NewElement("Hex"); hex->SetText("false"); elem->InsertEndChild(hex);
        auto digits = doc->NewElement("Digits"); digits->SetText("0"); elem->InsertEndChild(digits);
        auto decPlaces = doc->NewElement("DecimalPlaces"); decPlaces->SetText("2"); elem->InsertEndChild(decPlaces);
        return elem;
    }

    // LCD_HD44780 с UseFixed
    tinyxml2::XMLElement* CreateLCD_HD44780(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("LCD_HD44780");
        auto text = doc->NewElement("Text"); text->SetText("LCD"); elem->InsertEndChild(text);
        auto addr1 = doc->NewElement("Line1Address"); addr1->SetText("0x0000"); elem->InsertEndChild(addr1);
        auto line1 = doc->NewElement("Line1Text"); line1->SetText("Hello"); elem->InsertEndChild(line1);
        auto addr2 = doc->NewElement("Line2Address"); addr2->SetText("0x0002"); elem->InsertEndChild(addr2);
        auto line2 = doc->NewElement("Line2Text"); line2->SetText("World"); elem->InsertEndChild(line2);
        auto pSize = doc->NewElement("PixelSize"); pSize->SetText("2.5"); elem->InsertEndChild(pSize);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto useFixed = doc->NewElement("UseFixed"); useFixed->SetText("false"); elem->InsertEndChild(useFixed);
        return elem;
    }

    // VerticalGauge с DataType
    tinyxml2::XMLElement* CreateVerticalGauge(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("VerticalGauge");
        auto text = doc->NewElement("Text"); text->SetText("V Gauge"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto w = doc->NewElement("Width"); w->SetText("80"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("200"); elem->InsertEndChild(h);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); elem->InsertEndChild(dtype); // FLOAT
        return elem;
    }

    // NeumorphicKnob с DataType
    tinyxml2::XMLElement* CreateNeumorphicKnob(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("NeumorphicKnob");
        auto text = doc->NewElement("Text"); text->SetText("Knob"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("1"); elem->InsertEndChild(max);
        auto rad = doc->NewElement("Radius"); rad->SetText("50"); elem->InsertEndChild(rad);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("3"); elem->InsertEndChild(dtype); // S16_DEC
        return elem;
    }

    // StyledKnob с DataType
    tinyxml2::XMLElement* CreateStyledKnob(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("StyledKnob");
        auto text = doc->NewElement("Text"); text->SetText("Knob"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("1"); elem->InsertEndChild(max);
        auto factor = doc->NewElement("RadiusFactor"); factor->SetText("2.5"); elem->InsertEndChild(factor);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("3"); elem->InsertEndChild(dtype);
        return elem;
    }

    // CustomVerticalSlider с DataType
    tinyxml2::XMLElement* CreateCustomVerticalSlider(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("CustomVerticalSlider");
        auto text = doc->NewElement("Text"); text->SetText("VSlider"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("1"); elem->InsertEndChild(max);
        auto w = doc->NewElement("Width"); w->SetText("20"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("100"); elem->InsertEndChild(h);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); elem->InsertEndChild(dtype);
        return elem;
    }

    // BitControl8Bit с Scale
    tinyxml2::XMLElement* CreateBitControl8Bit(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BitControl8Bit");
        auto text = doc->NewElement("Text"); text->SetText("Bits"); elem->InsertEndChild(text);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto scale = doc->NewElement("Scale"); scale->SetText("0.55"); elem->InsertEndChild(scale);
        return elem;
    }

    tinyxml2::XMLElement* CreateBitControl(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BitControl");
        auto text = doc->NewElement("Text"); text->SetText("BitCtrl"); elem->InsertEndChild(text);
        auto mode = doc->NewElement("Mode"); mode->SetText("0"); elem->InsertEndChild(mode); // Bits8 = 0      
        auto sw = doc->NewElement("Switchable"); sw->SetText("true"); elem->InsertEndChild(sw);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateRotarySwitch(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("RotarySwitch");
        auto text = doc->NewElement("Text"); text->SetText("Rotary"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("3"); elem->InsertEndChild(max);
        auto rad = doc->NewElement("Radius"); rad->SetText("40"); elem->InsertEndChild(rad);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateLedIndicator(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("LedIndicator");
        auto text = doc->NewElement("Text"); text->SetText("LED"); elem->InsertEndChild(text);
        auto rad = doc->NewElement("Radius"); rad->SetText("10"); elem->InsertEndChild(rad);
        auto colOn = doc->NewElement("ColorOn"); colOn->SetText("#FF0000"); elem->InsertEndChild(colOn);
        auto colOff = doc->NewElement("ColorOff"); colOff->SetText("#330000"); elem->InsertEndChild(colOff);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    // Image с Width и Height
    tinyxml2::XMLElement* CreateImage(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Image");
        auto text = doc->NewElement("Text"); text->SetText("Image"); elem->InsertEndChild(text);
        auto file = doc->NewElement("File"); file->SetText("image.png"); elem->InsertEndChild(file);
        auto scale = doc->NewElement("Scale"); scale->SetText("1.0"); elem->InsertEndChild(scale);
        auto w = doc->NewElement("Width"); w->SetText("0"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("0"); elem->InsertEndChild(h);
        auto tint = doc->NewElement("Tint"); tint->SetText("#FFFFFFFF"); elem->InsertEndChild(tint);
        auto border = doc->NewElement("Border"); border->SetText("#00000000"); elem->InsertEndChild(border);
        auto uv0 = doc->NewElement("UV0"); uv0->SetText("0.0,0.0"); elem->InsertEndChild(uv0);
        auto uv1 = doc->NewElement("UV1"); uv1->SetText("1.0,1.0"); elem->InsertEndChild(uv1);
        return elem;
    }

    tinyxml2::XMLElement* CreateRPMIndicator(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("RPMIndicator");
        auto text = doc->NewElement("Text"); text->SetText("RPM"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("8000"); elem->InsertEndChild(max);
        auto rad = doc->NewElement("Radius"); rad->SetText("100"); elem->InsertEndChild(rad);
        auto unit = doc->NewElement("Unit"); unit->SetText("rpm"); elem->InsertEndChild(unit);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        // Добавляем зоны по умолчанию
        auto zones = doc->NewElement("Zones");
        auto zone1 = doc->NewElement("Zone"); zone1->SetAttribute("start", "0"); zone1->SetAttribute("end", "4000"); zone1->SetAttribute("color", "#64C850"); zones->InsertEndChild(zone1);
        auto zone2 = doc->NewElement("Zone"); zone2->SetAttribute("start", "4000"); zone2->SetAttribute("end", "6000"); zone2->SetAttribute("color", "#F0F050"); zones->InsertEndChild(zone2);
        auto zone3 = doc->NewElement("Zone"); zone3->SetAttribute("start", "6000"); zone3->SetAttribute("end", "8000"); zone3->SetAttribute("color", "#E64646"); zones->InsertEndChild(zone3);
        elem->InsertEndChild(zones);
        return elem;
    }

    tinyxml2::XMLElement* CreateKeypad(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Keypad");
        auto text = doc->NewElement("Text"); text->SetText("Keypad"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("9999"); elem->InsertEndChild(max);
        auto digits = doc->NewElement("Digits"); digits->SetText("4"); elem->InsertEndChild(digits);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    tinyxml2::XMLElement* CreateEqualizer(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Equalizer");
        auto text = doc->NewElement("Text"); text->SetText("EQ"); elem->InsertEndChild(text);
        auto bands = doc->NewElement("Bands"); bands->SetText("10"); elem->InsertEndChild(bands);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto bw = doc->NewElement("BandWidth"); bw->SetText("20"); elem->InsertEndChild(bw);
        auto h = doc->NewElement("Height"); h->SetText("120"); elem->InsertEndChild(h);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        return elem;
    }

    // Tank с DataType
    tinyxml2::XMLElement* CreateTank(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Tank");
        auto text = doc->NewElement("Text"); text->SetText("Tank"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto w = doc->NewElement("Width"); w->SetText("150"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("250"); elem->InsertEndChild(h);
        auto unit = doc->NewElement("Unit"); unit->SetText("l"); elem->InsertEndChild(unit);
        auto liq = doc->NewElement("LiquidColor"); liq->SetText("#40A0DF"); elem->InsertEndChild(liq);
        auto metal = doc->NewElement("MetalColor"); metal->SetText("#A0A0AA"); elem->InsertEndChild(metal);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("0"); elem->InsertEndChild(dtype);
        return elem;
    }

    // VUMeter с Unit и ShowPeak
    tinyxml2::XMLElement* CreateVUMeter(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("VUMeter");
        auto text = doc->NewElement("Text"); text->SetText("VU"); elem->InsertEndChild(text);
        auto min = doc->NewElement("Min"); min->SetText("0"); elem->InsertEndChild(min);
        auto max = doc->NewElement("Max"); max->SetText("100"); elem->InsertEndChild(max);
        auto orient = doc->NewElement("Orientation"); orient->SetText("horizontal"); elem->InsertEndChild(orient);
        auto w = doc->NewElement("Width"); w->SetText("200"); elem->InsertEndChild(w);
        auto h = doc->NewElement("Height"); h->SetText("30"); elem->InsertEndChild(h);
        auto unit = doc->NewElement("Unit"); unit->SetText(""); elem->InsertEndChild(unit);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto peak = doc->NewElement("ShowPeak"); peak->SetText("false"); elem->InsertEndChild(peak);
        return elem;
    }

    tinyxml2::XMLElement* CreateSegmentButton(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("SegmentButton");
        auto text = doc->NewElement("Text"); text->SetText("Segment"); elem->InsertEndChild(text);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
        auto items = doc->NewElement("Items");
        auto seg1 = doc->NewElement("Segment");
        auto txt1 = doc->NewElement("Text"); txt1->SetText("Low"); seg1->InsertEndChild(txt1);
        auto val1 = doc->NewElement("Value"); val1->SetText("0"); seg1->InsertEndChild(val1);
        items->InsertEndChild(seg1);
        auto seg2 = doc->NewElement("Segment");
        auto txt2 = doc->NewElement("Text"); txt2->SetText("Med"); seg2->InsertEndChild(txt2);
        auto val2 = doc->NewElement("Value"); val2->SetText("1"); seg2->InsertEndChild(val2);
        items->InsertEndChild(seg2);
        auto seg3 = doc->NewElement("Segment");
        auto txt3 = doc->NewElement("Text"); txt3->SetText("High"); seg3->InsertEndChild(txt3);
        auto val3 = doc->NewElement("Value"); val3->SetText("2"); seg3->InsertEndChild(val3);
        items->InsertEndChild(seg3);
        elem->InsertEndChild(items);
        return elem;
    }

    // MemoryView без WordSize
    tinyxml2::XMLElement* CreateMemoryView(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("MemoryView");
        auto text = doc->NewElement("Text"); text->SetText("Memory"); elem->InsertEndChild(text);
        auto base = doc->NewElement("BaseAddress"); base->SetText("0x0000"); elem->InsertEndChild(base);
        auto bytes = doc->NewElement("NumBytes"); bytes->SetText("256"); elem->InsertEndChild(bytes);
        auto bpl = doc->NewElement("BytesPerLine"); bpl->SetText("16"); elem->InsertEndChild(bpl);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
        return elem;
    }

        tinyxml2::XMLElement* CreateGif(tinyxml2::XMLDocument * doc) {
            auto elem = doc->NewElement("Gif");
            auto text = doc->NewElement("Text"); text->SetText("GIF"); elem->InsertEndChild(text);
            auto file = doc->NewElement("File"); file->SetText("animation.gif"); elem->InsertEndChild(file);
            auto scale = doc->NewElement("Scale"); scale->SetText("1.0"); elem->InsertEndChild(scale);
            auto w = doc->NewElement("Width"); w->SetText("0"); elem->InsertEndChild(w);
            auto h = doc->NewElement("Height"); h->SetText("0"); elem->InsertEndChild(h);
            auto loop = doc->NewElement("Loop"); loop->SetText("true"); elem->InsertEndChild(loop);
            auto speed = doc->NewElement("Speed"); speed->SetText("1.0"); elem->InsertEndChild(speed);
            return elem;
        }

        tinyxml2::XMLElement* CreateGlossyButton(tinyxml2::XMLDocument * doc) {
            auto elem = doc->NewElement("GlossyButton");
            auto text = doc->NewElement("Text"); text->SetText("Button"); elem->InsertEndChild(text);
            auto addr = doc->NewElement("Address"); addr->SetText("0x0000"); elem->InsertEndChild(addr);
            auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
            auto mode = doc->NewElement("Mode"); mode->SetText("1"); elem->InsertEndChild(mode);
            auto val = doc->NewElement("Value"); val->SetText("1"); elem->InsertEndChild(val);
            auto rad = doc->NewElement("Radius"); rad->SetText("20.0"); elem->InsertEndChild(rad);
            return elem;
        }

        tinyxml2::XMLElement* CreateMemoryEditor(tinyxml2::XMLDocument* doc) {
            auto elem = doc->NewElement("MemoryEditor");
            auto text = doc->NewElement("Text"); text->SetText("Memory Editor"); elem->InsertEndChild(text);
            auto base = doc->NewElement("BaseAddress"); base->SetText("0x0000"); elem->InsertEndChild(base);
            auto size = doc->NewElement("Size"); size->SetText("256"); elem->InsertEndChild(size);
            auto cols = doc->NewElement("Cols"); cols->SetText("16"); elem->InsertEndChild(cols);
            auto readOnly = doc->NewElement("ReadOnly"); readOnly->SetText("false"); elem->InsertEndChild(readOnly);
            auto showAscii = doc->NewElement("ShowAscii"); showAscii->SetText("true"); elem->InsertEndChild(showAscii);
            auto showHexII = doc->NewElement("ShowHexII"); showHexII->SetText("false"); elem->InsertEndChild(showHexII);
            auto showOptions = doc->NewElement("ShowOptions"); showOptions->SetText("true"); elem->InsertEndChild(showOptions);
            auto showPreview = doc->NewElement("ShowPreview"); showPreview->SetText("true"); elem->InsertEndChild(showPreview);
            auto greyZero = doc->NewElement("GreyZero"); greyZero->SetText("true"); elem->InsertEndChild(greyZero);
            auto upperHex = doc->NewElement("UpperHex"); upperHex->SetText("true"); elem->InsertEndChild(upperHex);
            auto le = doc->NewElement("LittleEndian"); le->SetText("true"); elem->InsertEndChild(le);
            auto useExternal = doc->NewElement("UseExternal"); useExternal->SetText("false"); elem->InsertEndChild(useExternal);
            auto useCache = doc->NewElement("UseCache"); useCache->SetText("true"); elem->InsertEndChild(useCache);
            return elem;
        }

    // ==================================================================
    // Функции создания простых дочерних элементов (параметры)
    // ==================================================================

    tinyxml2::XMLElement* CreateTextElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Text");
        elem->SetText("New Text");
        return elem;
    }

    tinyxml2::XMLElement* CreateColorElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Color");
        elem->SetText("#FFFFFFFF");
        return elem;
    }

    tinyxml2::XMLElement* CreateWidthElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Width");
        elem->SetText("80");
        return elem;
    }

    tinyxml2::XMLElement* CreateHeightElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Height");
        elem->SetText("30");
        return elem;
    }

    tinyxml2::XMLElement* CreateAddressElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Address");
        elem->SetText("0x00000000");
        return elem;
    }

    tinyxml2::XMLElement* CreateAddrBoolElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("AddrBool");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateMinElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Min");
        elem->SetText("0");
        return elem;
    }

    tinyxml2::XMLElement* CreateMaxElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Max");
        elem->SetText("100");
        return elem;
    }

    tinyxml2::XMLElement* CreateStepElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Step");
        elem->SetText("1");
        return elem;
    }

    tinyxml2::XMLElement* CreateFastElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Fast");
        elem->SetText("10");
        return elem;
    }

    tinyxml2::XMLElement* CreateFormatElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Format");
        elem->SetText("%d");
        return elem;
    }

    tinyxml2::XMLElement* CreateUnitElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Unit");
        elem->SetText("rpm");
        return elem;
    }

    tinyxml2::XMLElement* CreateRadiusElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Radius");
        elem->SetText("100");
        return elem;
    }

    tinyxml2::XMLElement* CreateDigitsElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Digits");
        elem->SetText("4");
        return elem;
    }

    tinyxml2::XMLElement* CreateFileElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("File");
        elem->SetText("image.png");
        return elem;
    }

    tinyxml2::XMLElement* CreateTintElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Tint");
        elem->SetText("#FFFFFFFF");
        return elem;
    }

    tinyxml2::XMLElement* CreateBorderElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Border");
        elem->SetText("#00000000");
        return elem;
    }

    tinyxml2::XMLElement* CreateUV0Element(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("UV0");
        elem->SetText("0.0,0.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateUV1Element(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("UV1");
        elem->SetText("1.0,1.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateLabelElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Label");
        elem->SetText("Label");
        return elem;
    }

    tinyxml2::XMLElement* CreateModeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Mode");
        elem->SetText("0");
        return elem;
    }

    tinyxml2::XMLElement* CreateValueElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Value");
        elem->SetText("1");
        return elem;
    }

    tinyxml2::XMLElement* CreateScaleElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Scale");
        elem->SetText("1.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateScaleXElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ScaleX");
        elem->SetText("1.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateScaleYElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ScaleY");
        elem->SetText("1.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateBarWidthElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BarWidth");
        elem->SetText("0.8");
        return elem;
    }

    tinyxml2::XMLElement* CreateDataTypeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("DataType");
        elem->SetText("7");
        return elem;
    }

    tinyxml2::XMLElement* CreateFreqElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Freq");
        elem->SetText("1");
        return elem;
    }

    tinyxml2::XMLElement* CreateSizeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Size");
        elem->SetText("100");
        return elem;
    }

    tinyxml2::XMLElement* CreateSmoothElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Smooth");
        elem->SetText("0");
        return elem;
    }

    tinyxml2::XMLElement* CreateMaxSizeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("MaxSize");
        elem->SetText("2000");
        return elem;
    }

    tinyxml2::XMLElement* CreateAutoSizeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("AutoSize");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateBufferIdElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BufferId");
        elem->SetText("0");
        return elem;
    }

    tinyxml2::XMLElement* CreateLineColorElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("LineColor");
        elem->SetText("#FFFFFFFF");
        return elem;
    }

    tinyxml2::XMLElement* CreateLineWeightElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("LineWeight");
        elem->SetText("1.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateBandWidthElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BandWidth");
        elem->SetText("20");
        return elem;
    }

    tinyxml2::XMLElement* CreateAddressStepElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("AddressStep");
        elem->SetText("2");
        return elem;
    }

    tinyxml2::XMLElement* CreateOrientationElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Orientation");
        elem->SetText("horizontal");
        return elem;
    }

    tinyxml2::XMLElement* CreateShowPeakElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ShowPeak");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateLoopElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Loop");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateSpeedElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Speed");
        elem->SetText("1.0");
        return elem;
    }

    tinyxml2::XMLElement* CreateBaseAddressElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BaseAddress");
        elem->SetText("0x0000");
        return elem;
    }

    tinyxml2::XMLElement* CreateMemorySizeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Size");
        elem->SetText("256");
        return elem;
    }

    tinyxml2::XMLElement* CreateMemoryColsElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Cols");
        elem->SetText("16");
        return elem;
    }

    tinyxml2::XMLElement* CreateReadOnlyElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ReadOnly");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateShowAsciiElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ShowAscii");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateShowHexIIElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ShowHexII");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateShowOptionsElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ShowOptions");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateShowPreviewElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ShowPreview");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateGreyZeroElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("GreyZero");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateUpperHexElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("UpperHex");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateUseExternalElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("UseExternal");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateUseCacheElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("UseCache");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateDecimalPlacesElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("DecimalPlaces");
        elem->SetText("2");
        return elem;
    }

    tinyxml2::XMLElement* CreatePixelSizeElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("PixelSize");
        elem->SetText("2.5");
        return elem;
    }

    tinyxml2::XMLElement* CreateUseFixedElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("UseFixed");
        elem->SetText("false");
        return elem;
    }

    tinyxml2::XMLElement* CreateLittleEndianElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("LittleEndian");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateHexElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Hex");
        elem->SetText("false");
        return elem;
    }
    tinyxml2::XMLElement* CreateSwitchableElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Switchable");
        elem->SetText("true");
        return elem;
    }

    tinyxml2::XMLElement* CreateDoneAddrElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("DoneAddr");
        elem->SetText("0x00000000");
        return elem;
    }

    tinyxml2::XMLElement* CreateLine1AddressElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Line1Address");
        elem->SetText("0x00000000");
        return elem;
    }

    tinyxml2::XMLElement* CreateLine1TextElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Line1Text");
        elem->SetText("Hello");
        return elem;
    }

    tinyxml2::XMLElement* CreateLine2AddressElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Line2Address");
        elem->SetText("0x00000000");
        return elem;
    }

    tinyxml2::XMLElement* CreateLine2TextElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Line2Text");
        elem->SetText("World");
        return elem;
    }

    tinyxml2::XMLElement* CreateCountElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Count");
        elem->SetText("2");
        return elem;
    }

    tinyxml2::XMLElement* CreateNumBytesElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("NumBytes");
        elem->SetText("256");
        return elem;
    }

    tinyxml2::XMLElement* CreateBytesPerLineElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("BytesPerLine");
        elem->SetText("16");
        return elem;
    }

    tinyxml2::XMLElement* CreateBandsElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Bands");
        elem->SetText("10");
        return elem;
    }

    tinyxml2::XMLElement* CreateColorOnElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ColorOn");
        elem->SetText("#FF0000");
        return elem;
    }

    tinyxml2::XMLElement* CreateColorOffElement(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("ColorOff");
        elem->SetText("#330000");
        return elem;
    }

    // ==================================================================
    // Функции создания вложенных элементов (для контейнеров)
    // ==================================================================

    tinyxml2::XMLElement* CreateComboItem(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Combo");
        auto lbl = doc->NewElement("Label"); lbl->SetText("New Item"); elem->InsertEndChild(lbl);
        auto val = doc->NewElement("Value"); val->SetText("0"); elem->InsertEndChild(val);
        return elem;
    }

    tinyxml2::XMLElement* CreateRadioItem(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Radio");
        auto lbl = doc->NewElement("Label"); lbl->SetText("Option"); elem->InsertEndChild(lbl);
        auto val = doc->NewElement("Value"); val->SetText("0"); elem->InsertEndChild(val);
        return elem;
    }

    tinyxml2::XMLElement* CreateSegmentItem(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Segment");
        auto txt = doc->NewElement("Text"); txt->SetText("Segment"); elem->InsertEndChild(txt);
        auto val = doc->NewElement("Value"); val->SetText("0"); elem->InsertEndChild(val);
        return elem;
    }

    tinyxml2::XMLElement* CreateZoneItem(tinyxml2::XMLDocument* doc) {
        auto elem = doc->NewElement("Zone");
        elem->SetAttribute("start", "0");
        elem->SetAttribute("end", "100");
        elem->SetAttribute("color", "#FF0000");
        return elem;
    }

    tinyxml2::XMLElement* CreateDataLogLine(tinyxml2::XMLDocument* doc) {
        auto line = doc->NewElement("Line");
        auto name = doc->NewElement("Name"); name->SetText("New Line"); line->InsertEndChild(name);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); line->InsertEndChild(addr);
        auto yScale = doc->NewElement("ScaleY"); yScale->SetText("1.0"); line->InsertEndChild(yScale);
        auto color = doc->NewElement("Color"); color->SetText("#FFFFFFFF"); line->InsertEndChild(color);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); line->InsertEndChild(dtype);
        return line;
    }

    tinyxml2::XMLElement* CreateHistogramLine(tinyxml2::XMLDocument* doc) {
        auto line = doc->NewElement("Line");
        auto name = doc->NewElement("Name"); name->SetText("New Line"); line->InsertEndChild(name);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); line->InsertEndChild(addr);
        auto yScale = doc->NewElement("ScaleY"); yScale->SetText("1.0"); line->InsertEndChild(yScale);
        auto color = doc->NewElement("Color"); color->SetText("#FFFFFFFF"); line->InsertEndChild(color);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); line->InsertEndChild(dtype);
        return line;
    }

    tinyxml2::XMLElement* CreatePlotLinesLine(tinyxml2::XMLDocument* doc) {
        auto line = doc->NewElement("Line");
        auto name = doc->NewElement("Name"); name->SetText("New Line"); line->InsertEndChild(name);
        auto addr = doc->NewElement("Address"); addr->SetText("0x00000000"); line->InsertEndChild(addr);
        auto dtype = doc->NewElement("DataType"); dtype->SetText("7"); line->InsertEndChild(dtype);
        auto le = doc->NewElement("LittleEndian"); le->SetText("true"); line->InsertEndChild(le);
        auto scale = doc->NewElement("Scale"); scale->SetText("1.0"); line->InsertEndChild(scale);
        auto bufId = doc->NewElement("BufferId"); bufId->SetText("0"); line->InsertEndChild(bufId);
        auto lineColor = doc->NewElement("LineColor"); lineColor->SetText("#FFFFFFFF"); line->InsertEndChild(lineColor);
        auto weight = doc->NewElement("LineWeight"); weight->SetText("1.0"); line->InsertEndChild(weight);
        return line;
    }

    // ==================================================================
    // Карты создателей
    // ==================================================================

    const std::map<std::string, Creator>& GetRootCreators() {
        static std::map<std::string, Creator> rootCreators = {
            {"Window", CreateNewWindow},
            {"Text", CreateText},
            {"TextColored", CreateTextColored},
            {"Button", CreateButton},
            {"Checkbox", CreateCheckbox},
            {"Group", CreateGroup},
            {"TreeNode", CreateTreeNode},
            {"SameLine", CreateSameLine},
            {"Spacing", CreateSpacing},
            {"Separator", CreateSeparator},
            {"Dummy", CreateDummy},
            {"Grid", CreateGrid},
            {"PlotLines", CreatePlotLines},
            {"TextFormatted", CreateTextFormatted},
            {"TextFormattedF", CreateTextFormattedF},
            {"AlignTextToFramePadding", CreateAlignText},
            {"NewLine", CreateNewLine},
            {"LabelText", CreateLabelText},
            {"RadioButton", CreateRadioButton},
            {"Tooltip", CreateTooltip},
            {"PlotTooltip", CreatePlotTooltip},
            {"ComboBox", CreateComboBox},
            {"ImputInt", CreateInputInt},   
            {"ImputFloat", CreateInputFloat},
            {"FixedWidth", CreateFixedWidth},
            {"SetCursorPos", CreateSetCursorPos},
            {"Indent", CreateIndent},
            {"Histogram", CreateHistogram},
            {"DataLog", CreateDataLog},
            {"ProgressBar", CreateProgressBar},
            {"SliderFloat", CreateSliderFloat},
            {"SliderInt", CreateSliderInt},
            {"TabBar", CreateTabBar},
            {"Tab", CreateTab},
            {"Column", CreateColumn},
            {"SelectableText", CreateSelectableText},
            {"ChildWindow", CreateChildWindow},
            {"Header", CreateHeader},
            {"Popup", CreatePopup},
            {"HintBox", CreateHintBox},
            {"HelpMarker", CreateHelpMarker},
            {"Watch", CreateWatch},
            {"SevenSegment", CreateSevenSegment},
            {"MechanicalSwitch2", CreateMechanicalSwitch2},
            {"Gauge", CreateGauge},
            {"GlossyDisplay", CreateGlossyDisplay},
            {"LCD_HD44780", CreateLCD_HD44780},
            {"VerticalGauge", CreateVerticalGauge},
            {"NeumorphicKnob", CreateNeumorphicKnob},
            {"StyledKnob", CreateStyledKnob},
            {"CustomVerticalSlider", CreateCustomVerticalSlider},
            {"BitControl8Bit", CreateBitControl8Bit},
            {"BitControl", CreateBitControl},
            {"RotarySwitch", CreateRotarySwitch},
            {"LedIndicator", CreateLedIndicator},
            {"Image", CreateImage},
            {"RPMIndicator", CreateRPMIndicator},
            {"Keypad", CreateKeypad},
            {"Equalizer", CreateEqualizer},
            {"Tank", CreateTank},
            {"VUMeter", CreateVUMeter},
            {"SegmentButton", CreateSegmentButton},
            {"MemoryView", CreateMemoryView},
            {"Gif", CreateGif},
            {"GlossyButton", CreateGlossyButton},
            {"MemoryEditor", CreateMemoryEditor},
        };
        return rootCreators;
    }

    const std::map<std::string, ChildCreatorMap>& GetChildCreators() {
        static std::map<std::string, ChildCreatorMap> childMap = {
            // Специализированные контейнеры
            {"ComboBox", {
                {"Combo", CreateComboItem}
            }},
            {"RadioButton", {
                {"Radio", CreateRadioItem}
            }},
            {"SegmentButton", {
                {"Segment", CreateSegmentItem}
            }},
            {"RPMIndicator", {
                {"Zone", CreateZoneItem}
            }},

            // Виджеты, имеющие дочерние элементы-параметры
            {"TextColored", {
                {"Text", CreateTextElement},
                {"Color", CreateColorElement}
            }},
            {"Button", {
                {"Text", CreateTextElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"Address", CreateAddressElement},
                {"Mode", CreateModeElement},
                {"Value", CreateValueElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            {"Checkbox", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"AddrBool", CreateAddrBoolElement}
            }},
            {"SliderInt", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Format", CreateFormatElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            {"SliderFloat", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Format", CreateFormatElement},
                {"Step", CreateStepElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            {"ProgressBar", {
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"Address", CreateAddressElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            {"Gauge", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Radius", CreateRadiusElement},
                {"Unit", CreateUnitElement}
            }},
            {"SevenSegment", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement}
            }},
            {"InputInt", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Step", CreateStepElement},
                {"Fast", CreateFastElement}
            }},
            {"InputFloat", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Step", CreateStepElement},
                {"Fast", CreateFastElement},
                {"Format", CreateFormatElement}
            }},
            {"PlotLines", {
                {"Text", CreateTextElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"BufferId", CreateBufferIdElement},
                {"Freq", CreateFreqElement},
                {"Size", CreateSizeElement},
                {"Smooth", CreateSmoothElement},
                {"MaxSize", CreateMaxSizeElement},
                {"AutoSize", CreateAutoSizeElement},
                {"Line", CreatePlotLinesLine}
            }},
            {"TextFormatted", {
                {"Format", CreateFormatElement},
                {"Address", CreateAddressElement}
            }},
            {"TextFormattedF", {
                {"Format", CreateFormatElement},
                {"Address", CreateAddressElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            { "LabelText", {
                {"Label", CreateLabelElement},
                {"Format", CreateFormatElement},
                {"Address", CreateAddressElement}
            } },
            {"Tooltip", {
                {"Text", CreateTextElement}
            }},
            {"PlotTooltip", {
                {"Text", CreateTextElement},
                {"Format", CreateFormatElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Size", CreateSizeElement},
                {"BufferId", CreateBufferIdElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"Address", CreateAddressElement},
                {"DataType", CreateDataTypeElement},
                {"AutoSize", CreateAutoSizeElement}
            }},
            {"SetCursorPos", {
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement}
            }},
            {"Indent", {
                {"Width", CreateWidthElement}
            }},
            {"Histogram", {
                {"Text", CreateTextElement},
                {"Format", CreateFormatElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"ScaleX", CreateScaleXElement},
                {"Size", CreateSizeElement},
                {"Smooth", CreateSmoothElement},
                {"DoneAddr", CreateDoneAddrElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"AutoSize", CreateAutoSizeElement},
                {"BarWidth", CreateBarWidthElement},
                {"Line", CreateHistogramLine}
            }},
            {"DataLog", {
                {"Text", CreateTextElement},
                {"Format", CreateFormatElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"ScaleX", CreateScaleXElement},
                {"Size", CreateSizeElement},
                {"Smooth", CreateSmoothElement},
                {"DoneAddr", CreateDoneAddrElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"AutoSize", CreateAutoSizeElement},
                {"Line", CreateDataLogLine}
            }},
            {"SelectableText", {
                {"Format", CreateFormatElement},
                {"Address", CreateAddressElement},
                {"AddrBool", CreateAddrBoolElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            {"HelpMarker", {
                {"Text", CreateTextElement}
            }},
            {"MechanicalSwitch2", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement}
            }},
            {"GlossyDisplay", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"Hex", CreateHexElement},
                {"Digits", CreateDigitsElement},
                {"DataType", CreateDataTypeElement},
                {"DecimalPlaces", CreateDecimalPlacesElement}
            }},
            { "LCD_HD44780", {
                {"Text", CreateTextElement},
                {"Line1Address", CreateLine1AddressElement},
                {"Line1Text", CreateLine1TextElement},
                {"Line2Address", CreateLine2AddressElement},
                {"Line2Text", CreateLine2TextElement},
                {"PixelSize", CreatePixelSizeElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"UseFixed", CreateUseFixedElement}
            } },
            {"VerticalGauge", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"DataType", CreateDataTypeElement}
            }},
            {"NeumorphicKnob", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Radius", CreateRadiusElement},
                {"DataType", CreateDataTypeElement}
            }},
            {"StyledKnob", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Radius", CreateRadiusElement},
                {"DataType", CreateDataTypeElement}
            }},
            {"CustomVerticalSlider", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"DataType", CreateDataTypeElement}
            }},
            {"BitControl8Bit", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Scale", CreateScaleElement}
            }},
            {"BitControl", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Mode", CreateModeElement},
                {"Switchable", CreateSwitchableElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            {"RotarySwitch", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Radius", CreateRadiusElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            { "LedIndicator", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Radius", CreateRadiusElement},
                {"ColorOn", CreateColorOnElement},
                {"ColorOff", CreateColorOffElement},
                {"LittleEndian", CreateLittleEndianElement}
            } },
            { "Image", {
                {"Text", CreateTextElement},
                {"File", CreateFileElement},
                {"Scale", CreateScaleElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"Tint", CreateTintElement},
                {"Border", CreateBorderElement},
                {"UV0", CreateUV0Element},
                {"UV1", CreateUV1Element}
            } },
            {"Keypad", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Digits", CreateDigitsElement},
                {"LittleEndian", CreateLittleEndianElement}
            }},
            { "Equalizer", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Bands", CreateBandsElement},        // исправлено: вместо CreateDigitsElement
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"BandWidth", CreateBandWidthElement},
                {"Height", CreateHeightElement},
                {"DataType", CreateDataTypeElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"AddressStep", CreateAddressStepElement}
            } },
            {"Tank", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"Unit", CreateUnitElement},
                {"LiquidColor", CreateColorElement},
                {"MetalColor", CreateColorElement},
                {"DataType", CreateDataTypeElement}
            }},
            {"VUMeter", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"Min", CreateMinElement},
                {"Max", CreateMaxElement},
                {"Orientation", CreateOrientationElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"Unit", CreateUnitElement},
                {"ShowPeak", CreateShowPeakElement}
            }},
            { "MemoryView", {
                {"Text", CreateTextElement},
                {"BaseAddress", CreateBaseAddressElement},
                {"NumBytes", CreateNumBytesElement},
                {"BytesPerLine", CreateBytesPerLineElement},
                {"LittleEndian", CreateLittleEndianElement}
            } },
            {"TreeNode", {
                {"Text", CreateTextElement}
            }},
            {"TabBar", {
                {"Text", CreateTextElement}
            }},
            {"Tab", {
                {"Text", CreateTextElement}
            }},
            {"Column", {
                {"Count", CreateCountElement}
            }},
            {"ChildWindow", {
                {"Text", CreateTextElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement}
            }},
            {"Header", {
                {"Text", CreateTextElement}
            }},
            { "Gif", {
                {"Text", CreateTextElement},
                {"File", CreateFileElement},
                {"Scale", CreateStepElement},
                {"Width", CreateWidthElement},
                {"Height", CreateHeightElement},
                {"Loop", CreateLoopElement},
                {"Speed", CreateSpeedElement}
            } },
            {"GlossyButton", {
                {"Text", CreateTextElement},
                {"Address", CreateAddressElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"Mode", CreateModeElement},
                {"Value", CreateValueElement},
                {"Radius", CreateRadiusElement}
            }},
            { "MemoryEditor", {
                {"Text", CreateTextElement},
                {"BaseAddress", CreateBaseAddressElement},   // <BaseAddress>0x0000</BaseAddress>
                {"Size", CreateMemorySizeElement},           // <Size>256</Size>
                {"Cols", CreateMemoryColsElement},           // <Cols>16</Cols>
                {"ReadOnly", CreateReadOnlyElement},
                {"ShowAscii", CreateShowAsciiElement},
                {"ShowHexII", CreateShowHexIIElement},
                {"ShowOptions", CreateShowOptionsElement},
                {"ShowPreview", CreateShowPreviewElement},
                {"GreyZero", CreateGreyZeroElement},
                {"UpperHex", CreateUpperHexElement},
                {"LittleEndian", CreateLittleEndianElement},
                {"UseExternal", CreateUseExternalElement},
                {"UseCache", CreateUseCacheElement}
            } }
        };
        return childMap;
    }

} // namespace WidgetTemplateFactory