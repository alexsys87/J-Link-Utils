#pragma once
#include "widget_common.h"
#include <cmath>

// Типы данных для значения
enum TankDataType {
    TANK_FLOAT = 0,
    TANK_U16 = 1,
    TANK_S16 = 2,
    TANK_U32 = 3,
    TANK_S32 = 4,
    // можно добавить другие
};

class TankWidget : public UIWidget {
    std::string label;
    uint32_t address;
    float minVal;
    float maxVal;
    float width;
    float height;
    std::string unit;
    ImU32 liquidColor;
    ImU32 metalColor;
    bool littleEndian;
    int dataType; // используем значения из enum, но int для простоты

public:
    TankWidget(const char* lbl, uint32_t addr, float minV, float maxV,
        float w, float h, const char* u, ImU32 liqCol, ImU32 metCol, bool le, int dtype = 0)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV),
        width(w), height(h), unit(u ? u : ""), liquidColor(liqCol), metalColor(metCol), littleEndian(le), dataType(dtype) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        float value = 0.0f;

        // Чтение в зависимости от типа данных
        if (dataType == TANK_FLOAT) {
            if (littleEndian) {
                uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
                value = *(float*)&raw;
            }
            else {
                value = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
            }
        }
        else if (dataType == TANK_U16) {
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            value = (float)raw;
        }
        else if (dataType == TANK_S16) {
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            value = (float)(int16_t)raw;
        }
        else if (dataType == TANK_U32) {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            value = (float)raw;
        }
        else if (dataType == TANK_S32) {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            value = (float)(int32_t)raw;
        }

        ImGui::PushID(addr);
        DrawTank(label.c_str(), value, minVal, maxVal, width, height, unit.c_str(), liquidColor, metalColor);
        ImGui::PopID();
    }

