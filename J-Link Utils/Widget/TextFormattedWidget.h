// TextFormattedWidget.h
#pragma once
#include "widget_common.h"

class TextFormattedWidget : public LeafWidget {
    std::string format;
    uint32_t address;
    ImVec4 color;
    bool hasColor;
public:
    TextFormattedWidget(const char* fmt, uint32_t addr, int hexColor = 0, bool useColor = false)
        : format(fmt ? fmt : ""), address(addr), hasColor(useColor) {
        if (hasColor) {
            ColorRGBA c = ColorConverter(hexColor);
            color = ImVec4(c.R, c.G, c.B, c.A);
        }
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int value = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        char buffer[MAX_ITEM_LEN];
        ui_safety::FormatScalar(buffer, sizeof(buffer), format.c_str(), value);
        if (hasColor)
            ImGui::TextColored(color, "%s", buffer);
        else
            ImGui::Text("%s", buffer);
    }
};