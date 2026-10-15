// SegmentButtonWidget.h
#pragma once
#include "widget_common.h"
#include <vector>
#include <string>

struct SegmentItem {
    std::string text;
    int value = 0;      // значение, которое будет записано при выборе этого сегмента
};

class SegmentButtonWidget : public LeafWidget {
    uint32_t address;
    std::vector<SegmentItem> segments;
    int currentValue;           // кэшированное текущее значение
    bool littleEndian;

public:
    SegmentButtonWidget(const char* lbl, uint32_t addr, const std::vector<SegmentItem>& items, bool le)
        : LeafWidget(lbl), address(addr), segments(items), currentValue(0), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        // Читаем текущее значение из памяти (16 бит)
        uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
        currentValue = static_cast<int>(raw);

        ImGui::PushID(addr);
        bool changed = DrawSegmentButtons(text.c_str(), segments, currentValue);
        ImGui::PopID();

        // Если значение изменилось, записываем обратно в память
        if (changed) {
            uint32_t writeAddr = addr;
            uint16_t v = static_cast<uint16_t>(currentValue);
            DataReadWriteU16(ctx, nullptr, &v, &writeAddr);
        }
    }

private:
    static bool DrawSegmentButtons(const char* label, const std::vector<SegmentItem>& segments, int& currentValue) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImDrawList* draw = window->DrawList;
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
        bool changed = false;

        // Вычисляем общую ширину
        float totalWidth = 0;
        float spacing = style.ItemSpacing.x;
        for (const auto& seg : segments) {
            ImVec2 textSize = ImGui::CalcTextSize(seg.text.c_str());
            totalWidth += textSize.x + style.FramePadding.x * 2;
        }
        totalWidth += spacing * (segments.size() - 1);

        // Фон группы (лёгкая тень)
        ImRect groupRect(pos, ImVec2(pos.x + totalWidth, pos.y + ImGui::GetFrameHeight()));
        draw->AddRectFilled(groupRect.Min, groupRect.Max, IM_COL32(0, 0, 0, 30), style.FrameRounding);

        float xOffset = pos.x;
        for (size_t i = 0; i < segments.size(); ++i) {
            const auto& seg = segments[i];
            bool isSelected = (seg.value == currentValue);

            ImVec2 textSize = ImGui::CalcTextSize(seg.text.c_str());
            float buttonWidth = textSize.x + style.FramePadding.x * 2;
            ImRect buttonRect(xOffset, pos.y, xOffset + buttonWidth, pos.y + ImGui::GetFrameHeight());

            ImGui::SetCursorScreenPos(buttonRect.Min);
            ImGui::PushID(static_cast<int>(i));

            // Цвета кнопки
            ImVec4 colBg, colHovered, colActive;
            if (isSelected) {
                colBg = style.Colors[ImGuiCol_ButtonActive];
                colHovered = style.Colors[ImGuiCol_ButtonHovered];
                colActive = style.Colors[ImGuiCol_ButtonActive];
            }
            else {
                colBg = style.Colors[ImGuiCol_Button];
                colHovered = style.Colors[ImGuiCol_ButtonHovered];
                colActive = style.Colors[ImGuiCol_ButtonActive];
            }

            if (disabled) {
                ImVec4 gray = colBg; gray.x *= 0.5f; gray.y *= 0.5f; gray.z *= 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Button, gray);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, gray);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, gray);
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, colBg);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHovered);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
            }

            if (ImGui::Button(seg.text.c_str(), ImVec2(buttonWidth, 0))) {
                if (!disabled && currentValue != seg.value) {
                    currentValue = seg.value;
                    changed = true;
                }
            }
            ImGui::PopStyleColor(3);

            if (i < segments.size() - 1) ImGui::SameLine(0, spacing);
            ImGui::PopID();

            xOffset += buttonWidth + spacing;
        }

        // Метка над группой
        if (label && label[0] != '\0') {
            ImVec2 labelSize = ImGui::CalcTextSize(label);
            ImVec2 labelPos(pos.x + (totalWidth - labelSize.x) * 0.5f, pos.y - labelSize.y - 2);
            draw->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), label);
        }

        // Занимаем место
        ImGui::Dummy(ImVec2(totalWidth, ImGui::GetFrameHeight() + (label ? ImGui::GetTextLineHeight() + 2 : 0)));

        return changed;
    }
};