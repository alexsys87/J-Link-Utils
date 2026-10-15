// CustomVerticalSliderWidget.h
#pragma once
#include "widget_common.h"

class CustomVerticalSliderWidget : public LeafWidget {
    uint32_t address;
    float minVal;
    float maxVal;
    ImVec2 size;
    bool littleEndian;
    int dataType; // data type: FLOAT, S16_DEC, U16_DEC, S32_DEC, U32_DEC, ...

public:
    CustomVerticalSliderWidget(const char* lbl, uint32_t addr, float minV, float maxV, float w, float h, bool le, int dtype = FLOAT)
        : LeafWidget(lbl), address(addr), minVal(minV), maxVal(maxV), size(w, h), littleEndian(le), dataType(dtype) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        // Read the current value as float according to the data type
        float value = (float)ReadTypedValue(ctx, addr, dataType, littleEndian);

        float oldValue = value;

        ImGui::PushID(addr);
        RenderCustomVerticalSlider(text.c_str(), size, &value, minVal, maxVal);
        ImGui::PopID();

        // Value changed: write it back
        if (value != oldValue)
            WriteTypedValue(ctx, addr, dataType, littleEndian, value);
    }

private:
    static void RenderCustomVerticalSlider(const char* label, const ImVec2& size, float* value, float min_v, float max_v) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImVec2 pos = window->DC.CursorPos;
        float center_x = pos.x + size.x * 0.5f;

        ImDrawList* draw_list = window->DrawList;

        // Center line
        draw_list->AddLine(ImVec2(center_x, pos.y), ImVec2(center_x, pos.y + size.y),
            ImGui::GetColorU32(ImGuiCol_Border), 1.0f);

        // Ticks
        int num_ticks = 10;
        for (int i = 0; i <= num_ticks; ++i) {
            float t = (float)i / (float)num_ticks;
            float y = pos.y + (1.0f - t) * size.y;
            float tick_off = 8.0f;
            draw_list->AddLine(ImVec2(center_x - tick_off, y), ImVec2(center_x - 2, y),
                ImGui::GetColorU32(ImGuiCol_TextDisabled));
            draw_list->AddLine(ImVec2(center_x + 2, y), ImVec2(center_x + tick_off, y),
                ImGui::GetColorU32(ImGuiCol_TextDisabled));
        }

        // Transparent background for the standard slider
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12.0f);

        // Standard vertical slider
        ImGui::VSliderScalar(label, size, ImGuiDataType_Float, value, &min_v, &max_v, "");

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }
};