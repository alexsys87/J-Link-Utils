// ColumnWidget.h
#pragma once
#include "widget_common.h"

class ColumnWidget : public ContainerWidget {
    int count;
public:
    ColumnWidget(int cnt) : count(cnt) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Columns(count, nullptr, true);
        for (auto& child : children) {
            if (ImGui::GetColumnIndex() == 0)
                ImGui::Separator();
            child->Draw(ctx, dynamicAddr);
            ImGui::NextColumn();
        }
        ImGui::Separator();
        ImGui::Columns(1);
    }
};