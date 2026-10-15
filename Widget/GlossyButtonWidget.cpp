// GlossyButtonWidget.cpp
#include "GlossyButtonWidget.h"
#include "imgui_internal.h"
#include <cmath>

void GlossyButtonWidget::DrawButton(ImDrawList* draw, ImVec2 center, float radius,
    bool state, bool hovered, bool held, bool momentaryActive) const {
    // Base color for the state
    ImU32 baseColor = state ? m_onColor : m_offColor;

    // Adjust for hover / press
    if (hovered || momentaryActive) {
        ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(baseColor);
        col4.x *= 1.2f; col4.y *= 1.2f; col4.z *= 1.2f;
        col4.x = (col4.x > 1.0f) ? 1.0f : col4.x;
        col4.y = (col4.y > 1.0f) ? 1.0f : col4.y;
        col4.z = (col4.z > 1.0f) ? 1.0f : col4.z;
        baseColor = ImGui::ColorConvertFloat4ToU32(col4);
    }
    if (held) {
        ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(baseColor);
        col4.x *= 0.8f; col4.y *= 0.8f; col4.z *= 0.8f;
        baseColor = ImGui::ColorConvertFloat4ToU32(col4);
    }

    // Shadow
    draw->AddCircleFilled(ImVec2(center.x + 2, center.y + 2), radius,
        IM_COL32(0, 0, 0, 80), 64);

    // Metal rim
    draw->AddCircleFilled(center, radius, IM_COL32(180, 180, 190, 255), 64);
    draw->AddCircle(center, radius, IM_COL32(80, 80, 90, 255), 64, 1.5f);

    // Grey ring, 5% of the radius thick
    float ringWidth = radius * 0.45f;
    draw->AddCircle(center, radius, IM_COL32(128, 128, 128, 200), 64, ringWidth);

    // Main colored circle (slightly smaller than the radius)
    float innerRadius = radius - 2.0f;
    draw->AddCircleFilled(center, innerRadius, baseColor, 64);

    // Highlight
    draw->AddCircleFilled(ImVec2(center.x, center.y - radius * 0.25f),
        radius * 0.5f, IM_COL32(255, 255, 255, 40), 32);

    // Text (ON/OFF)
    const char* label = state ? "ON" : "OFF";
    ImVec2 textSize = ImGui::CalcTextSize(label);
    // Text shadow
    draw->AddText(ImVec2(center.x - textSize.x * 0.5f + 1, center.y - textSize.y * 0.5f + 1),
        IM_COL32(0, 0, 0, 100), label);
    // Main text
    draw->AddText(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
        IM_COL32(255, 255, 255, 255), label);
}

GlossyButtonWidget::GlossyButtonWidget(const char* label, uint32_t addr, bool le,
    GlossyButtonMode mode, uint16_t writeValue,
    float radius, ImU32 onColor, ImU32 offColor)
    : m_label(label ? label : "")
    , m_addr(addr)
    , m_le(le)
    , m_mode(mode)
    , m_writeValue(writeValue)
    , m_radius(radius)
    , m_onColor(onColor)
    , m_offColor(offColor) {
}

void GlossyButtonWidget::Draw(MainContext* ctx, uint32_t* dynamicAddr) {
    uint32_t addr = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : m_addr;

    // Always read the value (the memory functions handle address 0)
    uint16_t currentValue = (uint16_t)ReadTypedValue(ctx, addr, U16_DEC, m_le);

    bool state = false;
    if (m_mode == GLOSSY_BUTTON_TOGGLE) {
        state = (currentValue & 1) != 0;
    }
    else {
        state = (currentValue != 0);
    }

    ImGui::PushID(addr);

    bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    if (disabled) ImGui::BeginDisabled();

    ImVec2 size(m_radius * 2.0f, m_radius * 2.0f);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(bb);
    if (ImGui::ItemAdd(bb, ImGui::GetID("##btn"))) {
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, ImGui::GetID("##btn"), &hovered, &held);

        bool writeNow = false;
        uint16_t valueToWrite = 0;

        if (m_mode == GLOSSY_BUTTON_SETBIT || m_mode == GLOSSY_BUTTON_WRITE) {
            if (pressed && !disabled) {
                valueToWrite = m_writeValue;
                writeNow = true;
            }
        }
        else if (m_mode == GLOSSY_BUTTON_TOGGLE) {
            if (pressed && !disabled) {
                valueToWrite = currentValue ^ 1;
                writeNow = true;
            }
        }
        else if (m_mode == GLOSSY_BUTTON_MOMENTARY) {
            if (held && !disabled) {
                valueToWrite = m_writeValue;
                writeNow = true;
            }
            else if (ImGui::IsItemDeactivated() && !disabled) {
                valueToWrite = 0;
                writeNow = true;
            }
        }

        if (writeNow) {
            WriteTypedValue(ctx, addr, U16_DEC, m_le, valueToWrite);
            // Update the displayed state
            if (m_mode == GLOSSY_BUTTON_TOGGLE) {
                state = (valueToWrite & 1) != 0;
            }
            else {
                state = (valueToWrite != 0);
            }
        }

        bool momentaryActive = (m_mode == GLOSSY_BUTTON_MOMENTARY && held && !disabled);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        DrawButton(draw, bb.GetCenter(), m_radius,
            state || momentaryActive,
            hovered, held, momentaryActive);
    }

    if (disabled) ImGui::EndDisabled();
    ImGui::PopID();
}