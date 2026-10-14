// HintBoxWidget.h
#pragma once
#include "widget_common.h"

class HintBoxWidget : public ContainerWidget {
public:
    HintBoxWidget() {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::IsItemHovered()) {
            for (auto& child : children)
                child->Draw(ctx, dynamicAddr);
        }
    }
};