// DummyWidget.h
#pragma once
#include "widget_common.h"

class DummyWidget : public LeafWidget {
    ImVec2 size;
public:
    DummyWidget(float w, float h) : size(w, h) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGui::Dummy(size);
    }
};