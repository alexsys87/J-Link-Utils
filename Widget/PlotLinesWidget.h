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
    double nextSampleTime;      // next accepted sample time (frequency limit)
    int smoothPos, smoothCount; // moving average ring position and fill
    double smoothSum;

    PlotLineConfig() : address(0), dataType(U32_DEC), littleEndian(true), scale(1.0f),
        bufferId(-1), color(IM_COL32_WHITE), weight(1.0f),
        nextSampleTime(0), smoothPos(0), smoothCount(0), smoothSum(0) {
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

    static constexpr float kMaxFreq = 1000.0f;     // samples per second per line
    static constexpr int kMaxHistory = 200000;     // points kept per line

    std::vector<DataSample> m_samples;             // scratch buffer for DataReadSamples
    double m_timeOrigin = -1.0;                    // DataTimeNow() at the first draw

    static int SampleSize(int dataType) {
        return typed_value::Size(dataType);
    }

    // Converts raw target bytes (memory order) like the typed readers do.
    static float DecodeSample(const uint8_t* bytes, int dataType, bool le) {
        return (float)typed_value::DecodeBytes(bytes, dataType, le);
    }

    // Trailing moving average over the last smoothWindow points.
    float Smooth(PlotLineConfig& line, float value) const {
        if (smoothWindow <= 1) return value;
        if (line.smoothCount == smoothWindow) line.smoothSum -= line.smoothBuf[line.smoothPos];
        else ++line.smoothCount;
        line.smoothBuf[line.smoothPos] = value;
        line.smoothSum += value;
        line.smoothPos = (line.smoothPos + 1) % smoothWindow;
        return (float)(line.smoothSum / line.smoothCount);
    }

    // The history must hold the whole time window at the sampling frequency.
    void ResizeHistory() {
        const double needed = std::ceil((double)freq * state.timeWindow) + 16.0;
        const int capacity = (int)std::min(std::max(needed, (double)m_maxSize), (double)kMaxHistory);
        for (auto& line : m_lines)
            if (line.history.MaxSize != capacity) line.history = ScrollingData(capacity);
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
        if (freq > kMaxFreq) freq = kMaxFreq;
        smoothWindow = std::clamp(smoothWindow, 0, bufferSize);
        PlotLineConfig line;
        line.name = txt ? txt : "";
        line.address = addr;
        line.dataType = dtype;
        line.littleEndian = le;
        line.scale = sc;
        line.bufferId = (id < 0 || id >= MAX_BUFF) ? 0 : id;
        line.color = lineColor;
        line.weight = lineWeight;
        line.smoothBuf.assign(std::max(smoothWindow, 1), 0.0f);
        line.history = ScrollingData(m_maxSize);
        m_lines.push_back(line);
        ResizeHistory();
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
        if (freq > kMaxFreq) freq = kMaxFreq;
        smoothWindow = std::clamp(smoothWindow, 0, bufferSize);
        for (auto& line : m_lines) {
            if (line.bufferId < 0 || line.bufferId >= MAX_BUFF) line.bufferId = 0;
            line.smoothBuf.assign(std::max(smoothWindow, 1), 0.0f);
            line.history = ScrollingData(m_maxSize);
        }
        ResizeHistory();
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (!ctx->StartComunication) return;

        // Points come from the communication thread with their own timestamps,
        // one per polling cycle, thinned out to the configured frequency.
        if (m_timeOrigin < 0) m_timeOrigin = DataTimeNow();
        const bool live = state.update && state.pause_check;
        const double period = 1.0 / (double)freq;
        for (auto& line : m_lines) {
            if (line.address == 0) continue;
            DataReadSamples(ctx, &line, line.address, (uint16_t)SampleSize(line.dataType), m_samples);
            if (!live) continue;  // samples taken while paused are dropped
            for (const DataSample& sample : m_samples) {
                if (sample.time < line.nextSampleTime) continue;
                line.nextSampleTime += period;
                if (line.nextSampleTime <= sample.time) line.nextSampleTime = sample.time + period;
                const float value = Smooth(line, DecodeSample(sample.bytes, line.dataType, line.littleEndian) * line.scale);
                line.history.AddPoint((float)(sample.time - m_timeOrigin), value);
            }
        }
        if (live) state.time = DataTimeNow() - m_timeOrigin;

        // ˜˜˜˜˜˜ ˜˜˜˜˜
        if (ImGui::CollapsingHeader("Options")) {
            ImGui::PushItemWidth(80);
            ImGui::Checkbox("Real Time Update", &state.pause_check);
            ImGui::SameLine();
            // ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
            float minFreq = 0.1f, maxFreq = kMaxFreq;
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputFloat("Freq (Hz)", &freq, 0.1f, 1.0f, "%.1f")) {
                if (freq < minFreq) freq = minFreq;
                if (freq > maxFreq) freq = maxFreq;
                ResizeHistory();
            }
            ImGui::SameLine();
            // ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜
            ImGui::SetNextItemWidth(500);
            if (ImGui::DragFloat("Time Window (s)", &state.timeWindow, 0.1f, 0.1f, 100.0f, "%.1f"))
                ResizeHistory();
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
                spec.LineColor = ImGui::ColorConvertU32ToFloat4(line.color);
                spec.LineWeight = line.weight;

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