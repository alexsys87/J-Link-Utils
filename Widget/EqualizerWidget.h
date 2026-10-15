// EqualizerWidget.h
#pragma once
#include "widget_common.h"
#include <vector>
#include <string>

enum EqDataType {
    EQ_FLOAT,   // 32-bit float (требует DataReadWriteFT32/DataReadWriteU32)
    EQ_INT16    // 16-bit integer (использует DataReadWriteU16)
};

class EqualizerWidget : public UIWidget {
    std::string label;
    uint32_t baseAddress;
    int numBands;
    float minVal;
    float maxVal;
    float bandWidth;          // ширина одного ползунка (общая ширина = numBands * bandWidth)
    float height;
    EqDataType dataType;
    bool littleEndian;
    int addressStep;          // шаг адреса между соседними полосами (обычно 2 для float, 1 для int16)

    std::vector<float> values; // кэшированные значения

public:
    EqualizerWidget(const char* lbl, uint32_t addr, int bands, float minV, float maxV,
        float w, float h, EqDataType type, bool le, int step)
        : label(lbl ? lbl : ""), baseAddress(addr), numBands(bands), minVal(minV), maxVal(maxV),
        bandWidth(w), height(h), dataType(type), littleEndian(le), addressStep(step),
        values(bands, 0.0f) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addrBase = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : baseAddress;

        // Чтение значений из памяти
        for (int i = 0; i < numBands; ++i) {
            uint32_t addr = addrBase + i * addressStep;
            if (dataType == EQ_FLOAT) {
                if (littleEndian) {
                    uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
                    values[i] = *(float*)&raw;
                }
                else {
                    values[i] = DataReadWriteFT32(ctx, nullptr, nullptr, &addr);
                }
            }
            else { // EQ_INT16
                uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
                values[i] = static_cast<float>(raw);
            }
            // Ограничение
            if (values[i] < minVal) values[i] = minVal;
            if (values[i] > maxVal) values[i] = maxVal;
        }

        ImGui::PushID(this);
        bool changed = DrawEqualizer(label.c_str(), values, numBands, minVal, maxVal, bandWidth, height);
        ImGui::PopID();

        // Запись изменённых значений обратно в память
        if (changed) {
            for (int i = 0; i < numBands; ++i) {
                uint32_t addr = addrBase + i * addressStep;
                if (dataType == EQ_FLOAT) {
                    if (littleEndian) {
                        uint32_t raw;
                        memcpy(&raw, &values[i], sizeof(float));
                        DataReadWriteU32(ctx, nullptr, &raw, &addr);
                    }
                    else {
                        DataReadWriteFT32(ctx, nullptr, &values[i], &addr);
                    }
                }
                else { // EQ_INT16
                    uint16_t v = static_cast<uint16_t>(values[i]);
                    DataReadWriteU16(ctx, nullptr, &v, &addr);
                }
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

        // Фоновая панель (небольшая тень)
        ImRect panel(pos, ImVec2(pos.x + totalWidth + 10, pos.y + totalHeight + 10));
        draw->AddRectFilled(panel.Min, panel.Max, IM_COL32(0, 0, 0, 30), 5.0f);

        bool anyChanged = false;

        for (int i = 0; i < numBands; ++i) {
            ImGui::PushID(i);
            ImVec2 sliderPos(pos.x + i * (bandWidth + style.ItemSpacing.x), pos.y);
            ImGui::SetCursorScreenPos(sliderPos);

            // Вертикальный слайдер
            float v = values[i];
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, style.Colors[ImGuiCol_SliderGrab]);
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, style.Colors[ImGuiCol_SliderGrabActive]);

            if (ImGui::VSliderFloat("##slider", ImVec2(bandWidth, height), &v, minVal, maxVal, "")) {
                values[i] = v;
                anyChanged = true;
            }

            ImGui::PopStyleColor(3);

            // Метка снизу (частота)
            char bandLabel[8];
            snprintf(bandLabel, sizeof(bandLabel), "%d", i * 100); // пример метки, можно настраивать
            ImVec2 labelSize = ImGui::CalcTextSize(bandLabel);
            ImVec2 labelPos(sliderPos.x + (bandWidth - labelSize.x) * 0.5f, sliderPos.y + height + 2);
            draw->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), bandLabel);

            ImGui::PopID();
        }

        // Общая подпись (label) под всеми ползунками
        if (label && label[0] != '\0') {
            ImVec2 labelSize = ImGui::CalcTextSize(label);
            ImVec2 labelPos(pos.x + (totalWidth - labelSize.x) * 0.5f, pos.y + height + ImGui::GetTextLineHeight() + 5);
            draw->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), label);
        }

        ImGui::Dummy(ImVec2(totalWidth, totalHeight + 20));
        return anyChanged;
    }
};