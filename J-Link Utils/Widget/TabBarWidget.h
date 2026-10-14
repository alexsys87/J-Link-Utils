// TabBarWidget.h
#pragma once
#include "widget_common.h"

class TabBarWidget : public ContainerWidget {
    std::string name;
public:
    TabBarWidget(const char* n) : name(n ? n : "") {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::BeginTabBar(name.c_str(), ImGuiTabBarFlags_TabListPopupButton)) {
            for (auto& child : children) {
                child->Draw(ctx, dynamicAddr);
            }
            ImGui::EndTabBar();
        }
    }
};