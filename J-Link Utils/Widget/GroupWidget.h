// GroupWidget.h
#pragma once
#include "widget_common.h"

class GroupWidget : public ContainerWidget {
public:
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::BeginGroup();
        for (auto& child : children)
            child->Draw(ctx, dynamicAddr);
        ImGui::EndGroup();
    }
};