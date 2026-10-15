#pragma once
#include "widget_common.h"
#include <cmath>

#define M_PI_F 3.14159265358979323846f

class NeumorphicKnobWidget : public LeafWidget {
    uint32_t address;
    float minVal;
    float maxVal;
    float radius;
    bool littleEndian;
    int dataType; // data type: FLOAT, S16_DEC, U16_DEC, S32_DEC, U32_DEC, ...

public:
    NeumorphicKnobWidget(const char* lbl, uint32_t addr, float minV, float maxV, float rad, bool le, int dtype = FLOAT)
        : LeafWidget(lbl), address(addr), minVal(minV), maxVal(maxV), radius(rad), littleEndian(le), dataType(dtype) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        float value = (float)ReadTypedValue(ctx, addr, dataType, littleEndian);

        float oldValue = value;

        ImGui::PushID(addr);
        bool held = DrawKnob(text.c_str(), &value, minVal, maxVal, radius);
        ImGui::PopID();

        // NaN in memory compares unequal to itself: do not write it back every frame.
        if (value != oldValue && !(std::isnan(value) && std::isnan(oldValue)))
            WriteTypedValue(ctx, addr, dataType, littleEndian, value);
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

        // Knob position 0..1; a value outside the range (or NaN) sits at an end stop.
        const float range = v_max - v_min;
        const bool hasRange = std::isfinite(range) && range != 0.0f;
        float current_t = hasRange ? (*p_value - v_min) / range : 0.0f;
        current_t = std::isfinite(current_t) ? ImClamp(current_t, 0.0f, 1.0f) : 0.0f;

        if (held && hasRange) {
            ImVec2 mouse_rel = ImVec2(io.MousePos.x - center.x, io.MousePos.y - center.y);
            float mouse_angle = atan2f(mouse_rel.y, mouse_rel.x);
            if (mouse_angle < 0) mouse_angle += M_PI_F * 2.0f;

            float current_angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * current_t;
            if (current_angle > M_PI_F * 2.0f) current_angle -= M_PI_F * 2.0f;

            // Shortest turn, -pi..pi
            float angle_diff = remainderf(mouse_angle - current_angle, M_PI_F * 2.0f);

            if (fabsf(angle_diff) < 1.0f) {
                float new_t = current_t + (angle_diff / (ANGLE_MAX - ANGLE_MIN));
                new_t = ImClamp(new_t, 0.0f, 1.0f);
                *p_value = v_min + new_t * (v_max - v_min);
            }
        }

        ImDrawList* draw_list = window->DrawList;
        float t = hasRange ? (*p_value - v_min) / range : 0.0f;
        t = std::isfinite(t) ? ImClamp(t, 0.0f, 1.0f) : 0.0f;
        float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * t;

        draw_list->AddCircleFilled(center, radius_outer, IM_COL32(235, 235, 235, 255), 32);
        draw_list->AddCircle(center, radius_outer, IM_COL32(200, 200, 200, 255), 32, 2.0f);

        for (int i = 0; i <= 20; i++) {
            float s_t = (float)i / 20.0f;
            float s_angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * s_t;
            ImVec2 d(cosf(s_angle), sinf(s_angle));
            draw_list->AddLine(
                ImVec2(center.x + d.x * radius_outer * 1.15f, center.y + d.y * radius_outer * 1.15f),
                ImVec2(center.x + d.x * radius_outer * 1.3f, center.y + d.y * radius_outer * 1.3f),
                IM_COL32(180, 180, 180, 255), 1.5f
            );
        }

        ImVec2 indicator = ImVec2(center.x + cosf(angle) * radius_outer * 0.6f,
            center.y + sinf(angle) * radius_outer * 0.6f);
        draw_list->AddCircleFilled(indicator, 5.0f, IM_COL32(170, 170, 170, 255), 12);

        return held;
    }
};