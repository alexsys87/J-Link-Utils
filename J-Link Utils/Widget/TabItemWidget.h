// TabItemWidget.h
#pragma once
#include "widget_common.h"

class TabItemWidget : public ContainerWidget {
    std::string label;
public:
    TabItemWidget(const char* lbl) : label(lbl ? lbl : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::BeginTabItem(label.c_str())) {
            for (auto& child : children)
                child->Draw(ctx, dynamicAddr);
            ImGui::EndTabItem();
        }
    }
};