// ChildWindowWidget.h
#pragma once
#include "widget_common.h"

class ChildWindowWidget : public ContainerWidget {
    std::string name;
    ImVec2 size;
public:
    ChildWindowWidget(const char* n, float w, float h) : name(n ? n : ""), size(w, h) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::BeginChild(name.c_str(), size, true, ImGuiWindowFlags_AlwaysAutoResize);
        for (auto& child : children)
            child->Draw(ctx, dynamicAddr);
        ImGui::EndChild();
    }
};