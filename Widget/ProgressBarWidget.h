// ProgressBarWidget.h
#pragma once
#include "widget_common.h"

class ProgressBarWidget : public LeafWidget {
    ImVec2 size;
    uint32_t address;
    bool littleEndian;
public:
    ProgressBarWidget(float w, float h, uint32_t addr, bool le)
        : size(w, h), address(addr), littleEndian(le) {
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        float value;
        if (littleEndian) {
            uint32_t tmp = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            value = *(float*)&tmp;
        }
        else {
            value = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
        }
        ImGui::ProgressBar(value, size);
    }
};