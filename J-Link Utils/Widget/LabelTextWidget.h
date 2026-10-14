// LabelTextWidget.h
#pragma once
#include "widget_common.h"

class LabelTextWidget : public LeafWidget {
    std::string label;
    std::string format;
    uint32_t address;
    bool littleEndian;
public:
    LabelTextWidget(const char* lbl, const char* fmt, uint32_t addr, bool le)
        : label(lbl ? lbl : ""), format(fmt ? fmt : ""), address(addr), littleEndian(le) {
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
        snprintf(buffer, sizeof(buffer), format.c_str(), value);
        ImGui::LabelText(label.c_str(), "%s", buffer);
    }
};