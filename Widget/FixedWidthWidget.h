// FixedWidthWidget.h
#pragma once
#include "widget_common.h"

class FixedWidthWidget : public ContainerWidget {
    float width;
public:
    FixedWidthWidget(float w) : width(w) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::PushItemWidth(width);
        DrawChildren(ctx, dynamicAddr);
        ImGui::PopItemWidth();
    }
};