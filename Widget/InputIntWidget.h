// InputIntWidget.h
#pragma once
#include "widget_common.h"

class InputIntWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    int minVal, maxVal, step, fastStep;
public:
    InputIntWidget(const char* lbl, uint32_t addr, int minV, int maxV, int st, int fast)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), step(st), fastStep(fast) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int value = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        char fullLabel[MAX_ITEM_LEN];
        snprintf(fullLabel, sizeof(fullLabel), "%s %.4d  ", label.c_str(), addr);
        if (ImGui::InputInt(fullLabel, &value, step, fastStep, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (value > maxVal) value = maxVal;
            if (value < minVal) value = minVal;
            uint16_t v = value;
            DataReadWriteU16(ctx, nullptr, &v, &addr);
        }
    }
};