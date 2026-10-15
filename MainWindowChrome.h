#pragma once

#include <SDL.h>
#include <imgui.h>

// ImGui owns the appearance, SDL/the window manager own the native window.
// Cached logical rectangles keep the OS callback independent of ImGui's frame.
struct MainWindowChrome {
    SDL_FRect dragArea{};
    bool blocked = false;
    float resizeBorder = 5.0f;
};

enum class MainWindowAction { None, Minimize, ToggleMaximize, Close };

// Must run before SDL_CreateWindow. SDL2 otherwise omits WS_THICKFRAME and
// WS_MAXIMIZEBOX for BORDERLESS windows, despite SDL_WINDOW_RESIZABLE. Its
// Win32 backend can then consider the window non-resizable and ignore maximize.
// WINDOWED_STYLE also keeps maximization within the monitor's work area rather
// than treating a desktop-sized borderless window as fullscreen. SDL continues
// to remove the native non-client frame; ImGui still draws the entire title bar.
inline void ConfigureMainWindowChrome()
{
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
}

inline SDL_HitTestResult SDLCALL MainWindowHitTest(SDL_Window* window, const SDL_Point* point, void* data)
{
    const auto* chrome = static_cast<const MainWindowChrome*>(data);
    if (!chrome || chrome->blocked) return SDL_HITTEST_NORMAL;
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    if (point->x < 0 || point->y < 0 || point->x >= width || point->y >= height)
        return SDL_HITTEST_NORMAL;
    const Uint32 flags = SDL_GetWindowFlags(window);
    if ((flags & SDL_WINDOW_RESIZABLE) && !(flags & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN))) {
        const float border = chrome->resizeBorder;
        const bool left = point->x < border, right = point->x >= width - border;
        const bool top = point->y < border, bottom = point->y >= height - border;
        if (top && left) return SDL_HITTEST_RESIZE_TOPLEFT;
        if (top && right) return SDL_HITTEST_RESIZE_TOPRIGHT;
        if (bottom && left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        if (left) return SDL_HITTEST_RESIZE_LEFT;
        if (right) return SDL_HITTEST_RESIZE_RIGHT;
        if (top) return SDL_HITTEST_RESIZE_TOP;
        if (bottom) return SDL_HITTEST_RESIZE_BOTTOM;
    }
    const SDL_FRect& r = chrome->dragArea;
    if (point->x >= r.x && point->x < r.x + r.w && point->y >= r.y && point->y < r.y + r.h)
        return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}

inline MainWindowAction DrawMainWindowTitleBar(SDL_Window* window, const char* title, MainWindowChrome& chrome)
{
    const ImVec2 pos = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float height = ImGui::GetFrameHeight();
    const float buttonWidth = height * 1.6f;
    const float buttonsX = pos.x + size.x - buttonWidth * 3.0f;
    chrome.blocked = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    chrome.dragArea = {0.0f, 0.0f, size.x - buttonWidth * 3.0f, height};
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    // The regular content clip excludes the title bar. Temporarily use its rect
    // for both ImGui hit testing and rendering, then restore the content cursor.
    ImGui::PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + height), false);
    draw->PushClipRect(pos, ImVec2(buttonsX - style.FramePadding.x, pos.y + height), true);
    draw->AddText(ImVec2(pos.x + style.FramePadding.x, pos.y + style.FramePadding.y),
        ImGui::GetColorU32(ImGuiCol_Text), title);
    draw->PopClipRect();
    MainWindowAction action = MainWindowAction::None;
    const char* ids[] = {"##NativeMinimize", "##NativeMaximize", "##NativeClose"};
    for (int i = 0; i < 3; ++i) {
        const ImVec2 a(buttonsX + buttonWidth * i, pos.y);
        const ImVec2 b(a.x + buttonWidth, a.y + height);
        ImGui::SetCursorScreenPos(a);
        if (ImGui::InvisibleButton(ids[i], ImVec2(buttonWidth, height)))
            action = i == 0 ? MainWindowAction::Minimize : i == 1 ? MainWindowAction::ToggleMaximize : MainWindowAction::Close;
        if (ImGui::IsItemHovered())
            draw->AddRectFilled(a, b, ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered));
        const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
        const float radius = height * 0.18f;
        const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
        if (i == 0) {
            draw->AddLine(ImVec2(center.x - radius, center.y), ImVec2(center.x + radius, center.y), color);
        } else if (i == 1) {
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) {
                draw->AddRect(ImVec2(center.x - radius + 2, center.y - radius), ImVec2(center.x + radius, center.y + radius - 2), color);
                draw->AddRect(ImVec2(center.x - radius, center.y - radius + 2), ImVec2(center.x + radius - 2, center.y + radius), color);
            } else {
                draw->AddRect(ImVec2(center.x - radius, center.y - radius), ImVec2(center.x + radius, center.y + radius), color);
            }
        } else {
            draw->AddLine(ImVec2(center.x - radius, center.y - radius), ImVec2(center.x + radius, center.y + radius), color);
            draw->AddLine(ImVec2(center.x - radius, center.y + radius), ImVec2(center.x + radius, center.y - radius), color);
        }
    }
    ImGui::PopClipRect();
    ImGui::SetCursorScreenPos(cursor);
    return action;
}

inline void ApplyMainWindowAction(SDL_Window* window, MainWindowAction action, bool& closing)
{
    switch (action) {
    case MainWindowAction::Minimize: SDL_MinimizeWindow(window); break;
    case MainWindowAction::ToggleMaximize:
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) SDL_RestoreWindow(window);
        else SDL_MaximizeWindow(window);
        break;
    case MainWindowAction::Close: closing = true; break;
    default: break;
    }
}
