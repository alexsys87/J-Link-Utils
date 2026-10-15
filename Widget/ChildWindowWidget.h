// ChildWindowWidget.h
#pragma once
#include "widget_common.h"

class ChildWindowWidget : public ContainerWidget {
    std::string name;
    ImVec2 size;
public:
    ChildWindowWidget(const char* n, float w, float h) : name(n ? n : ""), size(w, h) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        // AlwaysAutoResize is not allowed for child windows (ImGui asserts).
        ImGui::PushID(this);
        ImGui::BeginChild(name.empty() ? "##child" : name.c_str(), size, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
        DrawChildren(ctx, dynamicAddr);
        ImGui::EndChild();
        ImGui::PopID();
    }
};