// MemoryViewWidget.h
#pragma once
#include "../Localization.h"
#include "widget_common.h"
#include <vector>
#include <cctype>

class MemoryViewWidget : public UIWidget {
    std::string label;
    uint32_t baseAddress;
    uint16_t    numBytes;
    int       bytesPerLine;
    bool      littleEndian; // unused, kept for compatibility

    std::vector<uint8_t> m_cache;
    double m_lastUpdateTime;
    static constexpr double UPDATE_INTERVAL = 0.1; // refresh the cache 10 times per second

public:
    MemoryViewWidget(const char* lbl, uint32_t base, uint16_t bytes, int bpl = 16, bool le = true)
        : baseAddress(base), numBytes(bytes), bytesPerLine(std::clamp(bpl, 1, 256)), littleEndian(le),
        m_cache(bytes, 0), m_lastUpdateTime(0.0) {
        label = lbl ? lbl : "";
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (!ctx || !ctx->IOMAP_Data) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), app_language::Tr("IOMAP_Data not available", "IOMAP_Data недоступны"));
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

        // Request the whole range as one block
        DataReadWriteArray(ctx, nullptr, nullptr, &baseAddress, numBytes);

        // Then copy the current IOMAP_Data contents into the cache
        // (it may be stale, later frames refresh it)
        SDL_LockMutex(ctx->Datalock);
        if (numBytes > 0 && ui_safety::IsMappedRange(baseAddress, ctx->mcu_memory_base, numBytes, IOMAP_SIZE)) {
            uint32_t offset = baseAddress - ctx->mcu_memory_base;
            memcpy(m_cache.data(), ctx->IOMAP_Data + offset, numBytes);
        } else {
            std::fill(m_cache.begin(), m_cache.end(), 0);
        }
        SDL_UnlockMutex(ctx->Datalock);

        m_lastUpdateTime = now;
    }

    void DrawMemoryView(MainContext* ctx) {
        ImGuiStyle& style = ImGui::GetStyle();

        // Refresh the cache when needed
        UpdateCache(ctx);

        // Sizes
        float addrWidth = ImGui::CalcTextSize("000000000 ").x;
        float cellWidth = 30.0f;
        float cellSpacing = style.ItemSpacing.x;

        // Header
        ImGui::Text(app_language::Tr(" Address    ", " Адрес    "));
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
            uint32_t lineOffset = lineAddr - baseAddress; // offset within the cache

            // Line address
            ImGui::Text("%04X", lineAddr);

            // HEX columns (from the cache)
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
                            // Write through the regular function (it queues the write)
                            DataReadWriteU08(ctx, nullptr, &v, &physAddr);
                            // Update the cache at once to show the new value
                            m_cache[byteOffset] = v;
                        }
                    }
                    ImGui::PopID();
                }
                else {
                    ImGui::Text("   ");
                }
            }

            // ASCII columns
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