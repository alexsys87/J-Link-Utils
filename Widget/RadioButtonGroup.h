// RadioButtonGroup.h
#pragma once
#include "widget_common.h"

class RadioButtonGroup : public UIWidget {
    struct RadioItem {
        std::string label;
        int value;
    };
    std::vector<RadioItem> items;
    uint32_t address;
    bool sameLine;
public:
    RadioButtonGroup(uint32_t addr, bool line) : address(addr), sameLine(line) {}
    void AddItem(const char* lbl, int val) {
        items.push_back({ lbl ? lbl : "", val });
    }
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        int current = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        ImGui::PushID(this);
        for (const auto& item : items) {
            if (ImGui::RadioButton(item.label.c_str(), &current, item.value)) {
                uint16_t v = current;
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }
            if (sameLine) ImGui::SameLine();
        }
        ImGui::PopID();
    }
};