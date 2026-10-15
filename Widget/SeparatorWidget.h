// SeparatorWidget.h
#pragma once
#include "widget_common.h"

class SeparatorWidget : public LeafWidget {
public:
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Separator();
    }
};