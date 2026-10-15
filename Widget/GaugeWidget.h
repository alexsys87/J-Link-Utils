#pragma once
#include "widget_common.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

class GaugeWidget : public UIWidget {
    std::string label;
    uint32_t address;
    float minVal;
    float maxVal;
    float radius;
    std::string unit;          // unit (e.g. "km/h", "rpm")
    bool littleEndian;

public:
    GaugeWidget(const char* lbl, uint32_t addr, float minV, float maxV, float rad, const char* u, bool le)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV), radius(rad), unit(u ? u : ""), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        // Read the value as a 32-bit float
        float value = (float)ReadTypedValue(ctx, addr, FLOAT, littleEndian);

        ImGui::PushID(addr);
        DrawGauge(label.c_str(), value, minVal, maxVal, radius, unit.c_str());
        ImGui::PopID();
    }

private:
    static float DegToRad(float deg) {
        return deg * ((float)(M_PI) / 180.0f);
    }

    static void DrawGauge(const char* label, float value, float min_val, float max_val, float radius, const char* unit) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size(radius * 2, radius * 2);
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(label))) return;

        ImDrawList* draw = window->DrawList;
        ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);

        // Position of a value on the scale, 0..1
        const float range = (std::isfinite(max_val - min_val) && max_val != min_val) ? max_val - min_val : 1.0f;
        auto scale_t = [&](float v) {
            const float t = (v - min_val) / range;
            return std::isfinite(t) ? ImClamp(t, 0.0f, 1.0f) : 0.0f;
        };

        // Style colors with small adjustments
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 col_text_dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImU32 col_accent = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
        ImU32 col_display_bg = ImGui::GetColorU32(ImGuiCol_WindowBg);

        // Scale parameters
        float start_angle = 150.0f;
        float end_angle = 390.0f;
        float angle_range = end_angle - start_angle;

        // --- Dial background with a radial gradient ---
        // Gradient imitated by several concentric circles
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

        // Beveled outer rim
        draw->AddCircle(center, radius, col_border, 64, 2.0f);
        draw->AddCircle(center, radius * 0.96f, col_border, 64, 1.0f);

        // --- Colored zones (arcs) with a slight glow ---
        auto draw_arc_zone = [&](float val_start, float val_end, ImU32 color, float thickness_scale = 1.0f) {
            float a_start = DegToRad(start_angle + scale_t(val_start) * angle_range);
            float a_end = DegToRad(start_angle + scale_t(val_end) * angle_range);
            // Main arc
            draw->PathArcTo(center, radius * 0.88f, a_start, a_end, 32);
            draw->PathStroke(color, 0, radius * 0.08f * thickness_scale);
            // Lighter outer glow
            // IM_COL32 keeps red in the low byte; the float conversion also clamps the brightened color.
            const ImVec4 base = ImGui::ColorConvertU32ToFloat4(color);
            ImU32 glow = ImGui::ColorConvertFloat4ToU32(ImVec4(base.x * 1.5f, base.y * 1.5f, base.z * 1.5f, 0.3f));
            draw->PathArcTo(center, radius * 0.89f, a_start, a_end, 32);
            draw->PathStroke(glow, 0, radius * 0.02f);
            };

        // Zones at 60% and 75% of the scale
        draw_arc_zone(min_val, min_val + range * 0.60f, IM_COL32(100, 200, 80, 200), 1.2f);   // green, thicker
        draw_arc_zone(min_val + range * 0.60f, min_val + range * 0.75f, IM_COL32(230, 220, 50, 200));
        draw_arc_zone(min_val + range * 0.75f, min_val + range, IM_COL32(230, 70, 70, 200));

        // --- Ticks and numbers with a 3D effect ---
        for (int i = 0; i <= 10; i++) {
            float val = min_val + range * (i / 10.0f);
            float angle = DegToRad(start_angle + (i / 10.0f) * angle_range);
            ImVec2 dir(cosf(angle), sinf(angle));

            // Main (thick) tick
            draw->AddLine(ImVec2(center.x + dir.x * radius * 0.84f, center.y + dir.y * radius * 0.84f),
                ImVec2(center.x + dir.x * radius * 0.74f, center.y + dir.y * radius * 0.74f),
                col_text_dim, 2.0f);
            // Tick shadow
            draw->AddLine(ImVec2(center.x + dir.x * radius * 0.85f + 1, center.y + dir.y * radius * 0.85f + 1),
                ImVec2(center.x + dir.x * radius * 0.75f + 1, center.y + dir.y * radius * 0.75f + 1),
                IM_COL32(0, 0, 0, 60), 2.0f);

            // Numbers
            char buf[16];
            // Decimals only for small ranges (e.g. 0..1)
            snprintf(buf, 16, range >= 10.0f ? "%.0f" : (range >= 1.0f ? "%.1f" : "%.2f"), val);
            ImVec2 text_size = ImGui::CalcTextSize(buf);
            ImVec2 text_pos = ImVec2(center.x + dir.x * radius * 0.62f - text_size.x * 0.5f,
                center.y + dir.y * radius * 0.62f - text_size.y * 0.5f);
            // Text shadow
            draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 100), buf);
            draw->AddText(text_pos, col_text, buf);
        }

        // --- Unit label ---
        ImVec2 unit_size = ImGui::CalcTextSize(unit);
        draw->AddText(ImVec2(center.x - unit_size.x * 0.5f + 1, center.y - radius * 0.4f + 1), IM_COL32(0, 0, 0, 80), unit);
        draw->AddText(ImVec2(center.x - unit_size.x * 0.5f, center.y - radius * 0.4f), col_text_dim, unit);

        // --- Digital display with a shadow ---
        char val_buf[16];
        snprintf(val_buf, 16, "%.0f", value);
        ImVec2 val_size = ImGui::CalcTextSize(val_buf);
        ImVec2 rect_size(radius * 0.45f, radius * 0.22f);
        ImVec2 rect_pos(center.x - rect_size.x * 0.5f, center.y + radius * 0.35f);

        // Display shadow
        draw->AddRectFilled(ImVec2(rect_pos.x + 3, rect_pos.y + 3),
            ImVec2(rect_pos.x + rect_size.x + 3, rect_pos.y + rect_size.y + 3),
            IM_COL32(0, 0, 0, 80), style.FrameRounding);
        // Display background (solid)
        draw->AddRectFilled(rect_pos,
            ImVec2(rect_pos.x + rect_size.x, rect_pos.y + rect_size.y),
            col_display_bg, style.FrameRounding);
        // Outline
        draw->AddRect(rect_pos, ImVec2(rect_pos.x + rect_size.x, rect_pos.y + rect_size.y),
            col_border, style.FrameRounding);
        // Text with a shadow
        draw->AddText(ImVec2(center.x - val_size.x * 0.5f + 1, rect_pos.y + (rect_size.y - val_size.y) * 0.5f + 1),
            IM_COL32(0, 0, 0, 100), val_buf);
        draw->AddText(ImVec2(center.x - val_size.x * 0.5f, rect_pos.y + (rect_size.y - val_size.y) * 0.5f),
            col_text, val_buf);

        // --- Needle with a shadow and a gradient ---
        float arrow_angle = DegToRad(start_angle + scale_t(value) * angle_range);
        ImVec2 arrow_dir(cosf(arrow_angle), sinf(arrow_angle));

        // Needle shadow
        draw->AddLine(ImVec2(center.x + 2, center.y + 2),
            ImVec2(center.x + arrow_dir.x * radius * 0.85f + 2,
                center.y + arrow_dir.y * radius * 0.85f + 2),
            IM_COL32(0, 0, 0, 80), 4.0f);
        // The needle itself (gradient along its length)
        for (int s = 0; s < 3; s++) {
            float alpha = 0.2f + s * 0.3f;
            float off = s * 1.0f;
            draw->AddLine(ImVec2(center.x - arrow_dir.x * off, center.y - arrow_dir.y * off),
                ImVec2(center.x + arrow_dir.x * radius * 0.85f - arrow_dir.x * off,
                    center.y + arrow_dir.y * radius * 0.85f - arrow_dir.y * off),
                ImGui::GetColorU32(WithAlpha(col_accent, alpha)), 4.0f - s);
        }

        // Needle hub with a "rivet"
        draw->AddCircleFilled(center, 7.0f, col_accent, 16);
        draw->AddCircleFilled(center, 5.0f, ImGui::GetColorU32(ImGuiCol_WindowBg), 16);
        draw->AddCircle(center, 7.0f, col_border, 16, 1.5f);
    }
};