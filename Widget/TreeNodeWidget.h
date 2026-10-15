// TreeNodeWidget.h
#pragma once
#include "widget_common.h"

class TreeNodeWidget : public ContainerWidget {
    std::string label;
public:
    TreeNodeWidget(const char* lbl) : label(lbl ? lbl : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::PushID(this);
        if (ImGui::TreeNode(label.empty() ? "##node" : label.c_str())) {
            DrawChildren(ctx, dynamicAddr);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
};