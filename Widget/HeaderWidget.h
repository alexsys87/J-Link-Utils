// HeaderWidget.h
#pragma once
#include "widget_common.h"

class HeaderWidget : public ContainerWidget {
    std::string label;
public:
    HeaderWidget(const char* lbl) : label(lbl ? lbl : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::CollapsingHeader(label.c_str())) {
            DrawChildren(ctx, dynamicAddr);
        }
    }
};