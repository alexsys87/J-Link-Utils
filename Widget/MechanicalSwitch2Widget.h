#pragma once
#include "widget_common.h"
#include <cstdint>

class MechanicalSwitch2Widget : public LeafWidget {
    uint32_t address;
    ImVec2 size;
    bool littleEndian;

public:
    MechanicalSwitch2Widget(const char* lbl, uint32_t addr, float w, float h, bool le)
        : LeafWidget(lbl), address(addr), size(w, h), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        bool val = DataReadWriteBoolean(ctx, nullptr, nullptr, &addr);

        ImVec2 drawSize = size;
        if (drawSize.x <= 0) drawSize.x = 80;
        if (drawSize.y <= 0) drawSize.y = 130;

        ImGui::PushID(addr);
        bool clicked = DrawMechanicalSwitch(text.c_str(), &val, drawSize);
        ImGui::PopID();

        if (clicked) {
            uint8_t v = val ? 1 : 0;
            DataReadWriteBoolean(ctx, nullptr, &v, &addr);
        }
    }

private:
    static bool DrawMechanicalSwitch(const char* label, bool* v, ImVec2 size) {
        ImGui::InvisibleButton("##mechswitch", size);
        bool clicked = ImGui::IsItemClicked();
        bool active = ImGui::IsItemActive();
        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        if (!disabled && clicked) {
            *v = !(*v);
        }

        ImVec2 pos = ImGui::GetItemRectMin();
        ImVec2 rectMax = ImGui::GetItemRectMax();
        size.x = rectMax.x - pos.x;
        size.y = rectMax.y - pos.y;

        const ImGuiStyle& style = ImGui::GetStyle();
        ImU32 col_button_top, col_button_bottom, col_symbol;

        if (disabled) {
            ImVec4 base = style.Colors[ImGuiCol_Button];
            float gray = (base.x + base.y + base.z) / 3.0f;
            ImVec4 grayed(gray, gray, gray, base.w);
            col_button_top = ImGui::ColorConvertFloat4ToU32(grayed);
            col_button_bottom = ImGui::ColorConvertFloat4ToU32(ImVec4(gray * 0.7f, gray * 0.7f, gray * 0.7f, base.w));
            col_symbol = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        }
        else {
            ImGuiCol main_col = (*v) ? ImGuiCol_SliderGrab : ImGuiCol_Button;
            ImGuiCol active_col = (*v) ? ImGuiCol_SliderGrabActive : ImGuiCol_ButtonActive;

            ImU32 col_main;
            if (active && !(*v)) {
                col_main = ImGui::GetColorU32(active_col);
            }
            else {
                col_main = ImGui::GetColorU32(main_col);
            }

            ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(col_main);
            ImVec4 light = col4;
            light.x *= 1.2f; light.y *= 1.2f; light.z *= 1.2f;
            if (light.x > 1.0f) light.x = 1.0f;
            if (light.y > 1.0f) light.y = 1.0f;
            if (light.z > 1.0f) light.z = 1.0f;
            ImVec4 dark = col4;
            dark.x *= 0.7f; dark.y *= 0.7f; dark.z *= 0.7f;

            col_button_top = ImGui::ColorConvertFloat4ToU32(light);
            col_button_bottom = ImGui::ColorConvertFloat4ToU32(dark);
            col_symbol = ImGui::GetColorU32(ImGuiCol_Text);
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();
        float x0 = pos.x, y0 = pos.y;
        float x1 = pos.x + size.x, y1 = pos.y + size.y;

        // Body
        float frame_rounding = size.x * 0.1f;
        float shadow_offset = 3.0f;
        draw->AddRectFilled(ImVec2(x0 + 2, y0 + 2), ImVec2(x1 + shadow_offset, y1 + shadow_offset),
            IM_COL32(0, 0, 0, 60), frame_rounding);
        draw->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1),
            IM_COL32(70, 70, 70, 255),
            IM_COL32(70, 70, 70, 255),
            IM_COL32(30, 30, 30, 255),
            IM_COL32(30, 30, 30, 255));
        float inset = 2.0f;
        draw->AddRect(ImVec2(x0 + inset, y0 + inset), ImVec2(x1 - inset, y1 - inset),
            IM_COL32(120, 120, 120, 80), frame_rounding * 0.8f, 0, 1.0f);
        draw->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(20, 20, 20, 255), frame_rounding, 0, 1.5f);

        // Inner rocker
        float p = size.x * 0.15f;
        int pressOffset = (active && !disabled) ? 2 : 0; // offset while pressed

        ImVec2 b0(x0 + p + pressOffset, y0 + p + pressOffset);
        ImVec2 b1(x1 - p + pressOffset, y1 - p + pressOffset);

        if (*v) {
            draw->AddRectFilledMultiColor(b0, b1, col_button_top, col_button_top, col_button_bottom, col_button_bottom);
        }
        else {
            draw->AddRectFilledMultiColor(b0, b1, col_button_bottom, col_button_bottom, col_button_top, col_button_top);
        }

        // Shadows and highlights
        draw->AddRectFilled(ImVec2(b0.x - 2, b0.y + 2), ImVec2(b0.x, b1.y - 2), IM_COL32(0, 0, 0, 80));
        draw->AddRectFilled(ImVec2(b1.x, b0.y + 2), ImVec2(b1.x + 2, b1.y - 2), IM_COL32(255, 255, 255, 40));
        draw->AddRectFilled(ImVec2(b0.x + 2, b0.y - 2), ImVec2(b1.x - 2, b0.y), IM_COL32(0, 0, 0, 60));
        draw->AddRectFilled(ImVec2(b0.x + 2, b1.y), ImVec2(b1.x - 2, b1.y + 2), IM_COL32(0, 0, 0, 60));

        draw->AddRectFilled(ImVec2(b0.x + 3, b0.y + 3), ImVec2(b1.x - 3, b0.y + size.y * 0.1f),
            IM_COL32(255, 255, 255, 40));

        // Divider line
        float mid_y = (b0.y + b1.y) * 0.5f;
        draw->AddLine(ImVec2(b0.x + 3, mid_y), ImVec2(b1.x - 3, mid_y), IM_COL32(0, 0, 0, 100), 1.5f);

        // I and O symbols
        float mid_x = (b0.x + b1.x) * 0.5f;
        float sym_thickness = size.x * 0.08f;
        float sym_size = size.y * 0.18f;

        // I
        float i_y_center = b0.y + (b1.y - b0.y) * 0.25f;
        draw->AddLine(ImVec2(mid_x + 1, i_y_center - sym_size * 0.5f + 1),
            ImVec2(mid_x + 1, i_y_center + sym_size * 0.5f + 1),
            IM_COL32(0, 0, 0, 100), sym_thickness);
        draw->AddLine(ImVec2(mid_x, i_y_center - sym_size * 0.5f),
            ImVec2(mid_x, i_y_center + sym_size * 0.5f),
            col_symbol, sym_thickness);

        // O
        float o_y_center = b0.y + (b1.y - b0.y) * 0.75f;
        draw->AddCircle(ImVec2(mid_x + 1, o_y_center + 1), sym_size * 0.4f,
            IM_COL32(0, 0, 0, 100), 0, sym_thickness);
        draw->AddCircle(ImVec2(mid_x, o_y_center), sym_size * 0.4f,
            col_symbol, 0, sym_thickness);

        // Body highlight
        draw->AddLine(ImVec2(x0 + 2, y0 + 2), ImVec2(x1 - 2, y0 + 2), IM_COL32(255, 255, 255, 50), 2.0f);

        return clicked && !disabled;
    }
};