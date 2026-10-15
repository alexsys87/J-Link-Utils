// SliderIntWidget.h
#pragma once
#include "widget_common.h"

class SliderIntWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    int minVal, maxVal;
    std::string format;
    bool dragging = false;   // the value is written when the mouse is released
    int lastValue = 0;
public:
    SliderIntWidget(const char* lbl, uint32_t addr, int minV, int maxV, const char* fmt)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), format(ui_safety::ScalarFormatOrDefault(fmt, ui_safety::ScalarKind::Integer)) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int value = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        if (dragging) value = lastValue;
        ImGui::PushID(this);
        if (ImGui::SliderInt(label.c_str(), &value, minVal, maxVal, format.c_str())) {
            dragging = true;
            lastValue = value;
        }
        ImGui::PopID();
        ImGuiIO& io = ImGui::GetIO();
        if (!io.MouseDown[0] && dragging) {
            uint16_t v = (uint16_t)lastValue;
            DataReadWriteU16(ctx, nullptr, &v, &addr);
            dragging = false;
        }
    }
};