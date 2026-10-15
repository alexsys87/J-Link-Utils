// ColumnWidget.h
#pragma once
#include "widget_common.h"

class ColumnWidget : public ContainerWidget {
    int count;
public:
    // ImGui needs at least one column; many columns cost memory every frame.
    ColumnWidget(int cnt) : count(std::clamp(cnt, 1, 64)) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Columns(count, nullptr, true);
        for (size_t i = 0; i < children.size(); ++i) {
            if (ImGui::GetColumnIndex() == 0)
                ImGui::Separator();
            ImGui::PushID((int)i);
            children[i]->Draw(ctx, dynamicAddr);
            ImGui::PopID();
            ImGui::NextColumn();
        }
        ImGui::Separator();
        ImGui::Columns(1);
    }
};