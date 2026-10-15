// InputFloatWidget.h
#pragma once
#include "widget_common.h"

class InputFloatWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    float minVal, maxVal, step, fastStep;
    std::string format;
    bool littleEndian;
public:
    InputFloatWidget(const char* lbl, uint32_t addr, float minV, float maxV, float st, float fast, const char* fmt, bool le)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), step(st), fastStep(fast), format(ui_safety::ScalarFormatOrDefault(fmt, ui_safety::ScalarKind::Floating)), littleEndian(le) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        float value = (float)ReadTypedValue(ctx, addr, FLOAT, littleEndian);
        char fullLabel[MAX_ITEM_LEN];
        snprintf(fullLabel, sizeof(fullLabel), "%s %.4d  ", label.c_str(), addr);
        if (ImGui::InputFloat(fullLabel, &value, step, fastStep, format.c_str(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (value > maxVal) value = maxVal;
            if (value < minVal) value = minVal;
            WriteTypedValue(ctx, addr, FLOAT, littleEndian, value);
        }
    }
};