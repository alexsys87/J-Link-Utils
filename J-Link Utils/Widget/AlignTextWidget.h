// AlignTextWidget.h
#pragma once
#include "widget_common.h"

class AlignTextWidget : public LeafWidget {
public:
    AlignTextWidget() {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::AlignTextToFramePadding();
    }
};