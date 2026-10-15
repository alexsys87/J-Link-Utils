// TabItemWidget.h
#pragma once
#include "widget_common.h"

class TabItemWidget : public ContainerWidget {
    std::string label;
public:
    TabItemWidget(const char* lbl) : label(lbl ? lbl : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        // Outside a TabBar there is no tab to open: show the content directly.
        if (!ImGui::GetCurrentContext()->CurrentTabBar) {
            DrawChildren(ctx, dynamicAddr);
            return;
        }
        if (ImGui::BeginTabItem(label.c_str())) {
            DrawChildren(ctx, dynamicAddr);
            ImGui::EndTabItem();
        }
    }
};