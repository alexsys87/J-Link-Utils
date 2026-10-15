// GlossyButtonWidget.h
#pragma once
#include "widget_common.h"

// Режимы работы кнопки
enum GlossyButtonMode {
    GLOSSY_BUTTON_SETBIT = 0,   // записать заданное значение
    GLOSSY_BUTTON_TOGGLE = 1,   // инвертировать младший бит
    GLOSSY_BUTTON_WRITE = 2,    // записать фиксированное значение
    GLOSSY_BUTTON_MOMENTARY = 3, // при удержании пишется writeValue, при отпускании 0
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
    uint16_t    m_addr;          // статический адрес (может быть 0)
    bool        m_le;            // порядок байт (не используется для boolean, но оставлен)
    GlossyButtonMode m_mode;
    uint16_t    m_writeValue;
    float       m_radius;
    ImU32       m_onColor;
    ImU32       m_offColor;

    void DrawButton(ImDrawList* draw, ImVec2 center, float radius,
        bool state, bool hovered, bool held, bool momentaryActive) const;
};