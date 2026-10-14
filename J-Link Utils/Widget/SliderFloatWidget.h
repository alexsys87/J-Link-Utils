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
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), format(fmt ? fmt : "%.3f"), step(st), littleEndian(le) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        union { uint32_t hex; float real; } value;
        if (littleEndian) {
            value.hex = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
        }
        else {
            value.real = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
        }
        if (ImGui::SliderFloat(label.c_str(), &value.real, minVal, maxVal, format.c_str(), 0)) {
            if (littleEndian) {
                uint32_t v = SwapBeToLe32(value.hex);
                DataReadWriteU32(ctx, nullptr, &v, &addr);
            }
            else {
                DataReadWriteFT32(ctx, nullptr, &value.real, &addr);
            }
        }
    }
};