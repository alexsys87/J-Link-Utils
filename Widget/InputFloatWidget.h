// InputFloatWidget.h
#pragma once
#include "widget_common.h"

class InputFloatWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    float minVal, maxVal, step, fastStep;
    std::string format;
    bool littleEndian;
    bool pending = false;     // edited value waiting to be written
    float pendingValue = 0.0f;
public:
    InputFloatWidget(const char* lbl, uint32_t addr, float minV, float maxV, float st, float fast, const char* fmt, bool le)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), step(st), fastStep(fast), format(ui_safety::ScalarFormatOrDefault(fmt, ui_safety::ScalarKind::Floating)), littleEndian(le) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        float value = (float)ReadTypedValue(ctx, addr, FLOAT, littleEndian);
        char fullLabel[MAX_ITEM_LEN];
        snprintf(fullLabel, sizeof(fullLabel), "%s 0x%08X  ", label.c_str(), (unsigned int)addr);
        // InputScalar does not support EnterReturnsTrue: keep the edit and write it once the
        // field is left (Enter, Tab, click elsewhere) or a +/- button is released.
        ImGui::PushID(this);
        if (ImGui::InputFloat(fullLabel, &value, step, fastStep, format.c_str())) {
            pending = true;
            pendingValue = value;
        }
        const bool active = ImGui::IsItemActive();
        ImGui::PopID();
        if (pending && !active) {
            pending = false;
            value = pendingValue;
            // Min/Max limit the value only when they form a range.
            if (maxVal > minVal) {
                if (value > maxVal) value = maxVal;
                if (value < minVal) value = minVal;
            }
            WriteTypedValue(ctx, addr, FLOAT, littleEndian, value);
        }
    }
};