// MemoryViewWidget.h
#pragma once
#include "widget_common.h"
#include <vector>
#include <cctype>

class MemoryViewWidget : public UIWidget {
    std::string label;
    uint32_t baseAddress;
    uint16_t    numBytes;
    int       bytesPerLine;
    bool      littleEndian; // не используется, оставлено для совместимости

    std::vector<uint8_t> m_cache;
    double m_lastUpdateTime;
    static constexpr double UPDATE_INTERVAL = 0.1; // обновление кэша 10 раз в секунду

public:
    MemoryViewWidget(const char* lbl, uint32_t base, uint16_t bytes, int bpl = 16, bool le = true)
        : baseAddress(base), numBytes(bytes), bytesPerLine(bpl), littleEndian(le),
        m_cache(bytes, 0), m_lastUpdateTime(0.0) {
        label = lbl ? lbl : "";
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (!ctx || !ctx->IOMAP_Data) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "IOMAP_Data not available");
            return;
        }

        ImGui::PushID(this);
        DrawMemoryView(ctx);
        ImGui::PopID();
    }

private:
    void UpdateCache(MainContext* ctx) {
        double now = ImGui::GetTime();
        if (now - m_lastUpdateTime < UPDATE_INTERVAL)
            return;

        // Запрашиваем чтение всего диапазона через пакетную операцию
        DataReadWriteArray(ctx, nullptr, nullptr, &baseAddress, numBytes);

        // После постановки запроса копируем текущее содержимое IOMAP_Data в кэш
        // (оно может быть устаревшим, но будет обновлено в следующих кадрах)
        uint32_t offset = baseAddress - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        memcpy(m_cache.data(), ctx->IOMAP_Data + offset, numBytes);
        SDL_UnlockMutex(ctx->Datalock);

        m_lastUpdateTime = now;
    }

    void DrawMemoryView(MainContext* ctx) {
        ImGuiStyle& style = ImGui::GetStyle();

        // Обновляем кэш при необходимости
        UpdateCache(ctx);

        // Настройки размеров
        float addrWidth = ImGui::CalcTextSize("000000000 ").x;
        float cellWidth = 30.0f;
        float cellSpacing = style.ItemSpacing.x;

        // Заголовок
        ImGui::Text(" Address    ");
        ImGui::SameLine(addrWidth);
        for (int i = 0; i < bytesPerLine; ++i) {
            ImGui::SetCursorPosX(addrWidth + i * (cellWidth + cellSpacing));
            ImGui::Text("+%02X", i);
            if (i < bytesPerLine - 1)
                ImGui::SameLine(0, style.ItemSpacing.x);
        }
        ImGui::SameLine(addrWidth + bytesPerLine * (cellWidth + cellSpacing) + cellSpacing);
        ImGui::Text("ASCII");
        ImGui::Separator();

        ImGui::BeginChild("##memory_scroll", ImVec2(0, -1), true, ImGuiWindowFlags_HorizontalScrollbar);

        size_t numLines = (numBytes + bytesPerLine - 1) / bytesPerLine;
        for (size_t line = 0; line < numLines; ++line) {
            uint32_t lineAddr = baseAddress + (uint32_t)line * bytesPerLine;
            uint32_t lineOffset = lineAddr - baseAddress; // смещение внутри кэша

            // Адрес строки
            ImGui::Text("%04X", lineAddr);

            // Колонки HEX (используем данные из кэша)
            for (int col = 0; col < bytesPerLine; ++col) {
                uint32_t byteOffset = lineOffset + col;
                ImGui::SameLine(addrWidth + col * (cellWidth + cellSpacing));

                if (byteOffset < numBytes) {
                    uint8_t val = m_cache[byteOffset];
                    ImGui::PushID((int)(lineAddr + col));
                    ImGui::SetNextItemWidth(cellWidth);
                    char inputBuf[3];
                    snprintf(inputBuf, sizeof(inputBuf), "%02X", val);
                    if (ImGui::InputText("##hex", inputBuf, 3, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
                        unsigned int newVal;
                        if (sscanf_s(inputBuf, "%02X", &newVal) == 1) {
                            uint8_t v = (uint8_t)newVal;
                            uint32_t physAddr = lineAddr + col;
                            // Запись через обычную функцию (она поставит запрос в очередь)
                            DataReadWriteU08(ctx, nullptr, &v, &physAddr);
                            // Обновляем кэш сразу, чтобы отобразить новое значение
                            m_cache[byteOffset] = v;
                        }
                    }
                    ImGui::PopID();
                }
                else {
                    ImGui::Text("   ");
                }
            }

            // ASCII колонки
            ImGui::SameLine(addrWidth + bytesPerLine * (cellWidth + cellSpacing) + cellSpacing);
            for (int col = 0; col < bytesPerLine; ++col) {
                uint32_t byteOffset = lineOffset + col;
                if (byteOffset < numBytes) {
                    uint8_t val = m_cache[byteOffset];
                    char c = (val >= 32 && val < 127) ? (char)val : '.';
                    ImGui::Text("%c", c);
                    if (col < bytesPerLine - 1) ImGui::SameLine(0, 2);
                }
                else {
                    ImGui::Text(" ");
                    if (col < bytesPerLine - 1) ImGui::SameLine(0, 2);
                }
            }
        }

        ImGui::EndChild();
    }
};