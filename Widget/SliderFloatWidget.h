// SliderFloatWidget.h
#pragma once
#include "widget_common.h"

class SliderFloatWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    float minVal, maxVal;
    std::string format;
    float step;
    bool littleEndian;
public:
    SliderFloatWidget(const char* lbl, uint32_t addr, float minV, float maxV, const char* fmt, float st, bool le)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), format(ui_safety::ScalarFormatOrDefault(fmt, ui_safety::ScalarKind::Floating)), step(st), littleEndian(le) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        float value = (float)ReadTypedValue(ctx, addr, FLOAT, littleEndian);
        if (ImGui::SliderFloat(label.c_str(), &value, minVal, maxVal, format.c_str(), 0))
            WriteTypedValue(ctx, addr, FLOAT, littleEndian, value);
    }
};