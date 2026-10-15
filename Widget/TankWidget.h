#pragma once
#include "widget_common.h"
#include <cmath>

// Value data types
enum TankDataType {
    TANK_FLOAT = 0,
    TANK_U16 = 1,
    TANK_S16 = 2,
    TANK_U32 = 3,
    TANK_S32 = 4,
    // more can be added
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
    int dataType; // values from the enum, int for simplicity

public:
    TankWidget(const char* lbl, uint32_t addr, float minV, float maxV,
        float w, float h, const char* u, ImU32 liqCol, ImU32 metCol, bool le, int dtype = 0)
        : label(lbl ? lbl : ""), address(addr), minVal(minV), maxVal(maxV),
        width(w), height(h), unit(u ? u : ""), liquidColor(liqCol), metalColor(metCol), littleEndian(le), dataType(dtype) {
    }

    // Tank keeps its own type codes (TankDataType) for compatibility.
    static int ToDataType(int tankType) {
        switch (tankType) {
        case TANK_U16: return U16_DEC;
        case TANK_S16: return S16_DEC;
        case TANK_U32: return U32_DEC;
        case TANK_S32: return S32_DEC;
        default: return FLOAT;
        }
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        float value = (float)ReadTypedValue(ctx, addr, ToDataType(dataType), littleEndian);

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

        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 col_text_dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        float margin = 12.0f;
        float innerWidth = width - 2 * margin;
        float innerHeight = height - 2 * margin;
        float liquidLevel = (value - min_val) / (max_val - min_val);
        liquidLevel = ImClamp(liquidLevel, 0.0f, 1.0f);
        float liquidY = pos.y + height - margin - liquidLevel * innerHeight;

        // --- Metal body with a gradient and highlights ---
        // Lighter and darker metal; IM_COL32 keeps red in the low byte and the
        // float conversion clamps the brightened channels instead of overflowing.
        const ImVec4 metal = ImGui::ColorConvertU32ToFloat4(metalColor);
        ImU32 metalLight = ImGui::ColorConvertFloat4ToU32(ImVec4(metal.x * 1.2f, metal.y * 1.2f, metal.z * 1.2f, metal.w));
        ImU32 metalDark = ImGui::ColorConvertFloat4ToU32(ImVec4(metal.x * 0.7f, metal.y * 0.7f, metal.z * 0.7f, metal.w));

        // Main body (gradient from top to bottom)
        draw->AddRectFilledMultiColor(
            bb.Min, bb.Max,
            metalLight, metalLight, metalDark, metalDark
        );

        // Highlight at the top
        float highlightHeight = height * 0.15f;
        ImVec2 hlMin(bb.Min.x, bb.Min.y);
        ImVec2 hlMax(bb.Max.x, bb.Min.y + highlightHeight);
        draw->AddRectFilledMultiColor(
            hlMin, hlMax,
            IM_COL32(255, 255, 255, 40), IM_COL32(255, 255, 255, 40),
            IM_COL32(255, 255, 255, 10), IM_COL32(255, 255, 255, 10)
        );

        // Outer outline
        draw->AddRect(bb.Min, bb.Max, col_border, style.FrameRounding, 0, 2.0f);

        // Inner cavity (glass) with a gradient
        ImVec2 innerMin(pos.x + margin, pos.y + margin);
        ImVec2 innerMax(pos.x + width - margin, pos.y + height - margin);
        // Glass: translucent, slightly bluish, with a light gradient
        draw->AddRectFilledMultiColor(
            innerMin, innerMax,
            IM_COL32(200, 220, 240, 30), IM_COL32(200, 220, 240, 30),
            IM_COL32(180, 200, 220, 60), IM_COL32(180, 200, 220, 60)
        );
        draw->AddRect(innerMin, innerMax, IM_COL32(100, 100, 120, 100), style.FrameRounding * 0.7f, 0, 1.0f);

        // Liquid with a gradient and bubbles (cosmetic)
        if (liquidLevel > 0.0f) {
            ImVec2 liqMin(innerMin.x, liquidY);
            ImVec2 liqMax(innerMax.x, innerMax.y);
            // liquid gradient (lighter at the top)
            ImU32 liqLight = liquidColor; // could be made lighter
            const ImVec4 liquid = ImGui::ColorConvertU32ToFloat4(liquidColor);
            ImU32 liqDark = ImGui::ColorConvertFloat4ToU32(ImVec4(liquid.x * 0.7f, liquid.y * 0.7f, liquid.z * 0.7f, liquid.w));
            draw->AddRectFilledMultiColor(
                liqMin, liqMax,
                liqLight, liqLight, liqDark, liqDark
            );

            // Bubbles (a few translucent circles)
            if (liquidLevel > 0.2f) {
                for (int i = 0; i < 5; ++i) {
                    float bubbleX = innerMin.x + (i * innerWidth / 6.0f) + innerWidth / 12.0f;
                    float bubbleY = liquidY + (liquidLevel * innerHeight) * (i % 3) / 4.0f;
                    if (bubbleY < liquidY + 5) bubbleY = liquidY + 5;
                    draw->AddCircleFilled(ImVec2(bubbleX, bubbleY), 4.0f, IM_COL32(255, 255, 255, 120));
                }
            }
        }

        // Highlight on the liquid surface
        if (liquidLevel > 0.0f && liquidLevel < 1.0f) {
            draw->AddLine(ImVec2(innerMin.x + 4, liquidY),
                ImVec2(innerMax.x - 4, liquidY),
                IM_COL32(255, 255, 255, 180), 2.0f);
        }

        // Side scale (ticks and numbers)
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

        // Digital display with a frame at the bottom
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "%.1f %s", value, unit);
        ImVec2 valSize = ImGui::CalcTextSize(valBuf);
        float panelWidth = valSize.x + 20;
        float panelHeight = valSize.y + 8;
        ImVec2 panelPos(pos.x + (width - panelWidth) * 0.5f, pos.y + height + 5);
        // display background
        draw->AddRectFilled(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), IM_COL32(0, 0, 0, 180), 4.0f);
        draw->AddRect(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), col_border, 4.0f);
        // text
        draw->AddText(ImVec2(panelPos.x + 10, panelPos.y + 4), col_text, valBuf);

        // Reserved space (including the display)
        ImGui::Dummy(ImVec2(width, height + panelHeight + 10));
    }
};