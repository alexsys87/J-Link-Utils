// TextColoredWidget.h
#pragma once
#include "widget_common.h"

class TextColoredWidget : public LeafWidget {
    ImVec4 color;
public:
    TextColoredWidget(const char* txt, int hexColor) : LeafWidget(txt) {
        ColorRGBA c = ColorConverter(hexColor);
        color = ImVec4(c.R, c.G, c.B, c.A);
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::TextColored(color, "%s", text.c_str());
    }
};