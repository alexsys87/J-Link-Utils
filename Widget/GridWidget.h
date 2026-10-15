// GridWidget.h
#pragma once
#include "widget_common.h"

class GridWidget : public ContainerWidget {
    int rows, cols;
    uint32_t baseAddr;
    int addrInc;
public:
    // More than 256 x 256 cells would stall every frame.
    GridWidget(int r, int c, uint32_t base, int inc)
        : rows(std::clamp(r, 0, 256)), cols(std::clamp(c, 0, 256)), baseAddr(base), addrInc(inc) {}
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t currentAddr = baseAddr;
        for (int i = 0; i < rows * cols; ++i) {
            ImGui::PushID(i);
            for (size_t c = 0; c < children.size(); ++c) {
                uint32_t childAddr = currentAddr;
                ImGui::PushID((int)c);
                children[c]->Draw(ctx, &childAddr);
                ImGui::PopID();
            }
            if ((i % cols) < (cols - 1)) ImGui::SameLine();
            ImGui::PopID();
            currentAddr += (uint32_t)addrInc;   // wraps instead of signed overflow
        }
    }
};