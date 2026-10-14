// GridWidget.h
#pragma once
#include "widget_common.h"

class GridWidget : public ContainerWidget {
    int rows, cols;
    uint32_t baseAddr;
    int addrInc;
public:
    GridWidget(int r, int c, int base, int inc) : rows(r), cols(c), baseAddr(base), addrInc(inc) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        int currentAddr = baseAddr;
        for (int i = 0; i < rows * cols; ++i) {
            ImGui::PushID(i);
            for (auto& child : children) {
                uint32_t childAddr = currentAddr;
                child->Draw(ctx, &childAddr);
            }
            if ((i % cols) < (cols - 1)) ImGui::SameLine();
            ImGui::PopID();
            currentAddr += addrInc;
        }
    }
};