#pragma once
#include "widget_common.h"
#include "imgui_internal.h"  // для доступа к ImGuiItemFlags_Disabled (через GetItemFlags)
#include <cmath>
#include <vector>
#include <algorithm>   // для std::max, std::min, std::clamp

#define M_PI_F 3.14159265358979323846f   // суффикс f для float

struct RPMZone {
    float start;
    float end;
    ImU32 color;
    bool operator<(const RPMZone& other) const { return start < other.start; }
};

class RPMIndicatorWidget : public UIWidget {
    std::string label;
    uint32_t address;
    float minVal;
    float maxVal;
    float radius;
    std::string unit;
    std::vector<RPMZone> zones;
    bool littleEndian;

public:
    RPMIndicatorWidget(const char* lbl, uint32_t addr, float minV, float maxV, float rad, const char* u,
        const std::vector<RPMZone>& z, bool le)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), radius(rad), unit(u ? u : ""), zones(z), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        float value;
        if (littleEndian) {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            value = *(float*)&raw;
        }
        else {
            value = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
        }

        ImGui::PushID(addr);
        DrawRPMIndicator(label.c_str(), value, minVal, maxVal, radius, unit.c_str(), zones);
        ImGui::PopID();
    }

private:
    static void DrawRPMIndicator(const char* label, float value, float min_val, float max_val, float radius, const char* unit,
        const std::vector<RPMZone>& zones) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        ImDrawList* draw = window->DrawList;
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);
        ImRect bb(pos, ImVec2(pos.x + radius * 2, pos.y + radius * 2));

        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(label))) return;

        // Проверка, отключён ли виджет (например, через BeginDisabled)
        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 col_text_dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImU32 col_accent = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
        ImU32 col_display_bg = ImGui::GetColorU32(ImGuiCol_WindowBg);

        float start_angle = 150.0f;
        float end_angle = 390.0f;
        float angle_range = end_angle - start_angle;

        // Фон
        for (int i = 0; i < 5; i++) {
            float t = i / 4.0f;
            float r = radius * (0.7f + t * 0.3f);
            ImU32 col = ImGui::GetColorU32(ImVec4(
                style.Colors[ImGuiCol_FrameBg].x * (0.8f + t * 0.4f),
                style.Colors[ImGuiCol_FrameBg].y * (0.8f + t * 0.4f),
                style.Colors[ImGuiCol_FrameBg].z * (0.8f + t * 0.4f),
                style.Colors[ImGuiCol_FrameBg].w
            ));
            draw->AddCircleFilled(center, r, col, 64);
        }

        draw->AddCircle(center, radius, col_border, 64, 2.0f);
        draw->AddCircle(center, radius * 0.96f, col_border, 64, 1.0f);

        // Зоны
        for (const auto& zone : zones) {
            if (zone.end <= min_val || zone.start >= max_val) continue;
            float start_t = (std::max(zone.start, min_val) - min_val) / (max_val - min_val);
            float end_t = (std::min(zone.end, max_val) - min_val) / (max_val - min_val);
            float a_start = DegToRad(start_angle + start_t * angle_range);
            float a_end = DegToRad(start_angle + end_t * angle_range);
            draw->PathArcTo(center, radius * 0.88f, a_start, a_end, 32);
            draw->PathStroke(zone.color, 0, radius * 0.08f);
            // Подсветка
            ImU32 glow = ImGui::ColorConvertFloat4ToU32(ImVec4(
                ((zone.color >> 16) & 0xFF) / 255.0f * 1.5f,
                ((zone.color >> 8) & 0xFF) / 255.0f * 1.5f,
                (zone.color & 0xFF) / 255.0f * 1.5f,
                0.3f
            ));
            draw->PathArcTo(center, radius * 0.89f, a_start, a_end, 32);
            draw->PathStroke(glow, 0, radius * 0.02f);
        }

        // Рисочки и цифры
        int steps = 10;
        for (int i = 0; i <= steps; i++) {
            float val = min_val + i * (max_val - min_val) / steps;
            float t = (float)i / steps;
            float angle = DegToRad(start_angle + t * angle_range);
            ImVec2 dir(cosf(angle), sinf(angle));
            if (!std::isfinite(dir.x) || !std::isfinite(dir.y)) continue;

            ImVec2 p1(center.x + dir.x * radius * 0.84f, center.y + dir.y * radius * 0.84f);
            ImVec2 p2(center.x + dir.x * radius * 0.74f, center.y + dir.y * radius * 0.74f);
            draw->AddLine(p1, p2, col_text_dim, 2.0f);
            draw->AddLine(ImVec2(p1.x + 1, p1.y + 1), ImVec2(p2.x + 1, p2.y + 1), IM_COL32(0, 0, 0, 60), 2.0f);

            char buf[16];
            snprintf(buf, sizeof(buf), "%d", (int)val);
            ImVec2 text_size = ImGui::CalcTextSize(buf);
            ImVec2 text_pos(center.x + dir.x * radius * 0.62f - text_size.x * 0.5f,
                center.y + dir.y * radius * 0.62f - text_size.y * 0.5f);
            draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 100), buf);
            draw->AddText(text_pos, col_text, buf);
        }

        // Единица измерения
        ImVec2 unit_size = ImGui::CalcTextSize(unit);
        draw->AddText(ImVec2(center.x - unit_size.x * 0.5f + 1, center.y - radius * 0.4f + 1), IM_COL32(0, 0, 0, 80), unit);
        draw->AddText(ImVec2(center.x - unit_size.x * 0.5f, center.y - radius * 0.4f), col_text_dim, unit);

        // Цифровое табло
        char val_buf[16];
        snprintf(val_buf, sizeof(val_buf), "%.0f", value);
        ImVec2 val_size = ImGui::CalcTextSize(val_buf);
        float rect_w = radius * 0.45f;
        float rect_h = radius * 0.22f;
        ImVec2 rect_pos(center.x - rect_w * 0.5f, center.y + radius * 0.35f);

        draw->AddRectFilled(ImVec2(rect_pos.x + 3, rect_pos.y + 3), ImVec2(rect_pos.x + rect_w + 3, rect_pos.y + rect_h + 3), IM_COL32(0, 0, 0, 80), style.FrameRounding);
        draw->AddRectFilled(rect_pos, ImVec2(rect_pos.x + rect_w, rect_pos.y + rect_h), col_display_bg, style.FrameRounding);
        draw->AddRect(rect_pos, ImVec2(rect_pos.x + rect_w, rect_pos.y + rect_h), col_border, style.FrameRounding);
        draw->AddText(ImVec2(center.x - val_size.x * 0.5f + 1, rect_pos.y + (rect_h - val_size.y) * 0.5f + 1), IM_COL32(0, 0, 0, 100), val_buf);
        draw->AddText(ImVec2(center.x - val_size.x * 0.5f, rect_pos.y + (rect_h - val_size.y) * 0.5f), col_text, val_buf);

        // Стрелка
        float clamped_val = std::clamp(value, min_val, max_val);
        float arrow_t = (clamped_val - min_val) / (max_val - min_val);
        float arrow_angle = DegToRad(start_angle + arrow_t * angle_range);
        ImVec2 arrow_dir(cosf(arrow_angle), sinf(arrow_angle));
        if (std::isfinite(arrow_dir.x) && std::isfinite(arrow_dir.y)) {
            draw->AddLine(ImVec2(center.x + 2, center.y + 2),
                ImVec2(center.x + arrow_dir.x * radius * 0.85f + 2, center.y + arrow_dir.y * radius * 0.85f + 2),
                IM_COL32(0, 0, 0, 80), 4.0f);
            for (int s = 0; s < 3; s++) {
                float alpha = 0.2f + s * 0.3f;
                float off = s * 1.0f;
                draw->AddLine(ImVec2(center.x - arrow_dir.x * off, center.y - arrow_dir.y * off),
                    ImVec2(center.x + arrow_dir.x * radius * 0.85f - arrow_dir.x * off,
                        center.y + arrow_dir.y * radius * 0.85f - arrow_dir.y * off),
                    ImGui::GetColorU32(ImVec4(((col_accent >> 16) & 0xFF) / 255.0f,
                        ((col_accent >> 8) & 0xFF) / 255.0f,
                        (col_accent & 0xFF) / 255.0f,
                        alpha)), 4.0f - s);
            }
        }
        draw->AddCircleFilled(center, 7.0f, col_accent, 16);
        draw->AddCircleFilled(center, 5.0f, ImGui::GetColorU32(ImGuiCol_WindowBg), 16);
        draw->AddCircle(center, 7.0f, col_border, 16, 1.5f);
    }

    static float DegToRad(float deg) {
        return deg * (M_PI_F / 180.0f);
    }
};