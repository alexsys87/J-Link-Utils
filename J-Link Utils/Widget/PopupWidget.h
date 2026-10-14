// PopupWidget.h
#pragma once
#include "widget_common.h"

class PopupWidget : public ContainerWidget {
public:
    PopupWidget() {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (ImGui::BeginPopupContextItem()) {
            for (auto& child : children)
                child->Draw(ctx, dynamicAddr);
            ImGui::EndPopup();
        }
    }
};