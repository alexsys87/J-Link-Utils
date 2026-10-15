#pragma once
#include "widget_common.h"

class SevenSegmentWidget : public UIWidget {
    uint32_t address;
    ImVec2 size;
    bool littleEndian;
    std::string label;   // reserved, not drawn

public:
    SevenSegmentWidget(const char* lbl, uint32_t addr, float w, float h, bool le)
        : address(addr), size(w, h), littleEndian(le), label(lbl ? lbl : "") {
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : address;
        // Read a 16-bit value
        uint16_t raw = (uint16_t)ReadTypedValue(ctx, addr, U16_DEC, littleEndian);
        int value = static_cast<int>(raw);
        if (value < 0) value = 0;
        if (value > 9999) value = 9999;

        ImGui::PushID(addr);
        SevenSegment4(label.c_str(), value, size);
        ImGui::PopID();
    }

private:
    // Seven-segment display implementation (copied with small changes)
    static void SevenSegment4(const char* /*id*/, int value, ImVec2 size) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        float scale = ImGui::GetIO().FontGlobalScale;
        if (size.x == 0) size.x = 160 * scale;
        if (size.y == 0) size.y = 48 * scale;

        float digitWidth = size.x / 4.2f;
        float digitHeight = size.y;
        float thickness = digitWidth * 0.15f;
        ImU32 colorOn = IM_COL32(255, 80, 80, 255);   // bright red
        ImU32 colorOff = IM_COL32(40, 0, 0, 255);      // dark red

        static const bool segments[10][7] = {
            {1,1,1,1,1,1,0}, // 0
            {0,1,1,0,0,0,0}, // 1
            {1,1,0,1,1,0,1}, // 2
            {1,1,1,1,0,0,1}, // 3
            {0,1,1,0,0,1,1}, // 4
            {1,0,1,1,0,1,1}, // 5
            {1,0,1,1,1,1,1}, // 6
            {1,1,1,0,0,0,0}, // 7
            {1,1,1,1,1,1,1}, // 8
            {1,1,1,1,0,1,1}  // 9
        };

        int clamped = value;
        if (clamped < 0) clamped = 0;
        if (clamped > 9999) clamped = 9999;
        int digits[4] = {
            clamped / 1000,
            (clamped / 100) % 10,
            (clamped / 10) % 10,
            clamped % 10
        };

        for (int i = 0; i < 4; ++i) {
            int d = digits[i];
            float x0 = pos.x + i * (digitWidth + 2);
            float y0 = pos.y;
            float w = digitWidth;
            float h = digitHeight;

            // Segment a (top horizontal)
            draw->AddRectFilled(
                ImVec2(x0 + thickness * 0.5f, y0),
                ImVec2(x0 + w - thickness * 0.5f, y0 + thickness),
                segments[d][0] ? colorOn : colorOff);

            // Segment b (top right vertical)
            draw->AddRectFilled(
                ImVec2(x0 + w - thickness, y0 + thickness * 0.5f),
                ImVec2(x0 + w, y0 + h / 2 - thickness * 0.5f),
                segments[d][1] ? colorOn : colorOff);

            // Segment c (bottom right vertical)
            draw->AddRectFilled(
                ImVec2(x0 + w - thickness, y0 + h / 2 + thickness * 0.5f),
                ImVec2(x0 + w, y0 + h - thickness * 0.5f),
                segments[d][2] ? colorOn : colorOff);

            // Segment d (bottom horizontal)
            draw->AddRectFilled(
                ImVec2(x0 + thickness * 0.5f, y0 + h - thickness),
                ImVec2(x0 + w - thickness * 0.5f, y0 + h),
                segments[d][3] ? colorOn : colorOff);

            // Segment e (bottom left vertical)
            draw->AddRectFilled(
                ImVec2(x0, y0 + h / 2 + thickness * 0.5f),
                ImVec2(x0 + thickness, y0 + h - thickness * 0.5f),
                segments[d][4] ? colorOn : colorOff);

            // Segment f (top left vertical)
            draw->AddRectFilled(
                ImVec2(x0, y0 + thickness * 0.5f),
                ImVec2(x0 + thickness, y0 + h / 2 - thickness * 0.5f),
                segments[d][5] ? colorOn : colorOff);

            // Segment g (middle horizontal)
            draw->AddRectFilled(
                ImVec2(x0 + thickness * 0.5f, y0 + h / 2 - thickness * 0.5f),
                ImVec2(x0 + w - thickness * 0.5f, y0 + h / 2 + thickness * 0.5f),
                segments[d][6] ? colorOn : colorOff);
        }

        ImGui::Dummy(size);
    }
};