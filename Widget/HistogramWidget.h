// HistogramWidget.h
#pragma once
#include "widget_common.h"
#include "CaptureHandshake.h"
#include <vector>
#include <cstring>

class HistogramWidget : public UIWidget {
public:
    struct LineConfig {
        std::string name;
        uint32_t baseAddr = 0;
        float yScale = 1.0f;
        ImU32 color = IM_COL32_WHITE;
        int dataType = FLOAT;
    };

private:
    std::string text;
    std::string format;
    float minVal, maxVal;
    ImVec2 size;
    float xScale;
    int bufferSize;
    int smoothWindow;
    uint32_t doneAddr;
    bool littleEndian;
    bool autoSize;
    float barWidth;                     // width of one bar (fraction of the X step)

    std::vector<LineConfig> lines;
    std::vector<std::vector<float>> data;      // raw data after scaling
    std::vector<std::vector<float>> smoothed;  // smoothed data
    CaptureHandshake m_capture;           // DoneAddr handshake state

    // Element size in bytes for a data type
    static int GetElementSize(int dataType) {
        return typed_value::Size(dataType);
    }

    // Converts raw bytes to float using the data type and byte order
    float RawToFloat(const uint8_t* bytes, int dataType, bool le) const {
        return (float)typed_value::DecodeBytes(bytes, dataType, le);
    }

public:
    // Single-line constructor (old format)
    HistogramWidget(const char* txt, const char* fmt, float minV, float maxV,
        float w, float h, float xs, int sz, int sm,
        uint32_t done, bool le, bool autoSz, float bw,
        uint32_t addr, float ys, ImU32 color, int dtype = FLOAT)
        : text(txt ? txt : ""), format(fmt ? fmt : ""), minVal(minV), maxVal(maxV),
        size(w, h), xScale(xs), bufferSize(sz), smoothWindow(sm),
        doneAddr(done), littleEndian(le), autoSize(autoSz), barWidth(bw) {
        LineConfig line;
        line.name = txt ? txt : "";
        line.baseAddr = addr;
        line.yScale = ys;
        line.color = color;
        line.dataType = dtype;
        lines.push_back(line);
        InitBuffers();
    }

    // Multi-line constructor
    HistogramWidget(const char* txt, const char* fmt, float minV, float maxV,
        float w, float h, float xs, int sz, int sm,
        uint32_t done, bool le, bool autoSz, float bw,
        const std::vector<LineConfig>& lineList)
        : text(txt ? txt : ""), format(fmt ? fmt : ""), minVal(minV), maxVal(maxV),
        size(w, h), xScale(xs), bufferSize(sz), smoothWindow(sm),
        doneAddr(done), littleEndian(le), autoSize(autoSz), barWidth(bw),
        lines(lineList) {
        InitBuffers();
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        if (!ctx || !ctx->IOMAP_Data || bufferSize > BUFF_SIZE) return;

        // The arrays are taken from the cache only as a complete capture (see CaptureHandshake.h).
        const bool fresh = m_capture.Update(ctx, doneAddr, [&] {
            for (const auto& line : lines) {
                uint32_t addr = line.baseAddr;
                DataReadWriteArray(ctx, nullptr, nullptr, &addr, (uint16_t)(bufferSize * GetElementSize(line.dataType)));
            }
        });
        if (fresh) {
            for (size_t l = 0; l < lines.size(); ++l) {
                const auto& line = lines[l];
                const int elementSize = GetElementSize(line.dataType);
                const size_t bytesToRead = static_cast<size_t>(bufferSize) * elementSize;
                SDL_LockMutex(ctx->Datalock);
                if (!ui_safety::IsMappedRange(line.baseAddr, ctx->mcu_memory_base, bytesToRead, IOMAP_SIZE)) {
                    std::fill(data[l].begin(), data[l].end(), 0.0f);
                }
                else {
                    const uint32_t offset = line.baseAddr - ctx->mcu_memory_base;
                    for (int i = 0; i < bufferSize; ++i)
                        data[l][i] = RawToFloat(&ctx->IOMAP_Data[offset + i * elementSize], line.dataType, littleEndian) * line.yScale;
                }
                SDL_UnlockMutex(ctx->Datalock);
            }
        }

        // Smoothing, if enabled
        if (smoothWindow > 0) {
            for (size_t l = 0; l < lines.size(); ++l) {
                smoothed[l] = smooth(data[l], smoothWindow);
            }
        }

        // X axis
        std::vector<double> x(bufferSize);
        for (int i = 0; i < bufferSize; i++) {
            x[i] = i * xScale;
        }

        // Y limits (with AutoSize)
        float plotMin = minVal, plotMax = maxVal;
        if (autoSize) {
            plotMin = FLT_MAX;
            plotMax = -FLT_MAX;
            for (size_t l = 0; l < lines.size(); ++l) {
                const float* d = (smoothWindow > 0) ? smoothed[l].data() : data[l].data();
                for (int i = 0; i < bufferSize; ++i) {
                    if (d[i] < plotMin) plotMin = d[i];
                    if (d[i] > plotMax) plotMax = d[i];
                }
            }
            float range = plotMax - plotMin;
            if (range < 1e-6f) range = 1.0f;
            plotMin -= range * 0.05f;
            plotMax += range * 0.05f;
        }

        // Grouped bar chart
        if (ImPlot::BeginPlot(text.c_str(), size, ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, nullptr);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0 - barWidth, (bufferSize - 1) * xScale + barWidth, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, plotMin, plotMax, ImGuiCond_Always);

            int nLines = (int)lines.size();
            for (int l = 0; l < nLines; ++l) {
                const auto& line = lines[l];
                const float* plotData = (smoothWindow > 0) ? smoothed[l].data() : data[l].data();

                // Offset of this line within the group
                double offset = (l - (nLines - 1) / 2.0) * barWidth;

                // Label
                char label[64];
                if (!line.name.empty()) {
                    snprintf(label, sizeof(label), "%s", line.name.c_str());
                }
                else {
                    float avg = ::average({ plotData, plotData + bufferSize });  // average() takes a vector
                    ui_safety::FormatScalar(label, sizeof(label), format.c_str(), avg);
                }

                // Color
                ImPlotSpec spec;
                spec.LineColor = ImGui::ColorConvertU32ToFloat4(line.color);
                // Bars
                ImPlot::PlotBars(label, plotData, bufferSize, barWidth, offset, spec);
            }
            ImPlot::EndPlot();
        }
    }

private:
    void InitBuffers() {
        bufferSize = std::clamp(bufferSize, 1, static_cast<int>(BUFF_SIZE));
        smoothWindow = std::clamp(smoothWindow, 0, bufferSize);   // a wider window only costs time
        data.resize(lines.size());
        smoothed.resize(lines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            data[i].assign(bufferSize, 0.0f);
            smoothed[i].assign(bufferSize, 0.0f);
        }
    }
};