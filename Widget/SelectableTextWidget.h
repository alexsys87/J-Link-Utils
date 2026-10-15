// SelectableTextWidget.h
#pragma once
#include "widget_common.h"

class SelectableTextWidget : public LeafWidget {
    std::string format;
    uint32_t address;
    bool useBool;
public:
    SelectableTextWidget(const char* fmt, uint32_t addr, bool useBool)
        : format(fmt ? fmt : ""), address(addr), useBool(useBool) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        if (useBool) {
            bool val = DataReadWriteBoolean(ctx, nullptr, nullptr, &addr);
            char buffer[MAX_ITEM_LEN];
            ui_safety::FormatScalar(buffer, sizeof(buffer), format.c_str(), val);
            if (ImGui::Selectable(buffer, &val)) {
                uint8_t v = val;
                DataReadWriteBoolean(ctx, nullptr, &v, &addr);
            }
        }
        else {
            bool val = DataReadWriteU16(ctx, nullptr, nullptr, &addr) != 0;
            char buffer[MAX_ITEM_LEN];
            ui_safety::FormatScalar(buffer, sizeof(buffer), format.c_str(), val);
            if (ImGui::Selectable(buffer, &val)) {
                uint16_t v = val;
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }
        }
    }
};