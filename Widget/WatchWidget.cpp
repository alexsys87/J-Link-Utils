// WatchWidget.cpp
#include "WatchWidget.h"
#include <Main.h>
#include <string>
#include <cstdint>
#include <cstdlib> // strtoull
#include "Preferences.h"
#include <map>
#include <random>

using namespace tinyxml2;

// Static members
std::vector<WatchWidget::WatchRow> WatchWidget::s_rows;
bool WatchWidget::s_modified = false;
float WatchWidget::s_updateInterval = 100.0f;
float WatchWidget::s_opacity = 100.0f;
double WatchWidget::s_refreshTime = 0.0;
int WatchWidget::s_selectedRow = -1;      // row to delete
int WatchWidget::s_highlightedRow = -1;   // highlighted row
int WatchWidget::s_contextMenuRow = -1;
char WatchWidget::s_xmlPath[MAX_PATH] = "watch.xml";

// Length framing makes arbitrary names/descriptions unambiguous. Values and enabled
// state are deliberately absent: refreshing the target must never change a UI key.
static std::string WatchIdentityPart(const std::string& value) {
    return std::to_string(value.size()) + ":" + value;
}

static std::string NewWatchPreferenceId() {
    std::random_device random;
    std::string id;
    for (int i = 0; i < 4; ++i) id += WatchIdentityPart(std::to_string(random()));
    return id;
}

std::string WatchWidget::RowPreferenceIdentity(const WatchRow& row) {
    // Explicit IDs survive edits/reordering. Legacy XML gets a configuration identity
    // until the next ordinary configuration save adds an optional ID attribute.
    if (!row.preferenceId.empty()) return "id:" + row.preferenceId;
    return "config:" + WatchIdentityPart(row.name) + WatchIdentityPart(row.desc) +
        WatchIdentityPart(std::to_string(row.address)) + WatchIdentityPart(std::to_string(row.dataType)) +
        WatchIdentityPart(row.littleEndian ? "le" : "be");
}

void WatchWidget::LoadRowPreferences(WatchRow& row) {
    const std::string identity = RowPreferenceIdentity(row);
    // Indistinguishable legacy rows cannot safely be matched after reordering.
    // Do not guess; an ordinary configuration save gives them distinct IDs.
    int matches = 0;
    for (const auto& other : s_rows) if (RowPreferenceIdentity(other) == identity) ++matches;
    if (matches != 1) return;
    const std::string key = prefs::ScopedKey(s_xmlPath, "Watch.Row", identity);
    row.bitMode = prefs::ReadBool(key + ".Bit", false);
    row.hexMode = prefs::ReadBool(key + ".Hex", false);
}

void WatchWidget::SaveRowPreferences(const WatchRow& row) {
    const std::string identity = RowPreferenceIdentity(row);
    int matches = 0;
    for (const auto& other : s_rows) if (RowPreferenceIdentity(other) == identity) ++matches;
    if (matches != 1) return;
    const std::string key = prefs::ScopedKey(s_xmlPath, "Watch.Row", identity);
    prefs::WriteBool(key + ".Bit", row.bitMode);
    prefs::WriteBool(key + ".Hex", row.hexMode);
}

static bool IsSignedIntType(int dt) {
    return dt == S08_DEC || dt == S16_DEC || dt == S32_DEC || dt == S64_DEC;
}

