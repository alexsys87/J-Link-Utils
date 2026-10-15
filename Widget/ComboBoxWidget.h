// ComboBoxWidget.h
#pragma once
#include "widget_common.h"

class ComboBoxWidget : public UIWidget {
    std::string label;
    std::vector<std::string> items;
    std::vector<int> values;
    uint32_t address;
public:
    ComboBoxWidget(const char* lbl, uint32_t addr) : label(lbl ? lbl : ""), address(addr) {}
    void AddItem(const char* item, int val) {
        items.push_back(item ? item : "");
        values.push_back(val);
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int current = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        std::vector<const char*> itemPtrs;
        for (const auto& s : items) itemPtrs.push_back(s.c_str());
        if (ImGui::Combo(label.c_str(), &current, itemPtrs.data(), (int)itemPtrs.size())) {
            uint16_t v = values[current];
            DataReadWriteU16(ctx, nullptr, &v, &addr);
        }
    }
};