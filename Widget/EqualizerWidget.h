// EqualizerWidget.h
#pragma once
#include "widget_common.h"
#include <vector>
#include <string>

enum EqDataType {
    EQ_FLOAT,   // 32-bit float (DataReadWriteFT32/DataReadWriteU32)
    EQ_INT16    // 16-bit integer (DataReadWriteU16)
};

class EqualizerWidget : public UIWidget {
    std::string label;
    uint32_t baseAddress;
    int numBands;
    float minVal;
    float maxVal;
    float bandWidth;          // width of one slider (total width = numBands * bandWidth)
    float height;
    EqDataType dataType;
    bool littleEndian;
    int addressStep;          // bytes between neighbouring bands (4 for float, 2 for int16 by default)

    std::vector<float> values; // cached values

public:
    EqualizerWidget(const char* lbl, uint32_t addr, int bands, float minV, float maxV,
        float w, float h, EqDataType type, bool le, int step)
        : label(lbl ? lbl : ""), baseAddress(addr), numBands(bands), minVal(minV), maxVal(maxV),
        bandWidth(w), height(h), dataType(type), littleEndian(le), addressStep(step),
        values(bands, 0.0f) {
    }

    // EQ_INT16 values have always been read and written as unsigned 16-bit.
    int ValueType() const { return dataType == EQ_INT16 ? U16_DEC : FLOAT; }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addrBase = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : baseAddress;

        // Read the values from memory
        for (int i = 0; i < numBands; ++i) {
            uint32_t addr = addrBase + i * addressStep;
            values[i] = (float)ReadTypedValue(ctx, addr, ValueType(), littleEndian);
            // Clamp
            if (values[i] < minVal) values[i] = minVal;
            if (values[i] > maxVal) values[i] = maxVal;
        }

        ImGui::PushID(this);
        bool changed = DrawEqualizer(label.c_str(), values, numBands, minVal, maxVal, bandWidth, height);
        ImGui::PopID();

        // Write changed values back to memory
        if (changed) {
            for (int i = 0; i < numBands; ++i) {
                uint32_t addr = addrBase + i * addressStep;
                WriteTypedValue(ctx, addr, ValueType(), littleEndian, values[i]);
            }
        }
    }

private:
    static bool DrawEqualizer(const char* label, std::vector<float>& values, int numBands,
        float minVal, float maxVal, float bandWidth, float height) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImDrawList* draw = window->DrawList;
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float totalWidth = numBands * bandWidth + (numBands - 1) * style.ItemSpacing.x;
        float totalHeight = height + ImGui::GetTextLineHeight() + style.ItemSpacing.y * 2;

        // Background panel (light shadow)
        ImRect panel(pos, ImVec2(pos.x + totalWidth + 10, pos.y + totalHeight + 10));
        draw->AddRectFilled(panel.Min, panel.Max, IM_COL32(0, 0, 0, 30), 5.0f);

        bool anyChanged = false;

        for (int i = 0; i < numBands; ++i) {
            ImGui::PushID(i);
            ImVec2 sliderPos(pos.x + i * (bandWidth + style.ItemSpacing.x), pos.y);
            ImGui::SetCursorScreenPos(sliderPos);

            // Vertical slider
            float v = values[i];
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, style.Colors[ImGuiCol_SliderGrab]);
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, style.Colors[ImGuiCol_SliderGrabActive]);

            if (ImGui::VSliderFloat("##slider", ImVec2(bandWidth, height), &v, minVal, maxVal, "")) {
                values[i] = v;
                anyChanged = true;
            }

            ImGui::PopStyleColor(3);

            // Label below (frequency)
            char bandLabel[8];
            snprintf(bandLabel, sizeof(bandLabel), "%d", i * 100); // sample label, can be customized
            ImVec2 labelSize = ImGui::CalcTextSize(bandLabel);
            ImVec2 labelPos(sliderPos.x + (bandWidth - labelSize.x) * 0.5f, sliderPos.y + height + 2);
            draw->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), bandLabel);

            ImGui::PopID();
        }

        // Common label under all sliders
        if (label && label[0] != '\0') {
            ImVec2 labelSize = ImGui::CalcTextSize(label);
            ImVec2 labelPos(pos.x + (totalWidth - labelSize.x) * 0.5f, pos.y + height + ImGui::GetTextLineHeight() + 5);
            draw->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), label);
        }

        ImGui::Dummy(ImVec2(totalWidth, totalHeight + 20));
        return anyChanged;
    }
};