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
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), step(st), fastStep(fast), format(fmt ? fmt : "%.3f"), littleEndian(le) {
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
        char fullLabel[MAX_ITEM_LEN];
        snprintf(fullLabel, sizeof(fullLabel), "%s %.4d  ", label.c_str(), addr);
        if (ImGui::InputFloat(fullLabel, &value.real, step, fastStep, format.c_str(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (value.real > maxVal) value.real = maxVal;
            if (value.real < minVal) value.real = minVal;
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