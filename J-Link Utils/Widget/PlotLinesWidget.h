// PlotLinesWidget.h
// ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜, ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜ ˜˜˜˜˜ GUI

#pragma once
#include "widget_common.h"
#include "implot.h"
#include <vector>
#include <cstdio>
#include <float.h>

// ˜˜˜˜˜˜˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ (˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜)
struct ScrollingData {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;

    ScrollingData(int maxSize = 2000) : MaxSize(maxSize > 0 ? maxSize : 1), Offset(0) {
        Data.reserve(MaxSize);
    }

    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }

    void Erase() {
        if (Data.size() > 0) {
            Data.clear();
            Offset = 0;
        }
    }
};

// ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜
struct PlotLineConfig {
    std::string name;           // ˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜
    uint32_t address;           // ˜˜˜˜˜ ˜ ˜˜˜˜˜˜
    int dataType;               // ˜˜˜ ˜˜˜˜˜˜ (˜˜. enum ˜ widget_common.h)
    bool littleEndian;
    float scale;                // ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜
    int bufferId;               // ˜˜˜˜˜˜ ˜ VirtualBuffer
    ImU32 color;                 // ˜˜˜˜ ˜˜˜˜˜
    float weight;                // ˜˜˜˜˜˜˜

    // ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
    std::vector<float> smoothBuf;
    ScrollingData history;

    PlotLineConfig() : address(0), dataType(U32_DEC), littleEndian(false), scale(1.0f),
        bufferId(-1), color(IM_COL32_WHITE), weight(1.0f) {
    }
};

class PlotLinesWidget : public UIWidget {
    std::string text;            // ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
    float minVal, maxVal;        // ˜˜˜˜˜˜˜ Y (˜˜˜˜ ˜˜˜˜˜˜)
    ImVec2 size;                 // ˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ (˜˜˜˜ ˜˜ ˜˜˜˜)
    float freq;                  // ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ (˜˜) ˜ ˜˜˜˜˜˜˜˜˜˜
    int bufferSize;              // ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ (Size)
    int smoothWindow;            // ˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜
    bool autoSize;               // ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
    int m_maxSize;               // ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜˜ (MaxSize)

    std::vector<PlotLineConfig> m_lines;  // ˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜˜

    struct PlotState {
        double time;             // ˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜˜
        bool paused;             // ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜ (˜˜˜˜˜ ˜˜˜ ˜˜˜˜˜)
        bool pause_check;        // ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜ "Real Time Update"
        bool update;             // ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ (˜˜˜˜˜˜ true)
        double refresh_time;      // ˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
        float timeWindow;         // ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜ ˜˜˜ X (˜˜˜˜˜˜˜)

        PlotState() : time(0), paused(false), pause_check(true), update(true), refresh_time(0), timeWindow(10.0f) {}
    } state;

    // ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜
    float ReadValue(MainContext* ctx, uint32_t addr, int dataType, bool le) {
        switch (dataType) {
        case BOOLEAN: return DataReadWriteBoolean(ctx, nullptr, nullptr, &addr) ? 1.0f : 0.0f;
        case S08_DEC: return (float)(int8_t)DataReadWriteU08(ctx, nullptr, nullptr, &addr);
        case U08_DEC: return (float)DataReadWriteU08(ctx, nullptr, nullptr, &addr);
        case S16_DEC: {
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (!le) raw = (raw >> 8) | (raw << 8);
            return (float)(int16_t)raw;
        }
        case U16_DEC: {
            uint16_t raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr);
            if (!le) raw = (raw >> 8) | (raw << 8);
            return (float)raw;
        }
        case S32_DEC: {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!le) raw = SwapLeToBe32(raw);
            return (float)(int32_t)raw;
        }
        case U32_DEC: {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!le) raw = SwapLeToBe32(raw);
            return (float)raw;
        }
        case S64_DEC: {
            uint64_t raw = DataReadWriteU64(ctx, nullptr, nullptr, &addr);
            if (!le) raw = SwapLeToBe64(raw);
            return (float)(int64_t)raw;
        }
        case U64_DEC: {
            uint64_t raw = DataReadWriteU64(ctx, nullptr, nullptr, &addr);
            if (!le) raw = SwapLeToBe64(raw);
            return (float)raw;
        }
        case FLOAT: {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (!le) raw = SwapLeToBe32(raw);
            return *(float*)&raw;
        }
        case FLOAT_INV: {
            uint32_t raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
            if (le) raw = SwapLeToBe32(raw);
            return *(float*)&raw;
        }
        default: return 0.0f;
        }
    }

