#pragma once
#include "widget_common.h"
#include "imgui_internal.h"
#include <cstdint>
#include <cstdio>

class BitControl8BitWidget : public LeafWidget {
    uint32_t address;
    bool littleEndian; // задел на будущее (для 16/32 бит)
    float scaleFactor = 1.0f;   // коэффициент масштабирования

public:
    BitControl8BitWidget(const char* lbl, uint32_t addr, bool le, float scale = 1.0f)
        : LeafWidget(lbl), address(addr), littleEndian(le), scaleFactor(scale) {
    }

    // Изменение масштаба виджета
    void SetScale(float scale) {
        scaleFactor = scale;
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        // Чтение 8-битного значения
        uint16_t raw = DataReadWriteU08(ctx, nullptr, nullptr, &addr);
        uint8_t value = static_cast<uint8_t>(raw);
        uint8_t oldValue = value;

        ImGui::PushID(addr);
        DrawBitControl(text.c_str(), &value);
        ImGui::PopID();

        if (value != oldValue) {
            uint32_t writeAddr = addr;
            uint8_t v = value;
            DataReadWriteU08(ctx, nullptr, &v, &writeAddr);
        }
    }

private:
    void DrawBitControl(const char* label, uint8_t* value) {
        ImGui::PushID(label);
        ImGui::BeginGroup();

        const ImGuiStyle& style = ImGui::GetStyle();
        // Базовые размеры (при scale = 1.0)
        const float base_cell_size = 35.0f;
        const float base_text_offset_y = 4.0f;
        const float base_panel_height = 40.0f;
        const float shadow_offset = 3.0f; // тень (не масштабируем)
        const float rounding = 8.0f;       // скругление для тени
        const float cell_rounding = 4.0f;  // скругление ячеек

        // Масштабированные размеры
        float cell_size = base_cell_size * scaleFactor;
        float spacing = style.ItemSpacing.x * scaleFactor;
        float extra_spacing = spacing * 2.0f; // удвоенный отступ между 4 и 3 битами
        float text_offset_y = base_text_offset_y * scaleFactor;
        float panel_height = base_panel_height * scaleFactor;

        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        float total_width = 8 * cell_size + 7 * spacing + extra_spacing;

        ImVec2 area_start = ImGui::GetCursorScreenPos();

        // Рисуем тень под всей областью битов
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(
            ImVec2(area_start.x + shadow_offset, area_start.y + shadow_offset),
            ImVec2(area_start.x + total_width + shadow_offset, area_start.y + cell_size + 50 + text_offset_y + ImGui::GetTextLineHeight() * 2 + 20 * scaleFactor),
            IM_COL32(0, 0, 0, 50), rounding
        );

        ImGui::InvisibleButton("##bits_area", ImVec2(total_width, cell_size));
        bool area_clicked = ImGui::IsItemClicked();

        int clicked_bit = -1;
        if (area_clicked && !disabled) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float rel_x = mouse_pos.x - area_start.x;
            float cur_x = 0;
            for (int i = 7; i >= 0; --i) {
                if (rel_x >= cur_x && rel_x < cur_x + cell_size) {
                    clicked_bit = i;
                    break;
                }
                cur_x += cell_size;
                if (i > 0) cur_x += (i == 4) ? extra_spacing : spacing;
            }
        }
        if (clicked_bit != -1) {
            *value ^= (1 << clicked_bit);
        }

        // Отрисовка ячеек битов
        float draw_x = area_start.x;
        for (int i = 7; i >= 0; --i) {
            bool is_set = (*value >> i) & 1;

            ImVec2 rect_min(draw_x, area_start.y);
            ImVec2 rect_max(draw_x + cell_size, area_start.y + cell_size);

            ImU32 bg_color;
            if (disabled) {
                bg_color = IM_COL32(80, 80, 80, 255);
            }
            else if (is_set) {
                bg_color = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
            }
            else {
                bg_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
            }

            draw->AddRectFilled(rect_min, rect_max, bg_color, cell_rounding);
            draw->AddRect(rect_min, rect_max, ImGui::GetColorU32(ImGuiCol_Border), cell_rounding, 0, 1.5f);

            char text[2] = { is_set ? '1' : '0', '\0' };
            ImVec2 text_size = ImGui::CalcTextSize(text);
            float text_x = draw_x + (cell_size - text_size.x) * 0.5f;
            float text_y = area_start.y + (cell_size - text_size.y) * 0.5f;
            draw->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), text);

            draw_x += cell_size;
            if (i > 0) draw_x += (i == 4) ? extra_spacing : spacing;
        }

        // Подписи индексов
        ImGui::SetCursorScreenPos(ImVec2(area_start.x, area_start.y + cell_size + text_offset_y));
        draw_x = area_start.x;
        for (int i = 7; i >= 0; --i) {
            std::string idx = std::to_string(i);
            ImVec2 text_size = ImGui::CalcTextSize(idx.c_str());
            float text_x = draw_x + (cell_size - text_size.x) * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(text_x, area_start.y + cell_size + text_offset_y));
            ImGui::TextDisabled("%s", idx.c_str());

            draw_x += cell_size;
            if (i > 0) draw_x += (i == 4) ? extra_spacing : spacing;
        }

        // HEX-панели
        ImGui::SetCursorScreenPos(ImVec2(area_start.x, area_start.y + cell_size + text_offset_y + ImGui::GetTextLineHeight() + 8 * scaleFactor));
        ImVec2 panel_size = ImVec2((total_width - spacing) * 0.5f, panel_height);

        ImGui::BeginChild(ImGui::GetID("hex_frame1"), panel_size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPosY((panel_size.y - ImGui::GetTextLineHeight()) * 0.5f);
        char hex_buf1[16];
        snprintf(hex_buf1, sizeof(hex_buf1), "0x%02X", *value);
        ImGui::SetCursorPosX((panel_size.x - ImGui::CalcTextSize(hex_buf1).x) * 0.5f);
        ImGui::Text("%s", hex_buf1);
        ImGui::EndChild();

        ImGui::SameLine(0, spacing);

        ImGui::BeginChild(ImGui::GetID("hex_frame2"), panel_size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPosY((panel_size.y - ImGui::GetTextLineHeight()) * 0.5f);
        char hex_buf2[16];
        snprintf(hex_buf2, sizeof(hex_buf2), "%02X h", *value);
        ImGui::SetCursorPosX((panel_size.x - ImGui::CalcTextSize(hex_buf2).x) * 0.5f);
        ImGui::Text("%s", hex_buf2);
        ImGui::EndChild();

        ImGui::EndGroup();
        ImGui::PopID();
    }
};