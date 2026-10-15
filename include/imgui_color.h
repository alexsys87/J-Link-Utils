#pragma once
#include <imgui.h>

// ----------------------------------------------------------------------------
// Base color macros (IM_COL32 format)
// ----------------------------------------------------------------------------
#define IM_COL32_YELLOW        IM_COL32(255, 255, 0,   255)
#define IM_COL32_GREEN         IM_COL32(0,   255, 0,   255)
#define IM_COL32_RED           IM_COL32(255, 0,   0,   255)
#define IM_COL32_BLUE          IM_COL32(0,   0,   170, 255)
#define IM_COL32_CYAN          IM_COL32(0,   170, 170, 255)
#define IM_COL32_MAGENTA       IM_COL32(170, 0,   170, 255)
#define IM_COL32_BROWN         IM_COL32(170, 85,  0,   255)
#define IM_COL32_GRAY          IM_COL32(85,  85,  85,  255)

#define IM_COL32_ORANGE        IM_COL32(255, 165, 0,   255)
#define IM_COL32_PINK          IM_COL32(255, 192, 203, 255)
#define IM_COL32_PURPLE        IM_COL32(128, 0,   128, 255)
#define IM_COL32_TEAL          IM_COL32(0,   128, 128, 255)
#define IM_COL32_NAVY          IM_COL32(0,   0,   128, 255)
#define IM_COL32_MAROON        IM_COL32(128, 0,   0,   255)
#define IM_COL32_OLIVE         IM_COL32(128, 128, 0,   255)
#define IM_COL32_GOLD          IM_COL32(255, 215, 0,   255)
#define IM_COL32_SILVER        IM_COL32(192, 192, 192, 255)
#define IM_COL32_SKYBLUE       IM_COL32(135, 206, 235, 255)
#define IM_COL32_INDIGO        IM_COL32(75,  0,   130, 255)
#define IM_COL32_VIOLET        IM_COL32(238, 130, 238, 255)
#define IM_COL32_TURQUOISE     IM_COL32(64,  224, 208, 255)
#define IM_COL32_SALMON        IM_COL32(250, 128, 114, 255)
#define IM_COL32_CORAL         IM_COL32(255, 127, 80,  255)
#define IM_COL32_CRIMSON       IM_COL32(220, 20,  60,  255)
#define IM_COL32_KHAKI         IM_COL32(240, 230, 140, 255)
#define IM_COL32_BEIGE         IM_COL32(245, 245, 220, 255)
#define IM_COL32_MINT          IM_COL32(189, 252, 201, 255)
#define IM_COL32_LAVENDER      IM_COL32(230, 230, 250, 255)

// Bright versions
#define IM_COL32_BRIGHTBLUE    IM_COL32(85,  85,  255, 255)
#define IM_COL32_BRIGHTGREEN   IM_COL32(85,  255, 85,  255)
#define IM_COL32_BRIGHTCYAN    IM_COL32(85,  255, 255, 255)
#define IM_COL32_BRIGHTRED     IM_COL32(255, 85,  85,  255)
#define IM_COL32_BRIGHTMAGENTA IM_COL32(255, 85,  255, 255)

// Dark versions
#define IM_COL32_DARKRED       IM_COL32(139, 0,   0,   255)
#define IM_COL32_DARKGREEN     IM_COL32(0,   100, 0,   255)
#define IM_COL32_DARKBLUE      IM_COL32(0,   0,   139, 255)
#define IM_COL32_DARKCYAN      IM_COL32(0,   139, 139, 255)
#define IM_COL32_DARKMAGENTA   IM_COL32(139, 0,   139, 255)
#define IM_COL32_DARKYELLOW    IM_COL32(204, 204, 0,   255)
#define IM_COL32_DARKORANGE    IM_COL32(255, 140, 0,   255)
#define IM_COL32_DARKGRAY      IM_COL32(169, 169, 169, 255)
#define IM_COL32_LIGHTGRAY     IM_COL32(211, 211, 211, 255)

