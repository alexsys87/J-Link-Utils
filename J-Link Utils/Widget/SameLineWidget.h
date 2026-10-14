// SameLineWidget.h
#pragma once
#include "widget_common.h"

class SameLineWidget : public LeafWidget {
    float offset, spacing;
public:
    SameLineWidget(float off, float sp) : offset(off), spacing(sp) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::SameLine(offset, spacing);
    }
};