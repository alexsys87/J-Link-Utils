// BitControlWidget.h
#pragma once
#include "../Localization.h"
#include "widget_common.h"
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>

enum BitMode : int {
    Bits8 = 1,
    Bits16 = 2,
    Bits32 = 4
};

class BitControlWidget : public LeafWidget {
    uint32_t address;
    BitMode initialMode;
    bool littleEndian;
    bool allowModeSwitch;
    BitMode currentMode; // current display mode (can change in the UI)

public:
    BitControlWidget(const char* lbl, uint32_t addr, BitMode mode, bool le, bool switchable = true)
        : LeafWidget(lbl), address(addr), initialMode(mode), littleEndian(le), allowModeSwitch(switchable), currentMode(mode) {
        // The drawing code uses the mode as the byte count: only 1, 2 and 4 are valid.
        if (mode != Bits8 && mode != Bits16 && mode != Bits32) initialMode = currentMode = Bits8;
    }

    static int ModeType(BitMode mode) {
        if (mode & Bits8) return U08_DEC;
        if (mode & Bits16) return U16_DEC;
        return U32_DEC;
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        uint32_t value = (uint32_t)ReadTypedValue(ctx, addr, ModeType(currentMode), littleEndian);

        uint32_t oldValue = value;

        ImGui::PushID(addr);
        DrawBitControl(text.c_str(), &value, &currentMode, allowModeSwitch);
        ImGui::PopID();

        // Value changed: write it back in the current mode
        if (value != oldValue)
            WriteTypedValue(ctx, addr, ModeType(currentMode), littleEndian, value);
        // Only the mode changed: nothing to write, currentMode is updated
    }

private:
    static void DrawBitControl(const char* label, uint32_t* value, BitMode* mode, bool switchable) {
        ImGui::PushID(label);
        ImGui::BeginGroup();

        const ImGuiStyle& style = ImGui::GetStyle();
        float cell_size = 18.0f;
        float spacing = style.ItemSpacing.x * 0.8f;
        float extra_spacing = spacing * 1.5f;
        float block_spacing = spacing * 2.5f;
        float text_offset_y = 3.0f;

        int blocks = static_cast<int>(*mode);
        int total_bits = blocks * 8;

        float block_width = 8 * cell_size + 7 * spacing + extra_spacing;
        float total_width = blocks * block_width + (blocks - 1) * block_spacing;

        ImVec2 area_start = ImGui::GetCursorScreenPos();

        ImDrawList* draw = ImGui::GetWindowDrawList();
        float shadow_height = cell_size + text_offset_y + ImGui::GetTextLineHeight() + 70;
        draw->AddRectFilled(
            ImVec2(area_start.x, area_start.y),
            ImVec2(area_start.x + total_width + 2, area_start.y + shadow_height + 2),
            IM_COL32(0, 0, 0, 40), 6.0f
        );

        ImGui::InvisibleButton("##bits_area", ImVec2(total_width, cell_size));
        bool area_clicked = ImGui::IsItemClicked();
        bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

        int clicked_bit = -1;
        if (area_clicked && !disabled) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float rel_x = mouse_pos.x - area_start.x;

            int block = -1;
            float x_cur = 0;
            for (int b = 0; b < blocks; ++b) {
                if (rel_x >= x_cur && rel_x < x_cur + block_width) {
                    block = b;
                    break;
                }
                x_cur += block_width + block_spacing;
            }

            if (block != -1) {
                float inner_x = rel_x - x_cur;
                float inner_cur = 0;
                for (int col = 0; col < 8; ++col) {
                    if (inner_x >= inner_cur && inner_x < inner_cur + cell_size) {
                        clicked_bit = (blocks - 1 - block) * 8 + (7 - col);
                        break;
                    }
                    inner_cur += cell_size;
                    if (col < 7) inner_cur += (col == 3) ? extra_spacing : spacing;
                }
            }
        }
        if (clicked_bit != -1 && clicked_bit < total_bits) {
            *value ^= (1u << clicked_bit);
        }

