// SetCursorPosWidget.h
#pragma once
#include "widget_common.h"

class SetCursorPosWidget : public LeafWidget {
    ImVec2 pos;
public:
    SetCursorPosWidget(float x, float y) : pos(x, y) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::SetCursorPos(pos);
        // Submit an item there, as ImGui requires after moving the cursor.
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }
};