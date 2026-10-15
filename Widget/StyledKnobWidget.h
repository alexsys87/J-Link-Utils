#pragma once
#include "widget_common.h"
#include <cmath>
#include <cfloat>

#define M_PI_F 3.14159265358979323846f

class StyledKnobWidget : public LeafWidget {
    uint32_t address;
    float minVal;
    float maxVal;
    float radiusFactor;
    bool littleEndian;
    int dataType; 

public:
    StyledKnobWidget(const char* lbl, uint32_t addr, float minV, float maxV, float factor, bool le, int dtype = FLOAT)
        : LeafWidget(lbl), address(addr), minVal(minV), maxVal(maxV), radiusFactor(factor), littleEndian(le), dataType(dtype) {
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
            if (!littleEndian) {
                raw = (raw >> 8) | (raw << 8); // swap 16-bit
            }
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
            if (!littleEndian) {
                raw = SwapLeToBe32(raw);
            }
            if (dataType == S32_DEC) {
                int32_t signedVal = (int32_t)raw;
                value = (float)signedVal;
            }
            else {
                value = (float)raw;
            }
        }
        else if (dataType == S08_DEC || dataType == U08_DEC) {
            uint16_t raw = DataReadWriteU08(ctx, nullptr, nullptr, &addr);
            if (dataType == S08_DEC) {
                int8_t signedVal = (int8_t)raw;
                value = (float)signedVal;
            }
            else {
                value = (float)(raw & 0xFF);
            }
        }
        else if (dataType == FLOAT_INV) {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (littleEndian) {
                raw = SwapLeToBe32(raw); // преобразуем little->big, т.к. ожидается big
            }
            value = *(float*)&raw;
        }

        float oldValue = value;

        ImGui::PushID(addr);
        bool held = DrawKnob(text.c_str(), &value, minVal, maxVal, radiusFactor);
        ImGui::PopID();

        if (value != oldValue) {
            uint32_t writeAddr = addr;
            // Запись обратно с учётом типа
            if (dataType == FLOAT) {
                if (littleEndian) {
                    uint32_t raw;
                    memcpy(&raw, &value, sizeof(float));
                    DataReadWriteU32(ctx, nullptr, &raw, &writeAddr);
                }
                else {
                    DataReadWriteFT32(ctx, nullptr, &value, &writeAddr);
                }
            }
            else if (dataType == S16_DEC || dataType == U16_DEC) {
                uint16_t v;
                if (dataType == S16_DEC) {
                    int32_t intVal = (int32_t)value;
                    if (intVal < -32768) intVal = -32768;
                    if (intVal > 32767) intVal = 32767;
                    v = (uint16_t)(int16_t)intVal;
                }
                else {
                    uint32_t intVal = (uint32_t)(value + 0.5f);
                    if (intVal > 65535) intVal = 65535;
                    v = (uint16_t)intVal;
                }
                if (!littleEndian) {
                    v = (v >> 8) | (v << 8);
                }
                DataReadWriteU16(ctx, nullptr, &v, &writeAddr);
            }
            else if (dataType == S32_DEC || dataType == U32_DEC) {
                uint32_t v;
                if (dataType == S32_DEC) {
                    v = (uint32_t)(int32_t)value;
                }
                else {
                    v = (uint32_t)(value + 0.5f);
                }
                if (!littleEndian) {
                    v = SwapLeToBe32(v);
                }
                DataReadWriteU32(ctx, nullptr, &v, &writeAddr);
            }
            else if (dataType == S08_DEC || dataType == U08_DEC) {
                uint8_t v;
                if (dataType == S08_DEC) {
                    int32_t intVal = (int32_t)value;
                    if (intVal < -128) intVal = -128;
                    if (intVal > 127) intVal = 127;
                    v = (uint16_t)(uint8_t)(int8_t)intVal;
                }
                else {
                    uint32_t intVal = (uint32_t)(value + 0.5f);
                    if (intVal > 255) intVal = 255;
                    v = (uint8_t)intVal;
                }
                DataReadWriteU08(ctx, nullptr, &v, &writeAddr);
            }
            else if (dataType == FLOAT_INV) {
                uint32_t v;
                memcpy(&v, &value, sizeof(float));
                if (littleEndian) {
                    v = SwapBeToLe32(v);
                }
                DataReadWriteU32(ctx, nullptr, &v, &writeAddr);
            }
        }
    }

private:
    static bool DrawKnob(const char* label, float* p_value, float v_min, float v_max, float radius_outer) {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        const float ANGLE_MIN = M_PI_F * 0.75f;
        const float ANGLE_MAX = M_PI_F * 2.25f;

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
        ImRect bb(pos, ImVec2(pos.x + radius_outer * 2, pos.y + radius_outer * 2));

        ImGuiID id = window->GetID(label);
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        ImGui::ButtonBehavior(bb, id, &hovered, &held);

        if (held) {
            ImVec2 mouse_rel = ImVec2(io.MousePos.x - center.x, io.MousePos.y - center.y);
            float mouse_angle = atan2f(mouse_rel.y, mouse_rel.x);
            if (mouse_angle < 0) mouse_angle += M_PI_F * 2.0f;

            float current_t = (*p_value - v_min) / (v_max - v_min);
            float current_angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * current_t;
            if (current_angle > M_PI_F * 2.0f) current_angle -= M_PI_F * 2.0f;

            float angle_diff = mouse_angle - current_angle;
            while (angle_diff > M_PI_F) angle_diff -= M_PI_F * 2.0f;
            while (angle_diff < -M_PI_F) angle_diff += M_PI_F * 2.0f;

            if (fabsf(angle_diff) < 1.0f) {
                float new_t = current_t + (angle_diff / (ANGLE_MAX - ANGLE_MIN));
                new_t = ImClamp(new_t, 0.0f, 1.0f);
                *p_value = v_min + new_t * (v_max - v_min);
            }
        }

        ImDrawList* draw_list = window->DrawList;
        float t = (*p_value - v_min) / (v_max - v_min);
        float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * t;

        // Цвета из темы
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_mark = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImU32 col_indicator = ImGui::GetColorU32(ImGuiCol_SliderGrab);

        // Корпус
        draw_list->AddCircleFilled(center, radius_outer, col_bg, 32);
        draw_list->AddCircle(center, radius_outer, col_border, 32, 2.0f);

        // Метки по краю
        for (int i = 0; i <= 20; i++) {
            float s_t = (float)i / 20.0f;
            float s_angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * s_t;
            ImVec2 d(cosf(s_angle), sinf(s_angle));
            draw_list->AddLine(
                ImVec2(center.x + d.x * radius_outer * 1.15f, center.y + d.y * radius_outer * 1.15f),
                ImVec2(center.x + d.x * radius_outer * 1.3f, center.y + d.y * radius_outer * 1.3f),
                col_mark, 1.5f
            );
        }

        // Индикатор (точка)
        ImVec2 indicator = ImVec2(center.x + cosf(angle) * radius_outer * 0.6f,
            center.y + sinf(angle) * radius_outer * 0.6f);
        draw_list->AddCircleFilled(indicator, 5.0f, col_indicator, 12);

        return held;
    }
};