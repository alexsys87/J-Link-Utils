// FixedWidthWidget.h
#pragma once
#include "widget_common.h"

class FixedWidthWidget : public ContainerWidget {
    float width;
public:
    FixedWidthWidget(float w) : width(w) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::PushItemWidth(width);
        for (auto& child : children)
            child->Draw(ctx, dynamicAddr);
        ImGui::PopItemWidth();
    }
};