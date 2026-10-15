#pragma once
#include "widget_common.h"
#include "imgui_internal.h"
#include <cmath>
#include <cfloat>

#define M_PI_F 3.14159265358979323846f

class RotarySwitchWidget : public LeafWidget {
    uint32_t address;
    int minPos;
    int maxPos;
    float radius;
    bool littleEndian;

public:
    RotarySwitchWidget(const char* lbl, uint32_t addr, int minP, int maxP, float rad, bool le)
        : LeafWidget(lbl), address(addr), minPos(minP), maxPos(maxP), radius(rad), littleEndian(le) {
        // A reversed range would give a negative number of positions.
        if (maxPos < minPos) std::swap(minPos, maxPos);
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        int value = static_cast<int>(raw);
        int oldValue = value;

        ImGui::PushID(addr);
        bool changed = DrawRotarySwitch(text.c_str(), &value, minPos, maxPos, radius);
        ImGui::PopID();

        if (changed && value != oldValue) {
            uint32_t writeAddr = addr;
            uint16_t v = static_cast<uint16_t>(value);
            DataReadWriteU16(ctx, nullptr, &v, &writeAddr);
        }
    }

private:
    static bool DrawRotarySwitch(const char* label, int* p_value, int min_pos, int max_pos, float radius_outer) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        const ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw_list = window->DrawList;

        if (radius_outer <= 0.0f || !std::isfinite(radius_outer))
            radius_outer = 40.0f;

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
        ImRect bb(pos, ImVec2(pos.x + radius_outer * 2.0f, pos.y + radius_outer * 2.0f));

        ImGuiID id = window->GetID(label);
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        ImGui::ButtonBehavior(bb, id, &hovered, &held);

        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
        if (disabled)
            hovered = held = false;

        int oldValue = *p_value;
        const int numPos = max_pos - min_pos + 1;
        const float angle_step = 2.0f * M_PI_F / numPos;
        const float start_angle = -M_PI_F / 2.0f; // start at the top

        if (held && !disabled) {
            ImVec2 mouse_rel = ImVec2(io.MousePos.x - center.x, io.MousePos.y - center.y);
            float dist = sqrtf(mouse_rel.x * mouse_rel.x + mouse_rel.y * mouse_rel.y);
            // Ignore clicks too close to the center to avoid accidental switching
            if (dist > radius_outer * 0.2f) {
                float mouse_angle = atan2f(mouse_rel.y, mouse_rel.x);
                if (mouse_angle < 0) mouse_angle += 2.0f * M_PI_F;

                int best_idx = 0;
                float best_diff = FLT_MAX;
                for (int i = 0; i < numPos; ++i) {
                    float angle = start_angle + i * angle_step;
                    angle = fmodf(angle, 2.0f * M_PI_F);
                    if (angle < 0) angle += 2.0f * M_PI_F;

                    float diff = fabsf(angle - mouse_angle);
                    if (diff > M_PI_F) diff = 2.0f * M_PI_F - diff;
                    if (diff < best_diff) {
                        best_diff = diff;
                        best_idx = i;
                    }
                }
                *p_value = min_pos + best_idx;
            }
        }

        bool changed = (*p_value != oldValue);

        // A single-position switch has no range to divide by.
        float t = (max_pos > min_pos) ? static_cast<float>(*p_value - min_pos) / (max_pos - min_pos) : 0.0f;
        float current_angle = start_angle + t * (numPos - 1) * angle_step;
        current_angle = fmodf(current_angle, 2.0f * M_PI_F);
        if (current_angle < 0) current_angle += 2.0f * M_PI_F;

        ImU32 col_bg = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_marker = ImGui::GetColorU32(ImGuiCol_SliderGrab);
        ImU32 col_marker_active = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);

        draw_list->AddCircleFilled(center, radius_outer * 0.9f, col_bg, 32);
        draw_list->AddCircle(center, radius_outer * 0.9f, col_border, 32, 1.0f);

        for (int i = 0; i < numPos; ++i) {
            float angle = start_angle + i * angle_step;
            angle = fmodf(angle, 2.0f * M_PI_F);
            if (angle < 0) angle += 2.0f * M_PI_F;
            ImVec2 dir(cosf(angle), sinf(angle));
            ImVec2 p1 = ImVec2(center.x + dir.x * radius_outer * 0.75f, center.y + dir.y * radius_outer * 0.75f);
            ImVec2 p2 = ImVec2(center.x + dir.x * radius_outer * 0.85f, center.y + dir.y * radius_outer * 0.85f);
            ImU32 col = (i == (*p_value - min_pos)) ? col_marker_active : col_marker;
            draw_list->AddLine(p1, p2, col, 2.0f);
        }

        ImVec2 indicator = ImVec2(center.x + cosf(current_angle) * radius_outer * 0.5f,
            center.y + sinf(current_angle) * radius_outer * 0.5f);
        draw_list->AddCircleFilled(indicator, radius_outer * 0.12f,
            held ? col_marker_active : col_marker, 12);

        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImVec2 text_pos = ImVec2(center.x - text_size.x * 0.5f, center.y + radius_outer * 1.2f);
        draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), label);

        return changed;
    }
};