public:
    // ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜ (˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜)
    PlotLinesWidget(
        const char* txt, const char* fmt, float minV, float maxV,
        float w, float h, float f, float sc,
        int sz, int sm, int id, bool autosize,
        int dtype, bool le, uint32_t addr = 0,
        ImU32 lineColor = IM_COL32(255, 255, 255, 255), float lineWeight = 1.0f,
        int maxSize = 2000)
        : text(txt ? txt : ""), minVal(minV), maxVal(maxV),
        size(w, h), freq(f), bufferSize(sz), smoothWindow(sm), autoSize(autosize),
        m_maxSize(maxSize)
    {
        if (bufferSize <= 0) bufferSize = 100;
        if (bufferSize > BUFF_SIZE) bufferSize = BUFF_SIZE;
        if (freq <= 0.0f) freq = 1.0f;
        PlotLineConfig line;
        line.name = txt ? txt : "";
        line.address = addr;
        line.dataType = dtype;
        line.littleEndian = le;
        line.scale = sc;
        line.bufferId = (id < 0 || id >= MAX_BUFF) ? 0 : id;
        line.color = lineColor;
        line.weight = lineWeight;
        line.smoothBuf.resize(bufferSize);
        line.history = ScrollingData(m_maxSize);
        m_lines.push_back(line);
    }

    // ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜
    PlotLinesWidget(
        const char* txt, float minV, float maxV,
        float w, float h, float f, int sz, int sm, bool autosize, int maxSize,
        const std::vector<PlotLineConfig>& lines)
        : text(txt ? txt : ""), minVal(minV), maxVal(maxV),
        size(w, h), freq(f), bufferSize(sz), smoothWindow(sm), autoSize(autosize),
        m_maxSize(maxSize), m_lines(lines)
    {
        if (bufferSize <= 0) bufferSize = 100;
        if (bufferSize > BUFF_SIZE) bufferSize = BUFF_SIZE;
        if (freq <= 0.0f) freq = 1.0f;
        for (auto& line : m_lines) {
            if (line.bufferId < 0 || line.bufferId >= MAX_BUFF) line.bufferId = 0;
            line.smoothBuf.resize(bufferSize);
            line.history = ScrollingData(m_maxSize);
        }
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (!ctx->StartComunication) return;

        // ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜ ˜˜˜˜ ˜˜˜˜˜ (˜˜˜˜ ˜˜˜˜˜˜˜˜)
        if (state.update && state.pause_check) {
            if (state.refresh_time < ImGui::GetTime()) {
                for (auto& line : m_lines) {
                    uint32_t addr = line.address;
                    if (addr == 0) continue;

                    float value = ReadValue(ctx, addr, line.dataType, line.littleEndian) * line.scale;

                    // ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜
                    float* data = VirtualBuffer[line.bufferId];
                    for (int i = 0; i < bufferSize - 1; ++i)
                        data[i] = data[i + 1];
                    data[bufferSize - 1] = value;

                    // ˜˜˜˜˜˜˜˜˜˜˜ (˜˜˜˜ ˜˜˜˜˜)
                    float* displayData = data;
                    if (smoothWindow > 0) {
                        // ˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
                        std::vector<float> input(data, data + bufferSize);
                        // ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜ ˜˜˜˜˜ ˜˜˜˜˜
                        line.smoothBuf = smooth(input, smoothWindow);
                        displayData = line.smoothBuf.data();
                    }

                    // ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
                    if (!state.paused) {
                        state.time += ImGui::GetIO().DeltaTime;
                        line.history.AddPoint((float)state.time, displayData[bufferSize - 1]);
                    }
                }
                state.refresh_time = ImGui::GetTime() + 1.0 / (double)freq;
            }
        }

        // ˜˜˜˜˜˜ ˜˜˜˜˜
        if (ImGui::CollapsingHeader("Options")) {
            ImGui::PushItemWidth(80);
            ImGui::Checkbox("Real Time Update", &state.pause_check);
            ImGui::SameLine();
            // ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
            float minFreq = 0.1f, maxFreq = 60.0f;
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputFloat("Freq (Hz)", &freq, 0.1f, 1.0f, "%.1f")) {
                if (freq < minFreq) freq = minFreq;
                if (freq > maxFreq) freq = maxFreq;
                // ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜, ˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜
                state.refresh_time = ImGui::GetTime();
            }
            ImGui::SameLine();
            // ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜
            ImGui::SetNextItemWidth(500);
            ImGui::DragFloat("Time Window (s)", &state.timeWindow, 0.1f, 0.1f, 100.0f, "%.1f");
            ImGui::PopItemWidth();
        }

        // ˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
        ImVec2 plotSize = autoSize ? ImGui::GetContentRegionAvail() : size;
        if (plotSize.x < 10 && plotSize.x > 0) plotSize.x = 200;
        if (plotSize.y < 10 && plotSize.y > 0) plotSize.y = 100;

        // ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜ Y, ˜˜˜˜ minVal/maxVal ˜˜ ˜˜˜˜˜˜
        float ymin = minVal, ymax = maxVal;
        if (ymin >= ymax) {
            ymin = FLT_MAX;
            ymax = -FLT_MAX;
            for (const auto& line : m_lines) {
                for (const auto& pt : line.history.Data) {
                    if (pt.y < ymin) ymin = pt.y;
                    if (pt.y > ymax) ymax = pt.y;
                }
            }
            if (ymin > ymax) { ymin = 0; ymax = 1; }
            if (ymin == ymax) { ymin -= 1; ymax += 1; }
        }

        // ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜
        if (ImPlot::BeginPlot(text.c_str(), plotSize, ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, nullptr);
            // ˜˜˜˜˜˜˜˜˜˜ state.timeWindow ˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜ X
            ImPlot::SetupAxisLimits(ImAxis_X1, state.time - state.timeWindow, state.time,
                state.paused ? ImGuiCond_Once : ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, ymin, ymax,
                state.paused ? ImGuiCond_Once : ImGuiCond_Always);

            for (const auto& line : m_lines) {
                if (line.history.Data.size() == 0) continue;

                ImPlotSpec spec;
                spec.Offset = line.history.Offset;
                spec.Stride = (int)sizeof(ImVec2);
                spec.LineColor = ImVec4(
                    ((line.color >> 16) & 0xFF) / 255.0f,
                    ((line.color >> 8) & 0xFF) / 255.0f,
                    (line.color & 0xFF) / 255.0f,
                    ((line.color >> 24) & 0xFF) / 255.0f
                );
                //spec.LineWeight = line.weight;

                const char* label = line.name.empty() ? "Line" : line.name.c_str();
                ImPlot::PlotLine(label,
                    &line.history.Data[0].x,
                    &line.history.Data[0].y,
                    (int)line.history.Data.size(), spec);
            }
            ImPlot::EndPlot();
        }

        state.paused = !ctx->StartComunication || !state.pause_check;
    }
};