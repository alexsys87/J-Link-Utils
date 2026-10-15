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
        float value = 0.0f;

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
            if (dataType == S32_DEC) {
                int32_t signedVal = (int32_t)raw;
                value = (float)signedVal;
            }
            else {
                value = (float)raw;
            }
        }
        // 8-bit types (S08_DEC, U08_DEC) could be added here

        float oldValue = value;

        ImGui::PushID(addr);
        RenderCustomVerticalSlider(text.c_str(), size, &value, minVal, maxVal);
        ImGui::PopID();

        // Value changed: write it back
        if (value != oldValue) {
            uint32_t writeAddr = addr;
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
                uint16_t raw;
                if (dataType == S16_DEC) {
                    // Convert to int16_t with range clamping
                    int32_t intVal = (int32_t)value;
                    if (intVal < -32768) intVal = -32768;
                    if (intVal > 32767) intVal = 32767;
                    raw = (uint16_t)(int16_t)intVal;
                }
                else {
                    // U16: round and clamp
                    uint32_t intVal = (uint32_t)(value + 0.5f);
                    if (intVal > 65535) intVal = 65535;
                    raw = (uint16_t)intVal;
                }
                DataReadWriteU16(ctx, nullptr, &raw, &writeAddr);
            }
            else if (dataType == S32_DEC || dataType == U32_DEC) {
                uint32_t raw;
                if (dataType == S32_DEC) {
                    int32_t intVal = (int32_t)value; // truncates toward zero
                    raw = (uint32_t)intVal;
                }
                else {
                    uint32_t intVal = (uint32_t)(value + 0.5f);
                    raw = intVal;
                }
                DataReadWriteU32(ctx, nullptr, &raw, &writeAddr);
            }
        }
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