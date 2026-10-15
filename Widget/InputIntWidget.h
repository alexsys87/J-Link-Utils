// InputIntWidget.h
#pragma once
#include "widget_common.h"

class InputIntWidget : public LeafWidget {
    std::string label;
    uint32_t address;
    int minVal, maxVal, step, fastStep;
    bool pending = false;     // edited value waiting to be written
    int pendingValue = 0;
public:
    InputIntWidget(const char* lbl, uint32_t addr, int minV, int maxV, int st, int fast)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), step(st), fastStep(fast) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int value = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        char fullLabel[MAX_ITEM_LEN];
        snprintf(fullLabel, sizeof(fullLabel), "%s 0x%08X  ", label.c_str(), (unsigned int)addr);
        // InputScalar does not support EnterReturnsTrue: keep the edit and write it once the
        // field is left (Enter, Tab, click elsewhere) or a +/- button is released.
        ImGui::PushID(this);
        if (ImGui::InputInt(fullLabel, &value, step, fastStep)) {
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
            uint16_t v = (uint16_t)value;
            DataReadWriteU16(ctx, nullptr, &v, &addr);
        }
    }
};