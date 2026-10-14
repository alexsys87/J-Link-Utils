// WatchWidget.cpp
#include "WatchWidget.h"
#include <Main.h>
#include <string>
#include <cstdint>
#include <cstdlib> // для strtoull

using namespace tinyxml2;

// Определение статических членов
std::vector<WatchWidget::WatchRow> WatchWidget::s_rows;
bool WatchWidget::s_modified = false;
float WatchWidget::s_updateInterval = 100.0f;
float WatchWidget::s_opacity = 100.0f;
double WatchWidget::s_refreshTime = 0.0;
int WatchWidget::s_selectedRow = -1;      // для удаления
int WatchWidget::s_highlightedRow = -1;   // для подсветки строки
int WatchWidget::s_contextMenuRow = -1;
char WatchWidget::s_xmlPath[MAX_PATH] = "watch.xml";

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
    for (XMLElement* line = items->FirstChildElement("WatchLine"); line; line = line->NextSiblingElement()) {
        WatchRow row = {};
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
}

void WatchWidget::SaveToXML(const char* path) {
    XMLDocument doc;
    XMLElement* watch = doc.NewElement("Watch");
    doc.InsertEndChild(watch);
    XMLElement* items = doc.NewElement("Items");
    watch->InsertEndChild(items);

    for (const auto& row : s_rows) {
        XMLElement* line = doc.NewElement("WatchLine");
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
    doc.SaveFile(path);
}

WatchWidget::WatchWidget() {
    static bool initialized = false;
    if (!initialized) {
        s_updateInterval = 100.0f;
        s_opacity = 100.0f;
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
            // Сбрасываем таймер, чтобы новый интервал применился немедленно
            s_refreshTime = 0.0;
        }
        ImGui::SameLine();
        ImGui::DragFloat("Opacity(%)", &s_opacity, 0.1f, 1.0f, 100.0f);
        ImGui::PopItemWidth();
    }

    // Применяем прозрачность к цветам таблицы
    float alpha = s_opacity / 100.0f;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, alpha));
    ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.1f, 0.1f, 0.1f, alpha));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.5f, 0.5f, 0.5f, alpha));

    // Подсказка о контекстных меню
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

    // Заголовки колонок
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

        // --- Колонка 0: Enabled + Address ---
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

        // --- Колонка 1: Name ---
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

        // --- Колонка 2: Value ---
        ImGui::PushID(i + 2000);
        ImGui::PushItemWidth(-1);

        bool valueChanged = false;

        // Побитовый режим для целочисленных типов
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
            // Обычный редактор
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
                const char* format = nullptr;
                if (row.hexMode) {
                    int bits = GetDataTypeBits(row.dataType);
                    if (bits == 8) format = "%02X";
                    else if (bits == 16) format = "%04X";
                    else if (bits == 32) format = "%08X";
                    else if (bits == 64) format = "%016llX";
                }
                else {
                    format = "%lld"; // для знаковых и беззнаковых
                }

                if (row.dataType == S64_DEC || row.dataType == U64_DEC) {
                    ImGuiDataType dataType = (row.dataType == S64_DEC) ? ImGuiDataType_S64 : ImGuiDataType_U64;
                    valueChanged = ImGui::InputScalar("##Value", dataType, &row.value, NULL, NULL, format, flags);
                }
                else {
                    // Для меньших разрядностей используем временную переменную
                    int tempVal = (int)row.value;
                    valueChanged = ImGui::InputInt("##Value", &tempVal, 0, 0, flags);
                    if (valueChanged) {
                        row.value = tempVal;
                        int bits = GetDataTypeBits(row.dataType);
                        if (bits < 64) {
                            uint64_t mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1);
                            row.value &= mask;
                        }
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

        // Контекстное меню строки
        s_contextMenuRow = i;
        if (ImGui::BeginPopupContextItem("RowContextMenu")) {
            DrawRowContextMenu();
            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::NextColumn();

        // --- Колонка 3: Description ---
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

        // Подсветка строки
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

    // --- Общее контекстное меню окна ---
    if (ImGui::BeginPopupContextWindow("WatchMainMenu")) {
        if (ImGui::MenuItem("New Row")) {
            WatchRow newRow = {};
            newRow.enabled = true;
            newRow.dataType = BOOLEAN;
            newRow.bitMode = false;
            newRow.littleEndian = true;
            newRow.hexMode = false;
            s_rows.push_back(newRow);
            s_modified = true;
        }
        if (ImGui::MenuItem("Export...")) {
            const char* f = file_save_dialog("XML(*.xml)\0*.XML\0");
            if (f) {
                char path[MAX_PATH]; // Создаем локальный буфер нужного размера
                SDL_strlcpy(path, f, MAX_PATH); // Копируем путь из диалога в наш буфер

                EnsureXmlExtension(path, MAX_PATH); // Добавляем .xml, если его нет

                SaveToXML(path); // Сохраняем
                // SDL_free(path) БОЛЬШЕ НЕ НУЖЕН, так как массив на стеке
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

    // Удаление строки
    if (s_selectedRow >= 0 && s_selectedRow < (int)s_rows.size()) {
        s_rows.erase(s_rows.begin() + s_selectedRow);
        if (s_contextMenuRow == s_selectedRow) s_contextMenuRow = -1;
        else if (s_contextMenuRow > s_selectedRow) s_contextMenuRow--;
        if (s_highlightedRow == s_selectedRow) s_highlightedRow = -1;
        else if (s_highlightedRow > s_selectedRow) s_highlightedRow--;
        s_selectedRow = -1;
        s_modified = true;
    }

    // Автосохранение
    if (s_modified) {
        SaveToXML(s_xmlPath);
        s_modified = false;
    }

    // Периодическое обновление
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

    ImGui::PopStyleColor(4); // восстанавливаем исходные цвета
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
            // nothing
        }
        if (ImGui::MenuItem("Hex View", "", &s_rows[rowIndex].hexMode)) {
            if (s_rows[rowIndex].hexMode) s_rows[rowIndex].bitMode = false;
            s_modified = true;
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