void WatchWidget::LoadFromXML(const char* path) {
    XMLDocument doc;
    if (doc.LoadFile(path) != XML_SUCCESS) {
        s_rows.clear();
        s_rows.resize(1);
        s_rows[0].enabled = true;
        s_rows[0].dataType = BOOLEAN;
        s_rows[0].name[0] = '\0';
        s_rows[0].desc[0] = '\0';
        s_rows[0].address = 0;
        s_rows[0].value = 0;
        s_rows[0].bitMode = false;
        s_rows[0].littleEndian = true;
        s_rows[0].hexMode = false;
        return;
    }
    s_rows.clear();
    XMLElement* watch = doc.FirstChildElement("Watch");
    if (!watch) return;
    XMLElement* items = watch->FirstChildElement("Items");
    if (!items) return;
    for (XMLElement* line = items->FirstChildElement("WatchLine"); line; line = line->NextSiblingElement("WatchLine")) {
        WatchRow row = {};
        if (const char* id = line->Attribute("PreferenceId")) row.preferenceId = id;
        if (line->FirstChildElement("Enabled"))
            line->FirstChildElement("Enabled")->QueryBoolText(&row.enabled);
        if (line->FirstChildElement("DataType"))
            line->FirstChildElement("DataType")->QueryIntText(&row.dataType);
        if (line->FirstChildElement("Text")) {
            const char* txt = line->FirstChildElement("Text")->GetText();
            if (txt) strncpy_s(row.name, txt, MAX_STR_LEN - 1);
        }
        if (line->FirstChildElement("Address"))
            line->FirstChildElement("Address")->QueryIntText(&row.address);
        if (line->FirstChildElement("Value")) {
            const char* valStr = line->FirstChildElement("Value")->GetText();
            if (valStr) {
                char* end;
                row.value = strtoull(valStr, &end, 10);
            }
        }
        if (line->FirstChildElement("Description")) {
            const char* desc = line->FirstChildElement("Description")->GetText();
            if (desc) strncpy_s(row.desc, desc, MAX_STR_LEN - 1);
        }
        row.bitMode = false;
        row.littleEndian = true;
        if (line->FirstChildElement("LittleEndian"))
            line->FirstChildElement("LittleEndian")->QueryBoolText(&row.littleEndian);
        row.hexMode = false;
        s_rows.push_back(row);
    }
    for (auto& row : s_rows) LoadRowPreferences(row);
}

void WatchWidget::SaveToXML(const char* path, bool exportOnly) {
    // Export is a snapshot, not a save of the active watch.xml. In particular it
    // must not switch live preference identities to IDs absent from watch.xml.
    // Stage normal-save migrations too, so a failed XML write cannot orphan prefs.
    std::vector<WatchRow> rowsToSave = s_rows;
    if (!exportOnly) {
        std::map<std::string, int> idCounts;
        for (const auto& row : rowsToSave) if (!row.preferenceId.empty()) ++idCounts[row.preferenceId];
        for (auto& row : rowsToSave) {
            if (row.preferenceId.empty() || idCounts[row.preferenceId] > 1) {
                std::string id;
                do { id = NewWatchPreferenceId(); } while (idCounts.count(id));
                row.preferenceId = id;
                idCounts[id] = 1;
            }
        }
    }
    XMLDocument doc;
    XMLElement* watch = doc.NewElement("Watch");
    doc.InsertEndChild(watch);
    XMLElement* items = doc.NewElement("Items");
    watch->InsertEndChild(items);

    for (const auto& row : rowsToSave) {
        XMLElement* line = doc.NewElement("WatchLine");
        if (!row.preferenceId.empty()) line->SetAttribute("PreferenceId", row.preferenceId.c_str());
        items->InsertEndChild(line);

        XMLElement* en = doc.NewElement("Enabled");
        en->SetText(row.enabled);
        line->InsertEndChild(en);

        XMLElement* dt = doc.NewElement("DataType");
        dt->SetText(row.dataType);
        line->InsertEndChild(dt);

        XMLElement* txt = doc.NewElement("Text");
        txt->SetText(row.name);
        line->InsertEndChild(txt);

        XMLElement* addr = doc.NewElement("Address");
        addr->SetText(row.address);
        line->InsertEndChild(addr);

        XMLElement* val = doc.NewElement("Value");
        char buf[32];
        snprintf(buf, sizeof(buf), "%llu", (unsigned long long)row.value);
        val->SetText(buf);
        line->InsertEndChild(val);

        XMLElement* desc = doc.NewElement("Description");
        desc->SetText(row.desc);
        line->InsertEndChild(desc);

        XMLElement* le = doc.NewElement("LittleEndian");
        le->SetText(row.littleEndian);
        line->InsertEndChild(le);
    }
    if (doc.SaveFile(path) == XML_SUCCESS && !exportOnly) {
        // Only successful saves of the active configuration migrate live IDs.
        // UI view toggles never rewrite XML or snapshot current target values.
        for (size_t i = 0; i < s_rows.size(); ++i) {
            if (s_rows[i].preferenceId != rowsToSave[i].preferenceId) {
                s_rows[i].preferenceId = rowsToSave[i].preferenceId;
                SaveRowPreferences(s_rows[i]);
            }
        }
    }
}