        std::vector<float> block_x_start(blocks);
        float x_offset = area_start.x;
        for (int b = 0; b < blocks; ++b) {
            block_x_start[b] = x_offset;
            float block_x = x_offset;

            for (int col = 0; col < 8; ++col) {
                int bit_index = (blocks - 1 - b) * 8 + (7 - col);
                bool is_set = (*value >> bit_index) & 1;

                float rect_x = block_x;
                for (int c = 0; c < col; ++c) {
                    rect_x += cell_size;
                    if (c < 7) rect_x += (c == 3) ? extra_spacing : spacing;
                }
                ImVec2 rect_min(rect_x, area_start.y);
                ImVec2 rect_max(rect_x + cell_size, area_start.y + cell_size);

                ImU32 bg_color;
                if (disabled) {
                    bg_color = IM_COL32(80, 80, 80, 255);
                }
                else if (is_set) {
                    bg_color = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
                }
                else {
                    bg_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
                }

                draw->AddRectFilled(rect_min, rect_max, bg_color, 3.0f);
                draw->AddRect(rect_min, rect_max, ImGui::GetColorU32(ImGuiCol_Border), 3.0f, 0, 1.0f);

                char text[2] = { is_set ? '1' : '0', '\0' };
                ImVec2 text_size = ImGui::CalcTextSize(text);
                float text_x = rect_min.x + (cell_size - text_size.x) * 0.5f;
                float text_y = rect_min.y + (cell_size - text_size.y) * 0.5f;
                draw->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), text);
            }

            // Bit index labels under the block
            float label_x = block_x;
            for (int col = 0; col < 8; ++col) {
                int bit_index = (blocks - 1 - b) * 8 + (7 - col);
                std::string idx = std::to_string(bit_index);
                ImVec2 text_size = ImGui::CalcTextSize(idx.c_str());
                float text_x = label_x + (cell_size - text_size.x) * 0.5f;
                float text_y = area_start.y + cell_size + text_offset_y;
                ImGui::SetCursorScreenPos(ImVec2(text_x, text_y));
                ImGui::TextDisabled("%s", idx.c_str());

                label_x += cell_size;
                if (col < 7) label_x += (col == 3) ? extra_spacing : spacing;
            }

            x_offset += block_width + block_spacing;
        }

        // Byte HEX frames under each block
        float frame_width = cell_size * 8 + 50;
        float frame_height = 20.0f;
        float frame_y = area_start.y + cell_size + text_offset_y + ImGui::GetTextLineHeight() + 15;

        for (int b = 0; b < blocks; ++b) {
            float block_center = block_x_start[b] + block_width * 0.5f;
            float frame_x = block_center - frame_width * 0.5f;

            int byte_index = blocks - 1 - b;
            uint8_t byte_value = (*value >> (8 * byte_index)) & 0xFF;

            char label[8];
            snprintf(label, sizeof(label), "B%d", byte_index);
            ImVec2 label_size = ImGui::CalcTextSize(label);
            ImGui::SetCursorScreenPos(ImVec2(frame_x + (frame_width - label_size.x) * 0.5f, frame_y - label_size.y ));
            ImGui::TextDisabled("%s", label);

            ImGui::SetCursorScreenPos(ImVec2(frame_x, frame_y));
            ImGui::BeginChild(ImGui::GetID(("byte_frame" + std::to_string(b)).c_str()), ImVec2(frame_width, frame_height),
                ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
            ImGui::SetCursorPosY((frame_height - ImGui::GetTextLineHeight()) * 0.5f);
            char hex_byte[8];
            snprintf(hex_byte, sizeof(hex_byte), "0x%02X", byte_value);
            ImGui::SetCursorPosX((frame_width - ImGui::CalcTextSize(hex_byte).x) * 0.5f);
            ImGui::Text("%s", hex_byte);
            ImGui::EndChild();
        }

        if (switchable) {
            ImGui::SetCursorScreenPos(ImVec2(area_start.x, frame_y + frame_height + 12));
            ImGui::Text(app_language::Tr("Mode:", "Режим:"));
            ImGui::SameLine();
            int mode_int = static_cast<int>(*mode);
            if (ImGui::RadioButton(app_language::Label("8 bit", "8 бит"), &mode_int, Bits8)) {
                *mode = Bits8;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(app_language::Label("16 bit", "16 бит"), &mode_int, Bits16)) {
                *mode = Bits16;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(app_language::Label("32 bit", "32 бита"), &mode_int, Bits32)) {
                *mode = Bits32;
            }
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }
};