// DesignMode.h
// Design mode: the widgets of the open document are edited with the mouse. A click selects a
// widget or a window, dragging moves the widget (it gets <PosX>/<PosY>), the handles resize it,
// and the "Properties" window edits every property of the selection. Changes go into
// ctx->CurrentXMLDoc, which the windows are rebuilt from after every change; Save writes the file.
// The XML editor (XMLEditor.cpp) works on the same document and can stay open next to it.
#pragma once
#include <Main.h>
#include "imgui.h"
#include "imgui_internal.h"

namespace DesignMode {

    // Menu and shortcuts (main.cpp)
    bool CanEnable(const MainContext* ctx);           // a document is open
    void SetEnabled(MainContext* ctx, bool enabled);  // sets ctx->design_mode
    void Reset();                                     // forgets the selection and the undo history

    // One frame (Draw_Window in xml_window.cpp)
    void BeginFrame(bool active);                     // active: the windows are built from ctx->CurrentXMLDoc
    bool IsActive();
    void BeginWindow(const tinyxml2::XMLElement* key);  // right after ImGui::Begin() of an XML window
    // The user moved or resized an XML window: store its geometry in the <Window> element.
    void CommitWindowGeometry(MainContext* ctx, const tinyxml2::XMLElement* key, ImVec2 pos, ImVec2 size);
    void EndFrame(MainContext* ctx);                  // after the XML windows: mouse, keys, outlines, Properties window

    // Every widget (PlacedWidget.h). Keys are the widgets' elements; they are only compared.
    // Position of a widget being dragged, in window coordinates (ImGui::SetCursorPos).
    bool DraggedPosition(const tinyxml2::XMLElement* key, ImVec2* pos);
    void BeginWidget(bool container);
    void EndWidget(bool container, const tinyxml2::XMLElement* key, ImGuiWindow* window, const ImRect& rect,
        bool movable, float itemWidth);

} // namespace DesignMode