private:
    static void DrawTank(const char* label, float value, float min_val, float max_val,
        float width, float height, const char* unit,
        ImU32 liquidColor, ImU32 metalColor) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImDrawList* draw = window->DrawList;
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        if (width <= 0) width = 150.0f;
        if (height <= 0) height = 250.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(label))) return;

        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 col_text_dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        float margin = 12.0f;
        float innerWidth = width - 2 * margin;
        float innerHeight = height - 2 * margin;
        float liquidLevel = (value - min_val) / (max_val - min_val);
        liquidLevel = ImClamp(liquidLevel, 0.0f, 1.0f);
        float liquidY = pos.y + height - margin - liquidLevel * innerHeight;

        // --- Металлический корпус с градиентом и бликами ---
        // Разложим metalColor на компоненты
        float r = ((metalColor >> 16) & 0xFF) / 255.0f;
        float g = ((metalColor >> 8) & 0xFF) / 255.0f;
        float b = (metalColor & 0xFF) / 255.0f;
        float a = ((metalColor >> 24) & 0xFF) / 255.0f; // обычно 255

        ImU32 metalLight = IM_COL32((int)(r * 255 * 1.2f), (int)(g * 255 * 1.2f), (int)(b * 255 * 1.2f), (int)(a * 255));
        ImU32 metalDark = IM_COL32((int)(r * 255 * 0.7f), (int)(g * 255 * 0.7f), (int)(b * 255 * 0.7f), (int)(a * 255));

        // Основной корпус (градиент сверху вниз)
        draw->AddRectFilledMultiColor(
            bb.Min, bb.Max,
            metalLight, metalLight, metalDark, metalDark
        );

        // Блик в верхней части
        float highlightHeight = height * 0.15f;
        ImVec2 hlMin(bb.Min.x, bb.Min.y);
        ImVec2 hlMax(bb.Max.x, bb.Min.y + highlightHeight);
        draw->AddRectFilledMultiColor(
            hlMin, hlMax,
            IM_COL32(255, 255, 255, 40), IM_COL32(255, 255, 255, 40),
            IM_COL32(255, 255, 255, 10), IM_COL32(255, 255, 255, 10)
        );

        // Внешняя обводка
        draw->AddRect(bb.Min, bb.Max, col_border, style.FrameRounding, 0, 2.0f);

        // Внутренняя полость (стекло) с градиентом
        ImVec2 innerMin(pos.x + margin, pos.y + margin);
        ImVec2 innerMax(pos.x + width - margin, pos.y + height - margin);
        // Стекло: полупрозрачное с голубоватым оттенком и градиентом от света
        draw->AddRectFilledMultiColor(
            innerMin, innerMax,
            IM_COL32(200, 220, 240, 30), IM_COL32(200, 220, 240, 30),
            IM_COL32(180, 200, 220, 60), IM_COL32(180, 200, 220, 60)
        );
        draw->AddRect(innerMin, innerMax, IM_COL32(100, 100, 120, 100), style.FrameRounding * 0.7f, 0, 1.0f);

        // Жидкость с градиентом и пузырьками (для красоты)
        if (liquidLevel > 0.0f) {
            ImVec2 liqMin(innerMin.x, liquidY);
            ImVec2 liqMax(innerMax.x, innerMax.y);
            // градиент жидкости (светлее сверху)
            ImU32 liqLight = liquidColor; // можно сделать светлее
            ImU32 liqDark = IM_COL32(
                (int)(((liquidColor >> 16) & 0xFF) * 0.7f),
                (int)(((liquidColor >> 8) & 0xFF) * 0.7f),
                (int)((liquidColor & 0xFF) * 0.7f),
                (int)(((liquidColor >> 24) & 0xFF))
            );
            draw->AddRectFilledMultiColor(
                liqMin, liqMax,
                liqLight, liqLight, liqDark, liqDark
            );

            // Пузырьки (несколько полупрозрачных кружков)
            if (liquidLevel > 0.2f) {
                for (int i = 0; i < 5; ++i) {
                    float bubbleX = innerMin.x + (i * innerWidth / 6.0f) + innerWidth / 12.0f;
                    float bubbleY = liquidY + (liquidLevel * innerHeight) * (i % 3) / 4.0f;
                    if (bubbleY < liquidY + 5) bubbleY = liquidY + 5;
                    draw->AddCircleFilled(ImVec2(bubbleX, bubbleY), 4.0f, IM_COL32(255, 255, 255, 120));
                }
            }
        }

        // Блик на поверхности жидкости
        if (liquidLevel > 0.0f && liquidLevel < 1.0f) {
            draw->AddLine(ImVec2(innerMin.x + 4, liquidY),
                ImVec2(innerMax.x - 4, liquidY),
                IM_COL32(255, 255, 255, 180), 2.0f);
        }

        // Шкала сбоку (риски и цифры)
        int numTicks = 5;
        for (int i = 0; i <= numTicks; ++i) {
            float t = static_cast<float>(i) / numTicks;
            float y = innerMax.y - t * innerHeight;
            if (y < innerMin.y) y = innerMin.y;

            draw->AddLine(ImVec2(innerMax.x + 5, y), ImVec2(innerMax.x + 15, y), col_text_dim, 1.5f);
            float val = min_val + t * (max_val - min_val);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", val);
            ImVec2 textSize = ImGui::CalcTextSize(buf);
            draw->AddText(ImVec2(innerMax.x + 20, y - textSize.y * 0.5f), col_text, buf);
        }

        // Цифровое табло снизу с рамкой
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "%.1f %s", value, unit);
        ImVec2 valSize = ImGui::CalcTextSize(valBuf);
        float panelWidth = valSize.x + 20;
        float panelHeight = valSize.y + 8;
        ImVec2 panelPos(pos.x + (width - panelWidth) * 0.5f, pos.y + height + 5);
        // фон табло
        draw->AddRectFilled(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), IM_COL32(0, 0, 0, 180), 4.0f);
        draw->AddRect(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), col_border, 4.0f);
        // текст
        draw->AddText(ImVec2(panelPos.x + 10, panelPos.y + 4), col_text, valBuf);

        // Занимаемое место (включая табло)
        ImGui::Dummy(ImVec2(width, height + panelHeight + 10));
    }
};