WatchWidget::WatchWidget() {
    static bool initialized = false;
    if (!initialized) {
        s_updateInterval = (float)prefs::ReadDouble("Watch.UpdateInterval", 100.0, 1.0, 10000.0);
        s_opacity = (float)prefs::ReadDouble("Watch.Opacity", 100.0, 1.0, 100.0);
        s_refreshTime = 0.0;
        s_selectedRow = -1;
        s_highlightedRow = -1;
        s_contextMenuRow = -1;
        const char* defaultPath = GetFilePath("watch.xml");
        strncpy(s_xmlPath, defaultPath ? defaultPath : "watch.xml", MAX_PATH - 1);
        SDL_free((void*)defaultPath);
        LoadFromXML(s_xmlPath);
        initialized = true;
    }
}

void WatchWidget::Draw(MainContext* ctx, uint32_t* dynamicAddr) {
    if (ImGui::CollapsingHeader("Options")) {
        ImGui::PushItemWidth(250);
        float oldInterval = s_updateInterval;
        ImGui::DragFloat("Update Interval(ms)", &s_updateInterval, 1.0f, 1.0f, 10000.0f);
        if (oldInterval != s_updateInterval) {
            prefs::WriteDouble("Watch.UpdateInterval", s_updateInterval);
            // Reset the timer so that the new interval applies at once
            s_refreshTime = 0.0;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Opacity(%)", &s_opacity, 0.1f, 1.0f, 100.0f))
            prefs::WriteDouble("Watch.Opacity", s_opacity);
        ImGui::PopItemWidth();
    }

    // Apply the opacity to the table colors
    float alpha = s_opacity / 100.0f;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, alpha));
    ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.1f, 0.1f, 0.1f, alpha));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.5f, 0.5f, 0.5f, alpha));

    // Context menu hint
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(
            "Right-click on any field (Address, Name, Value, Description) "
            "to open row context menu (Delete, Show As, Bit View, Hex View).\n"
            "Right-click on empty area to open global menu (New Row, Export, Import)."
        );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::Text("Watch Table");

    // Column headers
    ImGui::Columns(4, nullptr, true);
    float col0 = ImGui::GetColumnWidth(0);
    float col1 = ImGui::GetColumnWidth(1);
    float col2 = ImGui::GetColumnWidth(2);
    float col3 = ImGui::GetColumnWidth(3);

    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Address");
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Name");
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Value");
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Comment");
    ImGui::Columns(1);

    ImGui::BeginChild("WatchTable", ImVec2(0, 0), false);

    ImGui::Columns(4, nullptr, true);
    ImGui::SetColumnWidth(0, col0);
    ImGui::SetColumnWidth(1, col1);
    ImGui::SetColumnWidth(2, col2);
    ImGui::SetColumnWidth(3, col3);

    for (int i = 0; i < (int)s_rows.size(); ++i) {
        WatchRow& row = s_rows[i];

        if (ImGui::GetColumnIndex() == 0)
            ImGui::Separator();

        ImVec2 row_start = ImGui::GetCursorScreenPos();

        // --- Column 0: Enabled + Address ---
        ImGui::PushID(i);
        ImGui::PushItemWidth(-1);

        if (ImGui::Checkbox("##En", &row.enabled)) {
            s_modified = true;
        }
        if (ImGui::IsItemClicked(0)) {
            s_highlightedRow = i;
        }

        ImGui::SameLine();
        if (ImGui::InputInt("##Addr", &row.address, 1, 100, ImGuiInputTextFlags_CharsHexadecimal)) {
            s_modified = true;
        }
        s_contextMenuRow = i;
        if (ImGui::BeginPopupContextItem("RowContextMenu")) {
            DrawRowContextMenu();
            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::NextColumn();

        // --- Column 1: Name ---
        ImGui::PushID(i + 1000);
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Name", row.name, MAX_STR_LEN)) {
            s_modified = true;
        }
        s_contextMenuRow = i;
        if (ImGui::BeginPopupContextItem("RowContextMenu")) {
            DrawRowContextMenu();
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::NextColumn();

        // --- Column 2: Value ---
        ImGui::PushID(i + 2000);
        ImGui::PushItemWidth(-1);

        bool valueChanged = false;

        // Bit view for integer types
        if (row.bitMode && IsIntegerType(row.dataType)) {
            int numBits = GetDataTypeBits(row.dataType);
            if (numBits > 0) {
                float bitSize = 18.0f;
                float spacing = 2.0f;
                int bitsPerRow = 32;
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 cursor_start = ImGui::GetCursorScreenPos();

                for (int b = numBits - 1; b >= 0; --b) {
                    int col = (numBits - 1 - b) % bitsPerRow;
                    int rowIndex = (numBits - 1 - b) / bitsPerRow;

                    ImVec2 rect_min(
                        cursor_start.x + col * (bitSize + spacing),
                        cursor_start.y + rowIndex * (bitSize + spacing)
                    );
                    ImVec2 rect_max(rect_min.x + bitSize, rect_min.y + bitSize);

                    bool is_set = (row.value & (1ULL << b)) != 0;

                    ImU32 bg_color = is_set ? ImGui::GetColorU32(ImGuiCol_SliderGrabActive) : ImGui::GetColorU32(ImGuiCol_FrameBg);
                    draw_list->AddRectFilled(rect_min, rect_max, bg_color, 2.0f);
                    draw_list->AddRect(rect_min, rect_max, ImGui::GetColorU32(ImGuiCol_Border), 2.0f);

                    char text[2] = { is_set ? '1' : '0', '\0' };
                    ImVec2 text_size = ImGui::CalcTextSize(text);
                    float text_x = rect_min.x + (bitSize - text_size.x) * 0.5f;
                    float text_y = rect_min.y + (bitSize - text_size.y) * 0.5f;
                    draw_list->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), text);

                    ImGui::SetCursorScreenPos(rect_min);
                    ImGui::InvisibleButton(("##bit" + std::to_string(b)).c_str(), ImVec2(bitSize, bitSize));
                    if (ImGui::IsItemClicked() && !(ImGui::GetItemFlags() & ImGuiItemFlags_Disabled)) {
                        row.value ^= (1ULL << b);
                        valueChanged = true;
                    }
                }

                int totalRows = (numBits + bitsPerRow - 1) / bitsPerRow;
                ImGui::SetCursorScreenPos(ImVec2(cursor_start.x, cursor_start.y + totalRows * (bitSize + spacing)));
            }
        }
        else
        {
            // Regular editor
            switch (row.dataType) {
            case BOOLEAN: {
                bool b = row.value != 0;
                if (ImGui::Checkbox("##Value", &b)) {
                    row.value = b;
                    valueChanged = true;
                }
                break;
            }
            case S08_DEC:
            case U08_DEC:
            case S16_DEC:
            case U16_DEC:
            case S32_DEC:
            case U32_DEC:
            case S64_DEC:
            case U64_DEC: {
                int flags = row.hexMode ? ImGuiInputTextFlags_CharsHexadecimal : ImGuiInputTextFlags_CharsDecimal;
                const int bits = GetDataTypeBits(row.dataType);
                // Hex view shows the raw bits, so it always uses the unsigned type.
                const bool isSigned = !row.hexMode && IsSignedIntType(row.dataType);

                // Edit the value at its own width: signed types show negative numbers
                // and the hex format is honored for every size.
                union { ImU8 u8; ImU16 u16; ImU32 u32; ImU64 u64; } edit = {};
                ImGuiDataType dataType;
                const char* format;
                switch (bits) {
                case 8:
                    edit.u8 = (ImU8)row.value;
                    dataType = isSigned ? ImGuiDataType_S8 : ImGuiDataType_U8;
                    format = row.hexMode ? "%02X" : (isSigned ? "%d" : "%u");
                    break;
                case 16:
                    edit.u16 = (ImU16)row.value;
                    dataType = isSigned ? ImGuiDataType_S16 : ImGuiDataType_U16;
                    format = row.hexMode ? "%04X" : (isSigned ? "%d" : "%u");
                    break;
                case 32:
                    edit.u32 = (ImU32)row.value;
                    dataType = isSigned ? ImGuiDataType_S32 : ImGuiDataType_U32;
                    format = row.hexMode ? "%08X" : (isSigned ? "%d" : "%u");
                    break;
                default:
                    edit.u64 = row.value;
                    dataType = isSigned ? ImGuiDataType_S64 : ImGuiDataType_U64;
                    format = row.hexMode ? "%016llX" : (isSigned ? "%lld" : "%llu");
                    break;
                }

                valueChanged = ImGui::InputScalar("##Value", dataType, &edit, NULL, NULL, format, flags);
                if (valueChanged) {
                    switch (bits) {
                    case 8:  row.value = edit.u8; break;
                    case 16: row.value = edit.u16; break;
                    case 32: row.value = edit.u32; break;
                    default: row.value = edit.u64; break;
                    }
                }
                break;
            }
            case FLOAT: {
                float f;
                memcpy(&f, &row.value, sizeof(float));
                if (ImGui::InputFloat("##Value", &f, 0.0f, 0.0f, "%.5f", 0)) {
                    memcpy(&row.value, &f, sizeof(float));
                    valueChanged = true;
                }
                break;
            }
            case FLOAT_INV: {
                uint32_t w = (uint32_t)row.value;
                w = SwapLeToBe32(w);
                float f = *(float*)&w;
                if (ImGui::InputFloat("##Value", &f, 0.0f, 0.0f, "%.5f", 0)) {
                    w = SwapBeToLe32(*(uint32_t*)&f);
                    row.value = w;
                    valueChanged = true;
                }
                break;
            }
            default:
                int temp = (int)row.value;
                if (ImGui::InputInt("##Value", &temp, 0, 0, ImGuiInputTextFlags_CharsDecimal)) {
                    row.value = temp;
                    valueChanged = true;
                }
            }
        }

        if (valueChanged && row.enabled) {
            uint32_t writeAddr = row.address;
            switch (row.dataType) {
            case BOOLEAN: {
                uint8_t v = row.value & 0x01;
                DataReadWriteBoolean(ctx, nullptr, &v, &writeAddr);
                break;
            }
            case S08_DEC:
            case U08_DEC: {
                uint8_t v = row.value & 0xFF;
                DataReadWriteU08(ctx, nullptr, &v, &writeAddr);
                break;
            }
            case S16_DEC:
            case U16_DEC: {
                uint16_t v = row.value & 0xFFFF;
                if (!row.littleEndian) v = Swap16(v);
                DataReadWriteU16(ctx, nullptr, &v, &writeAddr);
                break;
            }
            case S32_DEC:
            case U32_DEC: {
                uint32_t v = (uint32_t)row.value;
                if (!row.littleEndian) v = SwapLeToBe32(v);
                DataReadWriteU32(ctx, nullptr, &v, &writeAddr);
                break;
            }
            case S64_DEC:
            case U64_DEC: {
                uint64_t v = row.value;
                if (!row.littleEndian) v = SwapLeToBe64(v);
                DataReadWriteU64(ctx, nullptr, &v, &writeAddr);
                break;
            }
            case FLOAT: {
                uint32_t v;
                memcpy(&v, &row.value, sizeof(v));
                if (!row.littleEndian) v = SwapLeToBe32(v);
                DataReadWriteU32(ctx, nullptr, &v, &writeAddr);
                break;
            }
            case FLOAT_INV: {
                uint32_t v;
                memcpy(&v, &row.value, sizeof(v));
                if (row.littleEndian) v = SwapBeToLe32(v);
                DataReadWriteU32(ctx, nullptr, &v, &writeAddr);
                break;
            }
            }
        }

        // Row context menu
        s_contextMenuRow = i;
        if (ImGui::BeginPopupContextItem("RowContextMenu")) {
            DrawRowContextMenu();
            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::NextColumn();

        // --- Column 3: Description ---
        ImGui::PushID(i + 3000);
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Desc", row.desc, MAX_STR_LEN)) {
            s_modified = true;
        }
        s_contextMenuRow = i;
        if (ImGui::BeginPopupContextItem("RowContextMenu")) {
            DrawRowContextMenu();
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::NextColumn();

        // Row highlight
        if (i == s_highlightedRow) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 row_end = ImGui::GetCursorScreenPos();
            float row_height = row_end.y - row_start.y;
            if (row_height < 1.0f) row_height = ImGui::GetTextLineHeightWithSpacing();
            draw_list->AddRectFilled(
                row_start,
                ImVec2(row_end.x, row_start.y + row_height),
                IM_COL32(50, 50, 150, 80)
            );
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    // --- Window context menu ---
    if (ImGui::BeginPopupContextWindow("WatchMainMenu")) {
        if (ImGui::MenuItem("New Row")) {
            WatchRow newRow = {};
            newRow.enabled = true;
            newRow.dataType = BOOLEAN;
            newRow.bitMode = false;
            newRow.littleEndian = true;
            newRow.hexMode = false;
            newRow.preferenceId = NewWatchPreferenceId();
            s_rows.push_back(newRow);
            s_modified = true;
        }
        if (ImGui::MenuItem("Export...")) {
            const char* f = file_save_dialog("XML(*.xml)\0*.XML\0");
            if (f) {
                char path[MAX_PATH]; // local buffer of the right size
                SDL_strlcpy(path, f, MAX_PATH); // copy the path from the dialog

                EnsureXmlExtension(path, MAX_PATH); // append .xml if missing

                SaveToXML(path, true); // export without changing the active identities
                // no SDL_free(path): the array is on the stack
            }
        }
        if (ImGui::MenuItem("Import...")) {
            const char* f = file_open_dialog("XML(*.xml)\0*.XML\0");
            if (f) {
                LoadFromXML(f);
                s_modified = true;
            }
        }
        ImGui::EndPopup();
    }

    // Row removal
    if (s_selectedRow >= 0 && s_selectedRow < (int)s_rows.size()) {
        s_rows.erase(s_rows.begin() + s_selectedRow);
        if (s_contextMenuRow == s_selectedRow) s_contextMenuRow = -1;
        else if (s_contextMenuRow > s_selectedRow) s_contextMenuRow--;
        if (s_highlightedRow == s_selectedRow) s_highlightedRow = -1;
        else if (s_highlightedRow > s_selectedRow) s_highlightedRow--;
        s_selectedRow = -1;
        s_modified = true;
    }

    // Autosave
    if (s_modified) {
        SaveToXML(s_xmlPath);
        s_modified = false;
    }

    // Periodic refresh
    if (ctx->StartComunication && ImGui::GetTime() > s_refreshTime) {
        for (auto& row : s_rows) {
            if (!row.enabled) continue;
            uint32_t addr = row.address;
            switch (row.dataType) {
            case BOOLEAN:
                row.value = DataReadWriteBoolean(ctx, nullptr, nullptr, &addr) & 0x0001;
                break;
            case S08_DEC:
            case U08_DEC:
                row.value = DataReadWriteU08(ctx, nullptr, nullptr, &addr) & 0x00FF;
                break;
            case S16_DEC:
            case U16_DEC: {
                uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
                if (!row.littleEndian) raw = Swap16(raw);
                row.value = raw;
                break;
            }
            case S32_DEC:
            case U32_DEC: {
                uint32_t tmp = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
                if (!row.littleEndian) tmp = SwapLeToBe32(tmp);
                row.value = tmp;
                break;
            }
            case S64_DEC:
            case U64_DEC: {
                uint64_t tmp = DataReadWriteU64(ctx, nullptr, nullptr, &addr);
                if (!row.littleEndian) tmp = SwapLeToBe64(tmp);
                row.value = tmp;
                break;
            }
            case FLOAT: {
                uint32_t tmp = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
                if (!row.littleEndian) tmp = SwapLeToBe32(tmp);
                memcpy(&row.value, &tmp, sizeof(tmp));
                break;
            }
            case FLOAT_INV: {
                uint32_t tmp = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
                if (row.littleEndian) tmp = SwapLeToBe32(tmp);
                memcpy(&row.value, &tmp, sizeof(tmp));
                break;
            }
            }
        }
        s_refreshTime = ImGui::GetTime() + s_updateInterval / 1000.0;
    }

    ImGui::PopStyleColor(4); // restore the original colors
}

