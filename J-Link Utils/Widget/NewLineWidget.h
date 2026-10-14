// NewLineWidget.h
#pragma once
#include "widget_common.h"

class NewLineWidget : public LeafWidget {
public:
    NewLineWidget() {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::NewLine();
    }
};