// GlossyButtonWidget.h
#pragma once
#include "widget_common.h"

// Button modes
enum GlossyButtonMode {
    GLOSSY_BUTTON_SETBIT = 0,   // write the given value
    GLOSSY_BUTTON_TOGGLE = 1,   // invert the lowest bit
    GLOSSY_BUTTON_WRITE = 2,    // write a fixed value
    GLOSSY_BUTTON_MOMENTARY = 3, // writeValue while held, 0 on release
};

class GlossyButtonWidget : public UIWidget {
public:
    GlossyButtonWidget(const char* label, uint32_t addr = 0, bool le = false,
        GlossyButtonMode mode = GLOSSY_BUTTON_SETBIT,
        uint16_t writeValue = 1, float radius = 20.0f,
        ImU32 onColor = IM_COL32(20, 120, 60, 255),
        ImU32 offColor = IM_COL32(150, 30, 30, 255));
    void Draw(MainContext* ctx, uint32_t* dynamicAddr = nullptr) override;

private:
    std::string m_label;
    uint32_t    m_addr;          // static address (may be 0)
    bool        m_le;            // byte order (unused for boolean, kept for compatibility)
    GlossyButtonMode m_mode;
    uint16_t    m_writeValue;
    float       m_radius;
    ImU32       m_onColor;
    ImU32       m_offColor;

    void DrawButton(ImDrawList* draw, ImVec2 center, float radius,
        bool state, bool hovered, bool held, bool momentaryActive) const;
};