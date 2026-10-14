// TextFormattedFWidget.h
#pragma once
#include "widget_common.h"

class TextFormattedFWidget : public LeafWidget {
    std::string format;
    uint32_t address;
    ImVec4 color;
    bool hasColor;
    bool littleEndian;
public:
    TextFormattedFWidget(const char* fmt, uint32_t addr, bool le, int hexColor = 0, bool useColor = false)
        : format(fmt ? fmt : ""), address(addr), littleEndian(le), hasColor(useColor) {
        if (hasColor) {
            ColorRGBA c = ColorConverter(hexColor);
            color = ImVec4(c.R, c.G, c.B, c.A);
        }
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        float value;
        if (littleEndian) {
            uint32_t tmp = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            value = *(float*)&tmp;
        }
        else {
            value = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
        }
        char buffer[MAX_ITEM_LEN];
        _snprintf_s(buffer, sizeof(buffer), format.c_str(), value);
        if (hasColor)
            ImGui::TextColored(color, "%s", buffer);
        else
            ImGui::Text("%s", buffer);
    }
};