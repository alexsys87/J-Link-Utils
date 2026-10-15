#pragma once
#include <Main.h>
#include <string>
#include "WidgetTemplateFactory.h"

void ShowWidgetEditor(bool* p_open, MainContext* ctx);
void ResetXMLEditorState();

// Shared with the design mode (DesignMode.cpp)
// Input control for one property value, chosen from its type. Returns true with the new text in 'out'.
bool EditPropertyValue(const WidgetTemplateFactory::PropDef& prop, const char* text, std::string& out, float width = 220.0f);
// Body of the "add widget" popup opened with ImGui::OpenPopup(popupId); returns the chosen widget type.
const WidgetTemplateFactory::NodeDef* WidgetPickerPopup(const char* popupId);
