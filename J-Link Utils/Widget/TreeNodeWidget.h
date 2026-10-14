// TreeNodeWidget.h
#pragma once
#include "widget_common.h"

class TreeNodeWidget : public ContainerWidget {
    std::string label;
public:
    TreeNodeWidget(const char* lbl) : label(lbl) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::TreeNode(label.c_str())) {
            for (auto& child : children)
                child->Draw(ctx, dynamicAddr);
            ImGui::TreePop();
        }
    }
};