void WatchWidget::DrawRowContextMenu() {
    int rowIndex = s_contextMenuRow;
    if (rowIndex < 0 || rowIndex >= (int)s_rows.size())
        return;

    if (ImGui::MenuItem("Delete Row")) {
        s_selectedRow = rowIndex;
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Show As")) {
        bool selected[11] = { false };
        int dt = s_rows[rowIndex].dataType;
        auto setType = [&](int newType) {
            s_rows[rowIndex].dataType = newType;
            s_rows[rowIndex].bitMode = false;
            s_rows[rowIndex].hexMode = false;
            SaveRowPreferences(s_rows[rowIndex]);
            s_modified = true;
            };
        if (dt >= 0 && dt < 11) selected[dt] = true;

        if (ImGui::MenuItem("Boolean", "", selected[BOOLEAN])) {
            setType(BOOLEAN);
        }
        if (ImGui::MenuItem("S08 Dec", "", selected[S08_DEC])) {
            setType(S08_DEC);
        }
        if (ImGui::MenuItem("U08 Dec", "", selected[U08_DEC])) {
            setType(U08_DEC);
        }
        if (ImGui::MenuItem("S16 Dec", "", selected[S16_DEC])) {
            setType(S16_DEC);
        }
        if (ImGui::MenuItem("U16 Dec", "", selected[U16_DEC])) {
            setType(U16_DEC);
        }
        if (ImGui::MenuItem("S32 Dec", "", selected[S32_DEC])) {
            setType(S32_DEC);
        }
        if (ImGui::MenuItem("U32 Dec", "", selected[U32_DEC])) {
            setType(U32_DEC);
        }
        if (ImGui::MenuItem("S64 Dec", "", selected[S64_DEC])) {
            setType(S64_DEC);
        }
        if (ImGui::MenuItem("U64 Dec", "", selected[U64_DEC])) {
            setType(U64_DEC);
        }
        if (ImGui::MenuItem("Float", "", selected[FLOAT])) {
            setType(FLOAT);
        }
        if (ImGui::MenuItem("Float inverse", "", selected[FLOAT_INV])) {
            setType(FLOAT_INV);
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    int dt = s_rows[rowIndex].dataType;
    if (IsIntegerType(dt)) {
        if (ImGui::MenuItem("Bit View", "", &s_rows[rowIndex].bitMode)) {
            SaveRowPreferences(s_rows[rowIndex]);
        }
        if (ImGui::MenuItem("Hex View", "", &s_rows[rowIndex].hexMode)) {
            if (s_rows[rowIndex].hexMode) s_rows[rowIndex].bitMode = false;
            SaveRowPreferences(s_rows[rowIndex]);
        }
    }
    if (dt == S16_DEC || dt == U16_DEC || dt == S32_DEC || dt == U32_DEC ||
        dt == S64_DEC || dt == U64_DEC || dt == FLOAT || dt == FLOAT_INV) {
        if (ImGui::MenuItem("little Endian", "", &s_rows[rowIndex].littleEndian)) {
            s_modified = true;
        }
    }
}

bool WatchWidget::IsIntegerType(int dt) {
    return dt == S08_DEC || dt == U08_DEC || dt == S16_DEC || dt == U16_DEC ||
        dt == S32_DEC || dt == U32_DEC || dt == S64_DEC || dt == U64_DEC;
}

int WatchWidget::GetDataTypeBits(int dt) {
    switch (dt) {
    case S08_DEC: case U08_DEC: return 8;
    case S16_DEC: case U16_DEC: return 16;
    case S32_DEC: case U32_DEC: return 32;
    case S64_DEC: case U64_DEC: return 64;
    default: return 0;
    }
}