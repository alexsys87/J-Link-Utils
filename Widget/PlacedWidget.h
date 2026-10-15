// PlacedWidget.h
// Wraps every widget the loader creates and adds what all widgets share: an optional
// position in the window (<PosX>/<PosY>), the width of standard input fields (<ItemWidth>),
// and in design mode the frame that lets the mouse select, move and resize the widget.
#pragma once
#include "widget_common.h"
#include "DesignMode.h"

struct WidgetLayout {
    bool placeable = false;   // the widget has a place of its own (not SameLine, Tooltip...)
    bool absolute = false;    // <PosX>/<PosY> given: drawn there instead of in the flow layout
    ImVec2 pos;               // window coordinates, as ImGui::SetCursorPos() takes them
    float itemWidth = 0.0f;   // ImGui::PushItemWidth() value; 0 = default
};

class PlacedWidget : public UIWidget {
    std::unique_ptr<UIWidget> inner;
    const tinyxml2::XMLElement* key;   // element in the open document (design mode); never dereferenced
    WidgetLayout layout;
    bool container;                    // holds child widgets

    // Line state of the window. It is restored after an absolutely placed widget, so the
    // widgets around it keep the layout they would have without it.
    struct LineState {
        ImVec2 cursor, prevLine, currLineSize, prevLineSize;
        float currBaseOffset, prevBaseOffset;
        bool sameLine, setPos;
    };

    static LineState SaveLine(const ImGuiWindow* w) {
        const ImGuiWindowTempData& dc = w->DC;
        return { dc.CursorPos, dc.CursorPosPrevLine, dc.CurrLineSize, dc.PrevLineSize,
                 dc.CurrLineTextBaseOffset, dc.PrevLineTextBaseOffset, dc.IsSameLine, dc.IsSetPos };
    }

    // CursorMaxPos keeps the widget, so the window still scrolls to it.
    static void RestoreLine(ImGuiWindow* w, const LineState& s) {
        ImGuiWindowTempData& dc = w->DC;
        dc.CursorPos = s.cursor;
        dc.CursorPosPrevLine = s.prevLine;
        dc.CurrLineSize = s.currLineSize;
        dc.PrevLineSize = s.prevLineSize;
        dc.CurrLineTextBaseOffset = s.currBaseOffset;
        dc.PrevLineTextBaseOffset = s.prevBaseOffset;
        dc.IsSameLine = s.sameLine;
        dc.IsSetPos = s.setPos;
    }

public:
    PlacedWidget(std::unique_ptr<UIWidget> widget, const tinyxml2::XMLElement* source, const WidgetLayout& l)
        : inner(std::move(widget)), key(source), layout(l),
          container(dynamic_cast<ContainerWidget*>(inner.get()) != nullptr) {}

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const bool design = key && DesignMode::IsActive();
        // A Grid draws its children once per cell (with a cell address): they stay in its layout.
        const bool repeated = dynamicAddr != nullptr;
        const bool movable = layout.placeable && !repeated;

        ImVec2 pos = layout.pos;
        bool absolute = layout.absolute && movable;
        if (design && movable && DesignMode::DraggedPosition(key, &pos)) absolute = true;

        LineState line{};
        if (absolute) {
            line = SaveLine(window);
            ImGui::SetCursorPos(pos);
        }
        if (layout.itemWidth != 0.0f) ImGui::PushItemWidth(layout.itemWidth);

        if (!design) {
            inner->Draw(ctx, dynamicAddr);
        }
        else {
            // Measure the widget alone: CursorMaxPos grows with every item it submits.
            const ImVec2 start = window->DC.CursorPos;
            const ImVec2 maxBefore = window->DC.CursorMaxPos;
            window->DC.CursorMaxPos = start;
            const float itemWidth = ImGui::CalcItemWidth();
            DesignMode::BeginWidget(container);
            inner->Draw(ctx, dynamicAddr);
            const ImRect rect(start, window->DC.CursorMaxPos);
            DesignMode::EndWidget(container, key, window, rect, movable, itemWidth);
            window->DC.CursorMaxPos = ImMax(maxBefore, window->DC.CursorMaxPos);
        }

        if (layout.itemWidth != 0.0f) ImGui::PopItemWidth();
        if (absolute) RestoreLine(window, line);
    }
};