// ----------------------------------------------------------------------------
// Translucent versions (alpha = 128)
// ----------------------------------------------------------------------------
#define IM_COL32_WHITE_TRANS   IM_COL32(255, 255, 255, 128)
#define IM_COL32_RED_TRANS     IM_COL32(255, 0,   0,   128)
#define IM_COL32_GREEN_TRANS   IM_COL32(0,   255, 0,   128)
#define IM_COL32_BLUE_TRANS    IM_COL32(0,   0,   255, 128)
#define IM_COL32_YELLOW_TRANS  IM_COL32(255, 255, 0,   128)
#define IM_COL32_CYAN_TRANS    IM_COL32(0,   255, 255, 128)
#define IM_COL32_MAGENTA_TRANS IM_COL32(255, 0,   255, 128)
#define IM_COL32_GRAY_TRANS    IM_COL32(128, 128, 128, 128)
#define IM_COL32_ORANGE_TRANS  IM_COL32(255, 165, 0,   128)
#define IM_COL32_PURPLE_TRANS  IM_COL32(128, 0,   128, 128)
#define IM_COL32_TEAL_TRANS    IM_COL32(0,   128, 128, 128)

// ----------------------------------------------------------------------------
// More transparent versions (alpha = 64)
// ----------------------------------------------------------------------------
#define IM_COL32_WHITE_ALPHA64 IM_COL32(255, 255, 255, 64)
#define IM_COL32_BLACK_ALPHA64 IM_COL32(0,   0,   0,   64)
#define IM_COL32_RED_ALPHA64   IM_COL32(255, 0,   0,   64)
#define IM_COL32_GREEN_ALPHA64 IM_COL32(0,   255, 0,   64)
#define IM_COL32_BLUE_ALPHA64  IM_COL32(0,   0,   255, 64)
#define IM_COL32_GRAY_ALPHA64  IM_COL32(128, 128, 128, 64)

namespace ImGui
{
    // ------------------------------------------------------------------------
    // Colored text helpers
    // ------------------------------------------------------------------------
    const char ColorMarkerStart = '{';
    const char ColorMarkerEnd = '}';

    bool ProcessInlineHexColor(const char* start, const char* end, ImVec4& color)
    {
        const int hexCount = (int)(end - start);
        if (hexCount == 6 || hexCount == 8)
        {
            char hex[9];
            strncpy_s(hex, start, hexCount);
            hex[hexCount] = 0;

            unsigned int hexColor = 0;
            if (sscanf_s(hex, "%x", &hexColor) > 0)
            {
                color.x = static_cast<float>((hexColor & 0x00FF0000) >> 16) / 255.0f;
                color.y = static_cast<float>((hexColor & 0x0000FF00) >> 8) / 255.0f;
                color.z = static_cast<float>((hexColor & 0x000000FF)) / 255.0f;
                color.w = 1.0f;

                if (hexCount == 8)
                {
                    color.w = static_cast<float>((hexColor & 0xFF000000) >> 24) / 255.0f;
                }

                return true;
            }
        }

        return false;
    }

    void TextWithColors(const char* fmt, ...)
    {
        char tempStr[4096];

        va_list argPtr;
        va_start(argPtr, fmt);
        _vsnprintf_s(tempStr, sizeof(tempStr), fmt, argPtr);
        va_end(argPtr);
        tempStr[sizeof(tempStr) - 1] = '\0';

        bool pushedColorStyle = false;
        const char* textStart = tempStr;
        const char* textCur = tempStr;
        while (textCur < (tempStr + sizeof(tempStr)) && *textCur != '\0')
        {
            if (*textCur == ColorMarkerStart)
            {
                // Output the accumulated text
                if (textCur != textStart)
                {
                    ImGui::TextUnformatted(textStart, textCur);
                    ImGui::SameLine(0.0f, 0.0f);
                }

                // Handle the color code
                const char* colorStart = textCur + 1;
                do
                {
                    ++textCur;
                } while (*textCur != '\0' && *textCur != ColorMarkerEnd);

                // Change the color
                if (pushedColorStyle)
                {
                    ImGui::PopStyleColor();
                    pushedColorStyle = false;
                }

                ImVec4 textColor;
                if (ProcessInlineHexColor(colorStart, textCur, textColor))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                    pushedColorStyle = true;
                }

                textStart = textCur + 1;
            }
            else if (*textCur == '\n')
            {
                // Output the accumulated text and start a new line
                ImGui::TextUnformatted(textStart, textCur);
                textStart = textCur + 1;
            }

            ++textCur;
        }

        if (textCur != textStart)
        {
            ImGui::TextUnformatted(textStart, textCur);
        }
        else
        {
            ImGui::NewLine();
        }

