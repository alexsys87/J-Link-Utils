// SpacingWidget.h
#pragma once
#include "widget_common.h"

class SpacingWidget : public LeafWidget {
public:
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Spacing();
    }
};