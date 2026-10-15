// PopupWidget.h
#pragma once
#include "widget_common.h"

class PopupWidget : public ContainerWidget {
public:
    PopupWidget() {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        // An explicit ID: the previous item (e.g. plain text) may have none.
        ImGui::PushID(this);
        if (ImGui::BeginPopupContextItem("##popup")) {
            DrawChildren(ctx, dynamicAddr);
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
};