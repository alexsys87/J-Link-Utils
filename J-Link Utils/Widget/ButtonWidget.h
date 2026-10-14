// ButtonWidget.h
#pragma once
#include "widget_common.h"

enum ButtonMode {
    BUTTON_MODE_SETBIT = 0,
    BUTTON_MODE_TOGGLE = 1,
    BUTTON_MODE_WRITE = 2,
    BUTTON_MODE_MOMENTARY = 3,
};

class ButtonWidget : public LeafWidget {
    ImVec2 size;
    uint32_t address;
    ButtonMode mode;
    uint16_t writeValue;
    bool littleEndian;

public:
    ButtonWidget(const char* txt, float w, float h, uint32_t addr, ButtonMode m = BUTTON_MODE_SETBIT, uint16_t val = 1, bool le = true)
        : LeafWidget(txt), size(w, h), address(addr), mode(m), writeValue(val), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        if (mode == BUTTON_MODE_SETBIT || mode == BUTTON_MODE_WRITE) {
            if (ImGui::Button(text.c_str(), size)) {
                uint16_t v = writeValue;
                if (!littleEndian) v = Swap16(v);
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }
        }
        else if (mode == BUTTON_MODE_TOGGLE) {
            uint16_t current = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) current = Swap16(current);
            bool bitSet = (current & 1) != 0;

            if (bitSet) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (ImGui::Button(text.c_str(), size)) {
                uint16_t v = current ^ 1;
                if (!littleEndian) v = Swap16(v);
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }

            ImGui::PopStyleColor(3);
        }
        else if (mode == BUTTON_MODE_MOMENTARY) {
            ImGui::Button(text.c_str(), size);
            if (ImGui::IsItemActive()) {
                uint16_t v = writeValue;
                if (!littleEndian) v = Swap16(v);
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }
            else if (ImGui::IsItemDeactivated()) {
                uint16_t v = 0;
                if (!littleEndian) v = Swap16(v);
                DataReadWriteU16(ctx, nullptr, &v, &addr);
            }
        }
    }
};