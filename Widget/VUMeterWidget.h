#pragma once
#include "widget_common.h"
#include <cmath>

enum VUOrientation {
    VU_HORIZONTAL,
    VU_VERTICAL
};

class VUMeterWidget : public UIWidget {
    std::string label;
    uint32_t address;
    float minVal;
    float maxVal;
    VUOrientation orientation;
    float width;
    float height;
    std::string unit;
    bool littleEndian;
    bool showPeak;
    float peakHoldTime; // unused, reserved for later

public:
    VUMeterWidget(const char* lbl, uint32_t addr, float minV, float maxV,
        VUOrientation orient, float w, float h, const char* u,
        bool le, bool peak = false)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV),
        orientation(orient), width(w), height(h), unit(u ? u : ""),
        littleEndian(le), showPeak(peak), peakHoldTime(1.0f) {
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
        DrawVUMeter(label.c_str(), value, minVal, maxVal, orientation, width, height, unit.c_str(), showPeak);
        ImGui::PopID();
    }

private:
    static void DrawVUMeter(const char* label, float value, float min_val, float max_val,
        VUOrientation orientation, float width, float height, const char* unit,
        bool showPeak) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImDrawList* draw = window->DrawList;
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        if (width <= 0) width = 200.0f;
        if (height <= 0) height = 30.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(label))) return;

        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 col_text_dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        // Shadow under the meter
        draw->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + width + 2, pos.y + height + 2), IM_COL32(0, 0, 0, 60), style.FrameRounding);

        // Background (black or dark grey)
        draw->AddRectFilled(bb.Min, bb.Max, IM_COL32(30, 30, 30, 255), style.FrameRounding);
        draw->AddRect(bb.Min, bb.Max, col_border, style.FrameRounding, 0, 1.0f);

        float t = (value - min_val) / (max_val - min_val);
        t = ImClamp(t, 0.0f, 1.0f);

        // Gradient colors
        ImU32 col_start = IM_COL32(100, 255, 100, 255); // green
        ImU32 col_mid = IM_COL32(255, 255, 100, 255);  // yellow
        ImU32 col_end = IM_COL32(255, 100, 100, 255);  // red

        // Filled part, colored according to t
        if (t > 0.0f) {
            ImVec2 fillMin, fillMax;
            if (orientation == VU_HORIZONTAL) {
                fillMin = bb.Min;
                fillMax = ImVec2(bb.Min.x + t * width, bb.Max.y);
            }
            else {
                fillMin = ImVec2(bb.Min.x, bb.Max.y - t * height);
                fillMax = bb.Max;
            }

            // Horizontal transition from green to red through yellow
            // For simplicity the fill is one color that depends on t;
            // a gradient along the whole length would be more complex.
            // Smooth transition: for t <= 0.5 interpolate between green and yellow,
            // otherwise between yellow and red.
            ImU32 fillColor;
            if (t <= 0.5f) {
                float local_t = t / 0.5f; // 0..1
                fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    (1.0f - local_t) * 100 / 255.0f + local_t * 255 / 255.0f,
                    (1.0f - local_t) * 255 / 255.0f + local_t * 255 / 255.0f,
                    (1.0f - local_t) * 100 / 255.0f + local_t * 100 / 255.0f,
                    1.0f));
            }
            else {
                float local_t = (t - 0.5f) / 0.5f;
                fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    255 / 255.0f,
                    (1.0f - local_t) * 255 / 255.0f + local_t * 100 / 255.0f,
                    (1.0f - local_t) * 100 / 255.0f + local_t * 100 / 255.0f,
                    1.0f));
            }

            draw->AddRectFilled(fillMin, fillMax, fillColor, style.FrameRounding * 0.5f);
        }

        // Ticks (optional)
        int numTicks = 5;
        for (int i = 0; i <= numTicks; ++i) {
            float tick_t = (float)i / numTicks;
            if (orientation == VU_HORIZONTAL) {
                float x = bb.Min.x + tick_t * width;
                draw->AddLine(ImVec2(x, bb.Min.y), ImVec2(x, bb.Max.y), IM_COL32(80, 80, 80, 100), 1.0f);
            }
            else {
                float y = bb.Min.y + (1.0f - tick_t) * height; // flipped so that zero is at the bottom
                draw->AddLine(ImVec2(bb.Min.x, y), ImVec2(bb.Max.x, y), IM_COL32(80, 80, 80, 100), 1.0f);
            }
        }

        // Peak mark (for now a line at the current value)
        if (showPeak) {
            // The peak could be stored here; for simplicity the line follows the current value
            if (orientation == VU_HORIZONTAL) {
                float x = bb.Min.x + t * width;
                draw->AddLine(ImVec2(x, bb.Min.y - 2), ImVec2(x, bb.Max.y + 2), IM_COL32(255, 255, 255, 200), 2.0f);
            }
            else {
                float y = bb.Max.y - t * height;
                draw->AddLine(ImVec2(bb.Min.x - 2, y), ImVec2(bb.Max.x + 2, y), IM_COL32(255, 255, 255, 200), 2.0f);
            }
        }

        // Text (value and unit) under or next to the meter
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "%.1f %s", value, unit);
        ImVec2 valSize = ImGui::CalcTextSize(valBuf);
        ImVec2 valPos;
        if (orientation == VU_HORIZONTAL) {
            valPos = ImVec2(pos.x + (width - valSize.x) * 0.5f, pos.y + height + 5);
        }
        else {
            valPos = ImVec2(pos.x + width + 10, pos.y + (height - valSize.y) * 0.5f);
        }
        draw->AddText(valPos, col_text, valBuf);

        // Reserved space
        float extra = (orientation == VU_HORIZONTAL) ? valSize.y + 10 : valSize.x + 10;
        if (orientation == VU_HORIZONTAL)
            ImGui::Dummy(ImVec2(width, height + extra));
        else
            ImGui::Dummy(ImVec2(width + extra, height));
    }
};