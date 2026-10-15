// HelpMarkerWidget.h
#pragma once
#include "widget_common.h"

class HelpMarkerWidget : public LeafWidget {
    std::string text;
public:
    HelpMarkerWidget(const char* txt) : text(txt ? txt : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
};