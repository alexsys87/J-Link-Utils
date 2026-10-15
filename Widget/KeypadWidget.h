#pragma once
#include "widget_common.h"
#include <cctype>
#include <string>
#include <cstdlib>

class KeypadWidget : public LeafWidget {
    uint32_t address;
    int minVal;
    int maxVal;
    int maxDigits;
    bool littleEndian;
    std::string inputBuffer;

public:
    KeypadWidget(const char* lbl, uint32_t addr, int minV, int maxV, int digits, bool le)
        : LeafWidget(lbl), address(addr), minVal(minV), maxVal(maxV), maxDigits(digits), littleEndian(le) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        uint16_t raw = (uint16_t)ReadTypedValue(ctx, addr, U16_DEC, littleEndian);
        int memValue = static_cast<int>(raw);
        if (memValue < minVal) memValue = minVal;
        if (memValue > maxVal) memValue = maxVal;

        bool useMem = inputBuffer.empty();
        int displayValue = useMem ? memValue : 0;

        if (!inputBuffer.empty()) {
            char* end;
            long val = std::strtol(inputBuffer.c_str(), &end, 10);
            if (*end == '\0') {
                displayValue = static_cast<int>(val);
            }
            else {
                displayValue = memValue;
            }
        }

        ImGui::PushID(addr);
        bool enterPressed = DrawKeypad(text.c_str(), displayValue, inputBuffer, minVal, maxVal, maxDigits);
        ImGui::PopID();

        if (enterPressed && !inputBuffer.empty()) {
            char* end;
            long val = std::strtol(inputBuffer.c_str(), &end, 10);
            if (*end == '\0') {
                int newVal = static_cast<int>(val);
                if (newVal >= minVal && newVal <= maxVal) {
                    uint16_t writeVal = static_cast<uint16_t>(newVal);
                    uint32_t writeAddr = addr;
                    WriteTypedValue(ctx, writeAddr, U16_DEC, littleEndian, writeVal);
                    inputBuffer.clear();
                }
            }
        }
    }

private:
    bool DrawKeypad(const char* label, int displayValue, std::string& inputBuffer, int minVal, int maxVal, int maxDigits) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImDrawList* draw = window->DrawList;
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float itemSpacing = style.ItemSpacing.x;
        float buttonSize = 50.0f;
        float displayHeight = 40.0f;
        float totalWidth = buttonSize * 3 + itemSpacing * 2;

        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        // Display
        ImRect displayRect(pos, ImVec2(pos.x + totalWidth, pos.y + displayHeight));
        ImGui::ItemSize(displayRect);
        ImGui::ItemAdd(displayRect, 0);

        // Draw the display
        draw->AddRectFilled(displayRect.Min, displayRect.Max, IM_COL32(20, 20, 20, 255), 4.0f);
        draw->AddRect(displayRect.Min, displayRect.Max, IM_COL32(0, 0, 0, 255), 4.0f, 0, 2.0f);
        float split_y = displayRect.Min.y + displayHeight * 0.6f;
        draw->AddRectFilledMultiColor(
            ImVec2(displayRect.Min.x + 1, displayRect.Min.y + 1),
            ImVec2(displayRect.Max.x - 1, split_y),
            IM_COL32(70, 70, 70, 255), IM_COL32(70, 70, 70, 255),
            IM_COL32(40, 40, 40, 255), IM_COL32(40, 40, 40, 255));
        draw->AddRectFilled(ImVec2(displayRect.Min.x + 1, split_y), ImVec2(displayRect.Max.x - 1, displayRect.Max.y - 1),
            IM_COL32(25, 25, 25, 255));
        draw->AddLine(ImVec2(displayRect.Min.x + 1, split_y), ImVec2(displayRect.Max.x - 1, split_y), IM_COL32(0, 0, 0, 200), 1.0f);

        char dispBuf[32];
        if (!inputBuffer.empty()) {
            snprintf(dispBuf, sizeof(dispBuf), "%s", inputBuffer.c_str());
        }
        else {
            snprintf(dispBuf, sizeof(dispBuf), "%d", displayValue);
        }
        ImVec2 textSize = ImGui::CalcTextSize(dispBuf);
        ImVec2 textPos(displayRect.Max.x - textSize.x - 10.0f, displayRect.Min.y + (displayHeight - textSize.y) * 0.5f);
        draw->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 150), dispBuf);
        draw->AddText(textPos, IM_COL32(255, 255, 255, 255), dispBuf);

        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + displayHeight + style.ItemSpacing.y));

        const char* buttons[4][3] = {
            {"7", "8", "9"},
            {"4", "5", "6"},
            {"1", "2", "3"},
            {"C", "0", "E"}
        };

        bool enterPressed = false;

        for (int row = 0; row < 4; ++row) {
            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + displayHeight + style.ItemSpacing.y + row * (buttonSize + style.ItemSpacing.y)));
            for (int col = 0; col < 3; ++col) {
                ImGui::PushID(row * 3 + col);
                const char* btn = buttons[row][col];
                bool isDigit = isdigit(btn[0]);
                bool isEnter = (btn[0] == 'E');
                bool isClear = (btn[0] == 'C');

                if (!disabled) {
                    if (isEnter)
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                    else if (isClear)
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    else
                        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(
                        style.Colors[ImGuiCol_Button].x * 0.5f,
                        style.Colors[ImGuiCol_Button].y * 0.5f,
                        style.Colors[ImGuiCol_Button].z * 0.5f,
                        style.Colors[ImGuiCol_Button].w));
                }

                if (ImGui::Button(btn, ImVec2(buttonSize, buttonSize))) {
                    if (!disabled) {
                        if (isDigit) {
                            if ((int)inputBuffer.size() < maxDigits) {
                                inputBuffer += btn;
                                long val = std::strtol(inputBuffer.c_str(), nullptr, 10);
                                if (val > maxVal) {
                                    inputBuffer.pop_back();
                                }
                            }
                        }
                        else if (isClear) {
                            inputBuffer.clear();
                        }
                        else if (isEnter) {
                            enterPressed = true;
                        }
                    }
                }
                ImGui::PopStyleColor();
                if (col < 2) ImGui::SameLine(0, style.ItemSpacing.x);
                ImGui::PopID();
            }
        }

        ImGui::Dummy(ImVec2(totalWidth, displayHeight + 4 * (buttonSize + style.ItemSpacing.y)));
        return enterPressed;
    }
};