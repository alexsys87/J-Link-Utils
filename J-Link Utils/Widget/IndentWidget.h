// IndentWidget.h
#pragma once
#include "widget_common.h"

class IndentWidget : public LeafWidget {
    float width;
public:
    IndentWidget(float w) : width(w) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Indent(width);
    }
};