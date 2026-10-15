#pragma once
#include "widget_common.h"
#include "imgui_internal.h"  // ImGuiItemFlags_Disabled
#include <cmath>

class LedIndicatorWidget : public LeafWidget {
    uint32_t address;
    float radius;
    ImU32 colorOn;
    ImU32 colorOff;
    bool littleEndian;   // for reading the value

public:
    LedIndicatorWidget(const char* lbl, uint32_t addr, float rad, ImU32 onCol, ImU32 offCol, bool le)
        : LeafWidget(lbl), address(addr), radius(rad), colorOn(onCol), colorOff(offCol), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        bool state = (raw != 0);

        ImGui::PushID(addr);
        DrawLed(text.c_str(), state, radius, colorOn, colorOff);
        ImGui::PopID();
    }

private:
    static void DrawLed(const char* label, bool state, float radius, ImU32 colorOn, ImU32 colorOff) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImDrawList* draw = window->DrawList;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);

        ImRect bb(pos, ImVec2(pos.x + radius * 2, pos.y + radius * 2));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, 0)) return;

        // Is the item disabled through the flags (e.g. with BeginDisabled)?
        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        ImU32 baseColor;
        if (disabled) {
            baseColor = IM_COL32(80, 80, 80, 255);
        }
        else {
            baseColor = state ? colorOn : colorOff;
        }

        // Main circle
        draw->AddCircleFilled(center, radius, baseColor, 32);

        // Highlight (light translucent circle at the top left)
        ImVec2 highlightPos = ImVec2(center.x - radius * 0.25f, center.y - radius * 0.25f);
        draw->AddCircleFilled(highlightPos, radius * 0.3f, IM_COL32(255, 255, 255, 100), 16);

        // Outer glow, only when on
        if (state && !disabled) {
            for (int i = 0; i < 3; ++i) {
                float r = radius + i * 2.0f;
                ImU32 glowCol = (colorOn & 0x00FFFFFF) | (static_cast<int>(40 * (1.0f - i * 0.3f)) << 24);
                draw->AddCircle(center, r, glowCol, 32, 1.0f);
            }
        }

        // Text under the indicator (if set)
        if (label && label[0] != '\0') {
            ImVec2 textSize = ImGui::CalcTextSize(label);
            ImVec2 textPos = ImVec2(center.x - textSize.x * 0.5f, center.y + radius + 5.0f);
            draw->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), label);
        }
    }
};