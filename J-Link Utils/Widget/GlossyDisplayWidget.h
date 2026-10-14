// GlossyDisplayWidget.h
#pragma once
#include "widget_common.h"
#include "imgui_internal.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

class GlossyDisplayWidget : public LeafWidget {
    uint32_t address;
    ImVec2 size;
    bool littleEndian;
    int dataType;
    bool hexMode;
    int digits;
    int decimalPlaces;

public:
    GlossyDisplayWidget(const char* lbl, uint32_t addr, float w, float h, bool le,
        int dtype = U16_DEC, bool hex = false, int dig = 0, int dec = 2)
        : LeafWidget(lbl), address(addr), size(w, h), littleEndian(le),
        dataType(dtype), hexMode(hex), digits(dig), decimalPlaces(dec) {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;

        char buf[64] = { 0 };

        switch (dataType) {
        case BOOLEAN: {
            uint16_t val = DataReadWriteBoolean(ctx, nullptr, nullptr, &addr) & 0x0001;
            snprintf(buf, sizeof(buf), "%d", val);
            break;
        }
        case S08_DEC:
        case U08_DEC: {
            uint16_t raw = DataReadWriteU08(ctx, nullptr, nullptr, &addr) & 0x00FF;
            if (dataType == S08_DEC) {
                int8_t v = (int8_t)raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*X" : "%X", digits, (uint8_t)v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*d" : "%d", digits, v);
            }
            else {
                uint8_t v = (uint8_t)raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*X" : "%X", digits, v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*u" : "%u", digits, v);
            }
            break;
        }
        case S16_DEC:
        case U16_DEC: {
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) raw = Swap16(raw);
            if (dataType == S16_DEC) {
                int16_t v = (int16_t)raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*X" : "%X", digits, (uint16_t)v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*d" : "%d", digits, v);
            }
            else {
                uint16_t v = raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*X" : "%X", digits, v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*u" : "%u", digits, v);
            }
            break;
        }
        case S32_DEC:
        case U32_DEC: {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) raw = SwapLeToBe32(raw);
            if (dataType == S32_DEC) {
                int32_t v = (int32_t)raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*X" : "%X", digits, (uint32_t)v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*d" : "%d", digits, v);
            }
            else {
                uint32_t v = raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*X" : "%X", digits, v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*u" : "%u", digits, v);
            }
            break;
        }
        case S64_DEC:
        case U64_DEC: {
            uint64_t raw = DataReadWriteU64(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) raw = SwapLeToBe64(raw);
            if (dataType == S64_DEC) {
                int64_t v = (int64_t)raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*llX" : "%llX", digits, (unsigned long long)v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*lld" : "%lld", digits, (long long)v);
            }
            else {
                uint64_t v = raw;
                if (hexMode)
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*llX" : "%llX", digits, (unsigned long long)v);
                else
                    snprintf(buf, sizeof(buf), digits > 0 ? "%0*llu" : "%llu", digits, (unsigned long long)v);
            }
            break;
        }
        case FLOAT:
        case FLOAT_INV: {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (dataType == FLOAT) {
                if (!littleEndian) raw = SwapLeToBe32(raw);
            }
            else { // FLOAT_INV
                if (littleEndian) raw = SwapLeToBe32(raw);
            }
            float f = *(float*)&raw;
            snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, f);
            break;
        }
        default:
            // fallback: читаем как 16-бит беззнаковое
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (hexMode) {
                if (digits > 0)
                    snprintf(buf, sizeof(buf), "%0*X", digits, raw);
                else
                    snprintf(buf, sizeof(buf), "%X", raw);
            }
            else {
                if (digits > 0)
                    snprintf(buf, sizeof(buf), "%0*d", digits, raw);
                else
                    snprintf(buf, sizeof(buf), "%d", raw);
            }
            break;
        }

        ImVec2 drawSize = size;
        if (drawSize.x <= 0) drawSize.x = 120;
        if (drawSize.y <= 0) drawSize.y = 40;

        ImGui::PushID(addr);
        DrawGlossyDisplay(buf, drawSize);
        ImGui::PopID();
    }

private:
    static void DrawGlossyDisplay(const char* text, ImVec2 size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(text))) return;

        ImDrawList* draw = window->DrawList;
        float x = pos.x, y = pos.y;
        float w = size.x, h = size.y;
        float rounding = 4.0f;

        draw->AddRectFilled(pos, ImVec2(x + w, y + h), IM_COL32(20, 20, 20, 255), rounding);
        draw->AddRect(pos, ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 255), rounding, 0, 2.0f);

        float split_y = y + h * 0.6f;
        draw->AddRectFilledMultiColor(
            ImVec2(x + 1, y + 1), ImVec2(x + w - 1, split_y),
            IM_COL32(70, 70, 70, 255),
            IM_COL32(70, 70, 70, 255),
            IM_COL32(40, 40, 40, 255),
            IM_COL32(40, 40, 40, 255)
        );

        draw->AddRectFilled(ImVec2(x + 1, split_y), ImVec2(x + w - 1, y + h - 1),
            IM_COL32(25, 25, 25, 255));

        draw->AddLine(ImVec2(x + 1, split_y), ImVec2(x + w - 1, split_y), IM_COL32(0, 0, 0, 200), 1.0f);

        float prevScale = window->FontWindowScale;
        float targetTextHeight = h * 0.7f;
        float baseFontSize = ImGui::GetFontSize();
        float newScale = targetTextHeight / baseFontSize;
        newScale = std::clamp(newScale, 0.5f, 3.0f);

        ImGui::SetWindowFontScale(newScale);

        float padding_x = 10.0f;
        ImVec2 text_size = ImGui::CalcTextSize(text);
        ImVec2 text_pos = ImVec2(x + w - text_size.x - padding_x, y + (h - text_size.y) * 0.5f);

        draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 150), text);
        draw->AddText(text_pos, IM_COL32(255, 255, 255, 255), text);

        ImGui::SetWindowFontScale(prevScale);
    }
};