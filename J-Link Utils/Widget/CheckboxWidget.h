// CheckboxWidget.h
#pragma once
#include "widget_common.h"

class CheckboxWidget : public LeafWidget {
    uint32_t address;
    bool useBool;
public:
    CheckboxWidget(const char* txt, uint32_t addr, bool useBool)
        : LeafWidget(txt), address(addr), useBool(useBool) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        if (useBool) {
            bool val = DataReadWriteBoolean(ctx, nullptr, nullptr, &addr);
            if (ImGui::Checkbox(text.c_str(), &val)) {
                uint8_t v = val;
                DataReadWriteBoolean(ctx, nullptr, &v, &addr);
            }
        }
        else {
            bool val = DataReadWriteU16(ctx, nullptr, nullptr, &addr) != 0;
            if (ImGui::Checkbox(text.c_str(), &val)) {
                uint16_t v = val;
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }
        }
    }
};