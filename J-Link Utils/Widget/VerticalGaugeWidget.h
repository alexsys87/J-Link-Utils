#pragma once
#include "widget_common.h"

class VerticalGaugeWidget : public UIWidget {
    std::string label;
    uint32_t address;
    float minVal;
    float maxVal;
    float width;
    float height;
    bool littleEndian;
    int dataType;   // тип данных: FLOAT, S16_DEC, U16_HEX, S32_DEC, U32_HEX и т.д.

public:
    VerticalGaugeWidget(const char* lbl, uint32_t addr, float minV, float maxV, float w, float h, bool le, int dtype = FLOAT)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), width(w), height(h), littleEndian(le), dataType(dtype) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        float value = 0.0f;

        // Чтение данных в зависимости от типа
        if (dataType == FLOAT) {
            if (littleEndian) {
                uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
                value = *(float*)&raw;
            }
            else {
                value = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
            }
        }
        else if (dataType == S16_DEC || dataType == U16_DEC) {
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (dataType == S16_DEC) {
                int16_t signedVal = (int16_t)raw;
                value = (float)signedVal;
            }
            else {
                value = (float)raw;
            }
        }
        else if (dataType == S32_DEC || dataType == U32_DEC) {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (dataType == S32_DEC) {
                int32_t signedVal = (int32_t)raw;
                value = (float)signedVal;
            }
            else {
                value = (float)raw;
            }
        }
        // При необходимости можно добавить другие типы (S08_DEC, U08_HEX, BOOLEAN)

        ImGui::PushID(addr);
        DrawVerticalGauge(label.c_str(), value, minVal, maxVal, width, height);
        ImGui::PopID();
    }

private:
    static void DrawVerticalGauge(const char* /*label*/, float celsius_val, float min_c, float max_c, float width, float height) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImDrawList* draw_list = window->DrawList;
        ImGuiStyle& style = ImGui::GetStyle();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, 0)) return;

        // Цвета из стиля (адаптируются к текущей теме)
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 col_ticks = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        // Фон и рамка
        draw_list->AddRectFilled(bb.Min, bb.Max, col_bg, style.FrameRounding);
        if (style.FrameBorderSize > 0.0f) {
            draw_list->AddRect(bb.Min, bb.Max, col_border, style.FrameRounding);
        }

        float margin_y = 20.0f;
        float bar_x = pos.x + width * 0.5f;
        float bar_top = pos.y + margin_y;
        float bar_bottom = pos.y + height - margin_y;
        float bar_height = bar_bottom - bar_top;

        // Цветные зоны (фиксированные цвета, но можно сделать настраиваемыми)
        struct Zone { ImU32 color; float ratio; };
        Zone zones[] = {
            { IM_COL32(120, 200, 80, 200), 0.25f },
            { IM_COL32(240, 240, 80, 200), 0.25f },
            { IM_COL32(240, 160, 60, 200), 0.25f },
            { IM_COL32(230, 60, 40, 200),  0.25f }
        };

        float current_y = bar_bottom;
        for (int i = 0; i < 4; i++) {
            float zone_h = bar_height * zones[i].ratio;
            draw_list->AddRectFilled(ImVec2(bar_x - 2, current_y - zone_h), ImVec2(bar_x + 2, current_y), zones[i].color);
            current_y -= zone_h;
        }

        // Шкала и подписи
        int steps = 5;
        for (int i = 0; i <= steps; i++) {
            float t = (float)i / (float)steps;
            float y = bar_bottom - t * bar_height;

            float c_val = min_c + t * (max_c - min_c);
            float f_val = c_val * 1.8f + 32.0f;

            // Деления
            draw_list->AddLine(ImVec2(bar_x - 8, y), ImVec2(bar_x + 8, y), col_ticks, 1.0f);

            // Текст (°C и °F)
            char buf_c[16], buf_f[16];
            snprintf(buf_c, 16, "%.0f", c_val);
            snprintf(buf_f, 16, "%.0f", f_val);

            draw_list->AddText(ImVec2(bar_x - 35, y - 7), col_text, buf_c);
            draw_list->AddText(ImVec2(bar_x + 15, y - 7), col_text, buf_f);
        }

        // Указатель (активная линия)
        float t_val = ImSaturate((celsius_val - min_c) / (max_c - min_c));
        float val_y = bar_bottom - t_val * bar_height;
        draw_list->AddLine(ImVec2(bar_x, bar_bottom), ImVec2(bar_x, val_y), ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 4.0f);
    }
};