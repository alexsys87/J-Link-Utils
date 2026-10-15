// SliderIntWidget.h
#pragma once
#include "widget_common.h"

class SliderIntWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    int minVal, maxVal;
    std::string format;
public:
    SliderIntWidget(const char* lbl, uint32_t addr, int minV, int maxV, const char* fmt)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), format(ui_safety::ScalarFormatOrDefault(fmt, ui_safety::ScalarKind::Integer)) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int value = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        static std::unordered_map<uint16_t, bool> dragging;
        static std::unordered_map<uint16_t, int> lastValue;
        if (ImGui::SliderInt(label.c_str(), &value, minVal, maxVal, format.c_str())) {
            dragging[addr] = true;
            lastValue[addr] = value;
        }
        ImGuiIO& io = ImGui::GetIO();
        if (!io.MouseDown[0] && dragging[addr]) {
            uint16_t v = lastValue[addr];
            DataReadWriteU16(ctx, nullptr, &v, &addr);
            dragging[addr] = false;
        }
    }
};