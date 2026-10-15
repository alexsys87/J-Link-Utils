// GroupWidget.h
#pragma once
#include "widget_common.h"

class GroupWidget : public ContainerWidget {
public:
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::BeginGroup();
        DrawChildren(ctx, dynamicAddr);
        ImGui::EndGroup();
    }
};