        if (pushedColorStyle)
        {
            ImGui::PopStyleColor();
        }
    }

    // ------------------------------------------------------------------------
    // COLOR STYLES
    // ------------------------------------------------------------------------

    // ---------- Blender Dark (adapted from v3.0.0) ----------
    void StyleColorsBlender(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImVec4* colors = style->Colors;
        ImGui::StyleColorsDark(style); // base

        // Main overrides
        colors[ImGuiCol_Text] = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.19f, 0.39f, 0.69f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.20f, 0.39f, 0.69f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.28f, 0.45f, 0.70f, 1.00f);
        // Tables (matched to the palette)
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // same as WindowBg
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f); // same as Border
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // lighter than Border
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        // Other
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.28f, 0.45f, 0.70f, 0.90f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
        // Docking (if used)
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.28f, 0.45f, 0.70f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        style->WindowPadding = ImVec2(12.00f, 8.00f);
        style->ItemSpacing = ImVec2(7.00f, 3.00f);
        style->GrabMinSize = 20.00f;
        style->WindowRounding = 8.00f;
        style->FrameBorderSize = 0.00f;
        style->FrameRounding = 4.00f;
        style->GrabRounding = 12.00f;
    }

    // ---------- Cyberpunk Neon ----------
    void StyleColorsCyberpunk(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImVec4* colors = style->Colors;
        ImGui::StyleColorsDark(style);

        colors[ImGuiCol_Text] = ImVec4(0.00f, 0.82f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.36f, 0.63f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.04f, 0.12f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.03f, 0.04f, 0.22f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.06f, 0.27f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.61f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.75f, 1.00f, 0.20f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.34f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.08f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.81f, 0.95f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.61f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25f, 0.00f, 0.54f, 0.81f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.61f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.88f, 1.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.61f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.01f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.95f, 0.19f, 0.67f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.19f, 0.92f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 0.95f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.81f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.00f, 0.98f, 1.00f, 0.52f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.94f, 0.00f, 1.00f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.01f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.95f, 1.00f, 0.40f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.94f, 0.00f, 1.00f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.01f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.74f, 0.00f, 1.00f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.34f, 0.00f, 1.00f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 1.00f, 0.85f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.61f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.89f, 0.26f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.88f, 1.00f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.36f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.92f, 1.00f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.62f, 0.00f, 0.80f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 0.88f, 1.00f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.06f, 0.27f, 1.00f); // same as PopupBg
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.61f, 0.00f, 1.00f, 1.00f); // same as Border
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.40f, 0.00f, 0.70f, 1.00f); // softer
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        // Other
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.28f, 0.45f, 0.70f, 0.35f); // taken from the base theme, can be adjusted
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.95f, 0.19f, 0.92f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 1.00f, 0.88f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        // Docking
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.61f, 0.00f, 1.00f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        style->FrameBorderSize = 0.00f;
        style->WindowRounding = 0.00f;
        style->FrameRounding = 0.00f;
        style->ScrollbarRounding = 0.00f;
        style->GrabRounding = 0.00f;
    }

    // ---------- Nord (dark) ----------
    void StyleColorsNord(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImVec4* colors = style->Colors;
        ImGui::StyleColorsDark(style);

        colors[ImGuiCol_Text] = ImVec4(0.85f, 0.87f, 0.91f, 0.88f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.49f, 0.50f, 0.53f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.09f, 0.09f, 0.09f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.56f, 0.74f, 0.73f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.23f, 0.26f, 0.32f, 0.60f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.37f, 0.51f, 0.67f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.63f, 0.76f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.51f, 0.67f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.51f, 0.63f, 0.76f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.37f, 0.51f, 0.67f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.51f, 0.63f, 0.76f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.37f, 0.51f, 0.67f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.56f, 0.74f, 0.73f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.53f, 0.75f, 0.82f, 0.86f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.61f, 0.74f, 0.87f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.37f, 0.51f, 0.67f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.24f, 0.31f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.17f, 0.19f, 0.23f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.56f, 0.74f, 0.73f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.37f, 0.51f, 0.67f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.53f, 0.75f, 0.82f, 0.86f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.23f, 0.26f, 0.32f, 1.00f); // same as FrameBg
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f); // same as Border
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // medium
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        // Other
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.56f, 0.74f, 0.73f, 0.90f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        // Docking
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.53f, 0.75f, 0.82f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        style->WindowBorderSize = 1.00f;
        style->ChildBorderSize = 1.00f;
        style->PopupBorderSize = 1.00f;
        style->FrameBorderSize = 1.00f;
    }

    // ---------- Soft (light pastel) ----------
    void StyleColorsSoft()
    {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.92f, 0.91f, 0.88f, 0.70f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.92f, 0.91f, 0.88f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.84f, 0.83f, 0.80f, 0.65f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.99f, 1.00f, 0.40f, 0.78f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.47f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.90f, 0.91f, 0.00f, 0.78f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 1.00f, 0.00f, 0.80f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.99f, 1.00f, 0.22f, 0.86f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.25f, 1.00f, 0.00f, 0.76f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.86f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Tab] = colors[ImGuiCol_Header];
        colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive] = colors[ImGuiCol_HeaderActive];
        colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
        colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabActive];
        colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
        // Tables (matched to the light theme)
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.84f, 0.83f, 0.80f, 0.80f); // light grey
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 0.00f, 0.00f, 0.05f);
        // Docking (kept commented out for reference)
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        style->WindowPadding = ImVec2(15, 15);
        style->WindowRounding = 5.0f;
        style->FramePadding = ImVec2(5, 5);
        style->FrameRounding = 4.0f;
        style->ItemSpacing = ImVec2(12, 8);
        style->ItemInnerSpacing = ImVec2(8, 6);
        style->IndentSpacing = 25.0f;
        style->ScrollbarSize = 15.0f;
        style->ScrollbarRounding = 9.0f;
        style->GrabMinSize = 5.0f;
        style->GrabRounding = 3.0f;
    }

    // ---------- Black (pure black) ----------
    void StyleColorsBlack()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
        colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
        colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        // Docking
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        // style.GrabRounding; // this line does nothing and can be removed
    }

    // ---------- Green (green accents) ----------
    void StyleColorsGreen()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.60f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.57f, 0.57f, 0.57f, 0.70f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.80f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
        colors[ImGuiCol_Button] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
        colors[ImGuiCol_Header] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
        colors[ImGuiCol_Separator] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
        colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.54f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        // Docking
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.13f, 0.75f, 0.55f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    }

    // ---------- Red (red accents) ----------
    void StyleColorsRed()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;

        colors[ImGuiCol_Text] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.37f, 0.14f, 0.14f, 0.67f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.39f, 0.20f, 0.20f, 0.67f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.48f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.48f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.56f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.19f, 0.19f, 0.40f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.89f, 0.00f, 0.19f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(1.00f, 0.19f, 0.19f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.17f, 0.00f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.89f, 0.00f, 0.19f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.33f, 0.35f, 0.36f, 0.53f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.76f, 0.28f, 0.44f, 0.67f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.47f, 0.47f, 0.47f, 0.67f);
        colors[ImGuiCol_Separator] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
        colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.07f, 0.07f, 0.51f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.86f, 0.23f, 0.43f, 0.67f);
        colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.19f, 0.19f, 0.57f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.13f, 0.13f, 0.74f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        // Docking
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.86f, 0.23f, 0.43f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    }

    // ---------- Embrace (dark, low contrast) ----------
    void StyleColorsEmbrace()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
        // Docking
        //colors[ImGuiCol_DockingPreview]             = ImVec4(0.33f, 0.67f, 0.86f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8.00f, 8.00f);
        style.FramePadding = ImVec2(5.00f, 2.00f);
        style.CellPadding = ImVec2(6.00f, 6.00f);
        style.ItemSpacing = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
        style.IndentSpacing = 25;
        style.ScrollbarSize = 15;
        style.GrabMinSize = 10;
        style.WindowBorderSize = 1;
        style.ChildBorderSize = 1;
        style.PopupBorderSize = 1;
        style.FrameBorderSize = 1;
        style.TabBorderSize = 1;
        style.WindowRounding = 7;
        style.ChildRounding = 4;
        style.FrameRounding = 3;
        style.PopupRounding = 4;
        style.ScrollbarRounding = 9;
        style.GrabRounding = 3;
        style.LogSliderDeadzone = 4;
        style.TabRounding = 4;
    }

    // ---------- Dracula (dark purple) ----------
    void StyleColorsDracula()
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
        colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

        colors[ImGuiCol_Border] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
        colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.24f };

        colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
        colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };

        colors[ImGuiCol_Header] = ImVec4{ 0.13f, 0.13f, 0.17f, 1.0f };
        colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
        colors[ImGuiCol_HeaderActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

        colors[ImGuiCol_Button] = ImVec4{ 0.13f, 0.13f, 0.17f, 1.0f };
        colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
        colors[ImGuiCol_ButtonActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_CheckMark] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };

        colors[ImGuiCol_PopupBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 0.92f };

        colors[ImGuiCol_SliderGrab] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.54f };
        colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.54f };

        colors[ImGuiCol_FrameBg] = ImVec4{ 0.13f, 0.13f, 0.17f, 1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
        colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

        colors[ImGuiCol_Tab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_TabHovered] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };
        colors[ImGuiCol_TabActive] = ImVec4{ 0.2f, 0.22f, 0.27f, 1.0f };
        colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

        colors[ImGuiCol_TitleBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

        colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
        colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };

        colors[ImGuiCol_Separator] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };
        colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };
        colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 1.0f };

        colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
        colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.29f };
        colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 0.29f };

        // Tables (matched to the theme)
        colors[ImGuiCol_TableHeaderBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
        colors[ImGuiCol_TableBorderStrong] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.50f };
        colors[ImGuiCol_TableBorderLight] = ImVec4{ 0.30f, 0.30f, 0.40f, 0.50f };
        colors[ImGuiCol_TableRowBg] = ImVec4{ 0.00f, 0.00f, 0.00f, 0.00f };
        colors[ImGuiCol_TableRowBgAlt] = ImVec4{ 1.00f, 1.00f, 1.00f, 0.06f };
        // Other
        colors[ImGuiCol_TextSelectedBg] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.35f };
        colors[ImGuiCol_DragDropTarget] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.90f };
        colors[ImGuiCol_NavHighlight] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.00f };
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4{ 1.00f, 1.00f, 1.00f, 0.70f };
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4{ 0.80f, 0.80f, 0.80f, 0.20f };
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4{ 0.80f, 0.80f, 0.80f, 0.35f };
        // Docking
        //colors[ImGuiCol_DockingPreview]   = ImVec4{ 0.44f, 0.37f, 0.61f, 0.70f };
        //colors[ImGuiCol_DockingEmptyBg]   = ImVec4{ 0.20f, 0.20f, 0.20f, 1.00f };

        auto& style = ImGui::GetStyle();
        style.TabRounding = 4;
        style.ScrollbarRounding = 9;
        style.WindowRounding = 7;
        style.GrabRounding = 3;
        style.FrameRounding = 3;
        style.PopupRounding = 4;
        style.ChildRounding = 4;
    }

    // ==================== MORE STYLES ====================

    // ---------- Visual Studio Dark ----------
    void StyleColorsVisualStudio(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.26f, 0.33f, 0.44f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.33f, 0.42f, 0.55f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.44f, 0.54f, 0.67f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.26f, 0.33f, 0.44f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.42f, 0.55f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.54f, 0.67f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.68f, 0.96f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.68f, 0.96f, 0.70f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.44f, 0.68f, 0.96f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.33f, 0.44f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        // Other
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.44f, 0.68f, 0.96f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.44f, 0.68f, 0.96f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.44f, 0.68f, 0.96f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        // Docking (optional)
        //colors[ImGuiCol_DockingPreview]         = ImVec4(0.44f, 0.68f, 0.96f, 0.70f);
        //colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    }

    // ---------- One Dark (Atom) ----------
    void StyleColorsOneDark(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.33f, 0.45f, 0.68f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.39f, 0.52f, 0.78f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.44f, 0.58f, 0.86f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.33f, 0.45f, 0.68f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.39f, 0.52f, 0.78f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.58f, 0.86f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.21f, 0.24f, 0.29f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.73f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.65f, 0.73f, 1.00f, 0.70f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.65f, 0.73f, 1.00f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.45f, 0.68f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.28f, 0.33f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.12f, 0.14f, 0.17f, 0.50f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        // Other
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.65f, 0.73f, 1.00f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.65f, 0.73f, 1.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.65f, 0.73f, 1.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    // ---------- Light Classic ----------
    void StyleColorsLightClassic(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsLight(style);
        ImVec4* colors = style->Colors;

        // Adjustments
        colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.80f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.40f, 0.80f, 0.70f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        // Tables (kept from the base light theme, can be adjusted)
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        // The other colors come from StyleColorsLight
    }

    // ---------- Cherry ----------
    void StyleColorsCherry(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.08f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.10f, 0.16f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.08f, 0.11f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.55f, 0.16f, 0.24f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.65f, 0.20f, 0.30f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.75f, 0.24f, 0.36f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.55f, 0.16f, 0.24f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.65f, 0.20f, 0.30f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.24f, 0.36f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.16f, 0.18f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.36f, 0.20f, 0.26f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.40f, 0.50f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.90f, 0.40f, 0.50f, 0.70f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.40f, 0.50f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.55f, 0.16f, 0.24f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.32f, 0.14f, 0.20f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.32f, 0.14f, 0.20f, 1.00f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.24f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.40f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.30f, 0.15f, 0.20f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        // Other
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.90f, 0.40f, 0.50f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.90f, 0.40f, 0.50f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.90f, 0.40f, 0.50f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    // ---------- Nord Light (light version of Nord) ----------
    void StyleColorsNordLight(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsLight(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // nord0
        colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.97f, 0.98f, 1.00f); // nord6
        colors[ImGuiCol_TitleBg] = ImVec4(0.82f, 0.85f, 0.89f, 1.00f); // nord4
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.76f, 0.80f, 0.85f, 1.00f); // nord3
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.89f, 0.91f, 0.94f, 1.00f); // nord5
        colors[ImGuiCol_Header] = ImVec4(0.56f, 0.63f, 0.71f, 0.50f); // nord2, translucent
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.46f, 0.55f, 0.64f, 0.70f); // nord1
        colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // nord0
        colors[ImGuiCol_Button] = ImVec4(0.76f, 0.80f, 0.85f, 1.00f); // nord3
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.66f, 0.71f, 0.78f, 1.00f); // nord2
        colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.63f, 0.71f, 1.00f); // nord2, brighter
        colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.89f, 0.91f, 0.94f, 1.00f); // nord5
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.82f, 0.85f, 0.89f, 1.00f); // nord4
        colors[ImGuiCol_CheckMark] = ImVec4(0.34f, 0.43f, 0.53f, 1.00f); // nord0, slightly lighter
        colors[ImGuiCol_SliderGrab] = ImVec4(0.46f, 0.55f, 0.64f, 0.70f); // nord1
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // nord0
        colors[ImGuiCol_Tab] = ImVec4(0.89f, 0.91f, 0.94f, 1.00f); // nord5
        colors[ImGuiCol_TabHovered] = ImVec4(0.76f, 0.80f, 0.85f, 1.00f); // nord3
        colors[ImGuiCol_TabActive] = ImVec4(0.96f, 0.97f, 0.98f, 1.00f); // nord6
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.89f, 0.91f, 0.94f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.96f, 0.97f, 0.98f, 1.00f);
        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.82f, 0.85f, 0.89f, 1.00f); // nord4
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.76f, 0.80f, 0.85f, 1.00f); // nord3
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.89f, 0.91f, 0.94f, 1.00f); // nord5
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 0.00f, 0.00f, 0.05f);
        // Other (kept from the base light theme, can be adjusted)
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.36f, 0.46f, 0.56f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.55f, 0.64f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.46f, 0.55f, 0.64f, 1.00f);
    }

    // ---------- Matrix (green neon) ----------
    void StyleColorsMatrix(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.00f, 0.80f, 0.00f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.20f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.00f, 0.40f, 0.00f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.60f, 0.00f, 0.67f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.20f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.40f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.10f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.15f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.80f, 0.00f, 0.60f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.00f, 0.30f, 0.00f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.40f, 0.00f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.60f, 0.00f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.00f, 0.50f, 0.00f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.00f, 0.70f, 0.00f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.40f, 0.00f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.60f, 0.00f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.80f, 0.00f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.20f, 0.00f, 0.80f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.50f, 0.00f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.30f, 0.00f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.15f, 0.00f, 0.80f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.25f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.20f, 0.00f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.50f, 0.00f, 0.50f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.00f, 0.30f, 0.00f, 0.30f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 0.10f, 0.00f, 0.10f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.60f, 0.00f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.00f, 1.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.20f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.20f, 0.00f, 0.35f);
    }

    // ---------- High Contrast ----------
    void StyleColorsHighContrast(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.50f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.80f, 0.80f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.40f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.60f, 0.60f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 1.00f, 0.00f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 0.00f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 0.00f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.60f, 0.60f, 0.60f, 0.60f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.80f, 0.80f, 0.00f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    // ==================== 4 POPULAR STYLES ====================

    // ---------- Adobe Spectrum ----------
    void StyleColorsAdobeSpectrum(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        // Adobe brand blue #1473E6
        const ImVec4 adobeBlue = ImVec4(20.0f / 255.0f, 115.0f / 255.0f, 230.0f / 255.0f, 1.0f);
        const ImVec4 adobeBlueHover = ImVec4(0.2f, 0.5f, 0.9f, 1.0f);
        const ImVec4 adobeBlueActive = ImVec4(0.1f, 0.4f, 0.8f, 1.0f);

        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.50f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_CheckMark] = adobeBlue;
        colors[ImGuiCol_SliderGrab] = adobeBlue;
        colors[ImGuiCol_SliderGrabActive] = adobeBlueActive;
        colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = adobeBlueHover;
        colors[ImGuiCol_ButtonActive] = adobeBlueActive;
        colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
        colors[ImGuiCol_HeaderHovered] = adobeBlueHover;
        colors[ImGuiCol_HeaderActive] = adobeBlueActive;
        colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = adobeBlueHover;
        colors[ImGuiCol_SeparatorActive] = adobeBlueActive;
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = adobeBlueHover;
        colors[ImGuiCol_ResizeGripActive] = adobeBlueActive;
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabHovered] = adobeBlueHover;
        colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotLines] = adobeBlue;
        colors[ImGuiCol_PlotLinesHovered] = adobeBlueHover;
        colors[ImGuiCol_PlotHistogram] = adobeBlue;
        colors[ImGuiCol_PlotHistogramHovered] = adobeBlueHover;
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(adobeBlue.x, adobeBlue.y, adobeBlue.z, 0.35f);
        colors[ImGuiCol_DragDropTarget] = adobeBlue;
        colors[ImGuiCol_NavHighlight] = adobeBlue;
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    // ---------- Doug Binks Dark (elegant dark with transparency) ----------
    void StyleColorsDougBinksDark(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImVec4* colors = style->Colors;
        ImGui::StyleColorsDark(style); // base

        // Translucent windows
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 0.60f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.25f, 0.80f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.32f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    }

    // ---------- Enemy Mouse (retro synth) ----------
    void StyleColorsEnemyMouse(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 0.85f, 0.70f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.50f, 0.40f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.08f, 0.06f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.10f, 0.08f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.06f, 0.04f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.80f, 0.60f, 0.40f, 0.50f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.15f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.20f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.25f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.10f, 0.05f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.15f, 0.05f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.08f, 0.04f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.60f, 0.20f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.50f, 0.00f, 0.70f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.30f, 0.20f, 0.10f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.30f, 0.10f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.70f, 0.40f, 0.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.40f, 0.25f, 0.10f, 0.70f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.35f, 0.10f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.80f, 0.45f, 0.00f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.60f, 0.40f, 0.20f, 0.50f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.60f, 0.40f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.70f, 0.20f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.80f, 0.00f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.12f, 0.06f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.25f, 0.00f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.18f, 0.08f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.12f, 0.06f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.60f, 0.40f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.40f, 0.25f, 0.10f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);
    }

    // ---------- LED Synth Master (neon synth) ----------
    void StyleColorsLEDSynthMaster(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImGui::StyleColorsDark(style);
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.95f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.20f, 0.80f, 1.00f, 0.50f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.00f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.00f, 0.20f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.20f, 0.20f, 0.70f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.10f, 0.10f, 0.20f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 1.00f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.50f, 1.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.40f, 0.80f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.40f, 1.00f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.00f, 1.00f, 1.00f, 0.50f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 0.00f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.00f, 1.00f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.00f, 0.10f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.50f, 1.00f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.00f, 0.30f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.10f, 0.00f, 0.10f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.30f, 0.30f, 0.60f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.35f);
    }
} // namespace ImGui