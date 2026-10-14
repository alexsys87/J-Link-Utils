// TooltipWidget.h
#pragma once
#include "widget_common.h"

class TooltipWidget : public LeafWidget {
    std::string text;
public:
    TooltipWidget(const char* txt) : text(txt ? txt : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", text.c_str());
        }
    }
};