// TextWidget.h
#pragma once
#include "widget_common.h"

class TextWidget : public LeafWidget {
public:
    TextWidget(const char* txt) : LeafWidget(txt) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Text("%s", text.c_str());
    }
};