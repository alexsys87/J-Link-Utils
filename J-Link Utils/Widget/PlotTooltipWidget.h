// PlotTooltipWidget.h
#pragma once
#include "widget_common.h"
#include <vector>

class PlotTooltipWidget : public UIWidget {
    std::string text, format;
    float minVal, maxVal;
    int sizeVal, bufferId;
    uint32_t address;
    bool littleEndian;
    int dataType;
    bool autosize;
    std::vector<float> m_displayBuf; // ˜˜˜˜˜ ˜ ˜˜˜˜

    float ReadValue(MainContext* ctx, uint32_t addr) const {
        switch (dataType) {
        case BOOLEAN: {
            uint8_t v = DataReadWriteU08(ctx, nullptr, nullptr, &addr);
            return v ? 1.0f : 0.0f;
        }
        case S08_DEC: {
            uint8_t v = DataReadWriteU08(ctx, nullptr, nullptr, &addr);
            return static_cast<float>(static_cast<int8_t>(v));
        }
        case U08_DEC: {
            uint8_t v = DataReadWriteU08(ctx, nullptr, nullptr, &addr);
            return static_cast<float>(v);
        }
        case S16_DEC: {
            uint16_t v = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) v = Swap16(v);
            return static_cast<float>(static_cast<int16_t>(v));
        }
        case U16_DEC: {
            uint16_t v = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) v = Swap16(v);
            return static_cast<float>(v);
        }
        case S32_DEC: {
            uint32_t v = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) v = SwapLeToBe32(v);
            return static_cast<float>(static_cast<int32_t>(v));
        }
        case U32_DEC: {
            uint32_t v = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) v = SwapLeToBe32(v);
            return static_cast<float>(v);
        }
        case FLOAT:
        default: {
            uint32_t tmp = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!littleEndian) tmp = SwapLeToBe32(tmp);
            float f;
            memcpy(&f, &tmp, sizeof(f));
            return f;
        }
        }
    }

public:
    PlotTooltipWidget(const char* txt, const char* fmt, float minV, float maxV,
        int sz, int id, uint32_t addr, bool le, int dtype, bool autoSz)
        : text(txt ? txt : ""), format(fmt ? fmt : ""), minVal(minV), maxVal(maxV),
        sizeVal(sz), bufferId(id), address(addr), littleEndian(le), dataType(dtype), autosize(autoSz) {
        if (sizeVal <= 0) sizeVal = 1;
        if (sizeVal > BUFF_SIZE) sizeVal = BUFF_SIZE;
        if (bufferId < 0 || bufferId >= MAX_BUFF) bufferId = 0;
        m_displayBuf.resize(sizeVal);
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        static int writePos[MAX_BUFF] = { 0 };

        uint32_t addr = address ? address : (dynamicAddr ? *dynamicAddr : 0);
        if (addr != 0 && bufferId >= 0 && bufferId < MAX_BUFF && sizeVal > 0 && sizeVal <= BUFF_SIZE) {
            float val = ReadValue(ctx, addr);
            VirtualBuffer[bufferId][writePos[bufferId]] = val;
            writePos[bufferId] = (writePos[bufferId] + 1) % sizeVal;
        }

        if (ImGui::IsItemHovered()) {
            if (bufferId < 0 || bufferId >= MAX_BUFF || sizeVal <= 0 || sizeVal > BUFF_SIZE) return;

            int wp = writePos[bufferId];
            // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
            int idx = 0;
            for (int i = wp; i < sizeVal; ++i) m_displayBuf[idx++] = VirtualBuffer[bufferId][i];
            for (int i = 0; i < wp; ++i) m_displayBuf[idx++] = VirtualBuffer[bufferId][i];

            int lastIdx = (wp - 1 + sizeVal) % sizeVal;
            float lastVal = VirtualBuffer[bufferId][lastIdx];
            char label[64];
            snprintf(label, sizeof(label), format.c_str(), lastVal);

            float plotMin = minVal, plotMax = maxVal;
            if (autosize) {
                plotMin = m_displayBuf[0];
                plotMax = m_displayBuf[0];
                for (int i = 1; i < sizeVal; ++i) {
                    if (m_displayBuf[i] < plotMin) plotMin = m_displayBuf[i];
                    if (m_displayBuf[i] > plotMax) plotMax = m_displayBuf[i];
                }
                float range = plotMax - plotMin;
                if (range < 1e-6f) range = 1.0f;
                plotMin -= range * 0.05f;
                plotMax += range * 0.05f;
            }

            ImGui::BeginTooltip();
            ImGui::Text("%s", text.c_str());
            ImGui::PlotLines(label, m_displayBuf.data(), sizeVal, 0, "", plotMin, plotMax, ImVec2(0, 0));
            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            ImGui::EndTooltip();
        }
    }
};