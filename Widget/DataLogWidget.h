// DataLogWidget.h
#pragma once
#include "widget_common.h"
#include "CaptureHandshake.h"
#include <vector>
#include <cstring>
#include <algorithm> // std::min/max

class DataLogWidget : public UIWidget {
public:
    struct LineConfig {
        std::string name;
        uint32_t baseAddr = 0;
        float yScale = 1.0f;
        ImU32 color = IM_COL32_WHITE;
        int dataType = FLOAT;      // element data type (see the constants in widget_common.h)
    };

private:
    std::string text;            // plot title
    std::string format;          // label format (average value)
    float minVal, maxVal;        // Y limits (when autosize=false)
    ImVec2 size;                 // plot size
    float xScale;                // X scale
    int bufferSize;              // number of points
    int smoothWindow;            // smoothing window
    uint32_t doneAddr;           // ready flag address
    bool littleEndian;           // byte order
    bool autosize;               // automatic Y limits

    std::vector<LineConfig> lines;            // line settings
    std::vector<std::vector<float>> data;     // data of each line (scaled floats)
    std::vector<std::vector<float>> smoothed; // smoothed data
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
    DataLogWidget(const char* txt, const char* fmt, float minV, float maxV,
        float w, float h, float xs, int sz, int sm,
        uint32_t addr, uint32_t done, bool le, float ys, ImU32 color, int dtype = FLOAT, bool autoSz = false)
        : text(txt ? txt : ""), format(fmt ? fmt : ""), minVal(minV), maxVal(maxV),
        size(w, h), xScale(xs), bufferSize(sz), smoothWindow(sm),
        doneAddr(done), littleEndian(le), autosize(autoSz) {
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
    DataLogWidget(const char* txt, const char* fmt, float minV, float maxV,
        float w, float h, float xs, int sz, int sm,
        uint32_t done, bool le, const std::vector<LineConfig>& lineList, bool autoSz = false)
        : text(txt ? txt : ""), format(fmt ? fmt : ""), minVal(minV), maxVal(maxV),
        size(w, h), xScale(xs), bufferSize(sz), smoothWindow(sm),
        doneAddr(done), littleEndian(le), autosize(autoSz), lines(lineList) {
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

        // X axis (shared by all lines)
        std::vector<float> x(bufferSize);
        for (int i = 0; i < bufferSize; i++) {
            x[i] = i * xScale;
        }

        // Y limits
        float yMin = minVal, yMax = maxVal;
        if (autosize) {
            // Global minimum and maximum over all lines
            yMin = std::numeric_limits<float>::max();
            yMax = std::numeric_limits<float>::lowest();
            for (size_t l = 0; l < lines.size(); ++l) {
                const float* plotData = (smoothWindow > 0) ? smoothed[l].data() : data[l].data();
                for (int i = 0; i < bufferSize; ++i) {
                    if (plotData[i] < yMin) yMin = plotData[i];
                    if (plotData[i] > yMax) yMax = plotData[i];
                }
            }
            // 5% margin so points do not touch the edges
            float range = yMax - yMin;
            if (range < 1e-6f) range = 1.0f; // guard against a zero range
            yMin -= range * 0.05f;
            yMax += range * 0.05f;
        }

        // Plot
        if (ImPlot::BeginPlot(text.c_str(), size, ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, nullptr);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, (bufferSize - 1) * xScale, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, yMin, yMax, ImGuiCond_Always);

            for (size_t l = 0; l < lines.size(); ++l) {
                const auto& line = lines[l];
                const float* plotData = (smoothWindow > 0) ? smoothed[l].data() : data[l].data();

                // Label
                char label[64];
                if (!line.name.empty()) {
                    snprintf(label, sizeof(label), "%s", line.name.c_str());
                }
                else {
                    float avg = ::average({ plotData, plotData + bufferSize });
                    ui_safety::FormatScalar(label, sizeof(label), format.c_str(), avg);
                }

                // Line color
                ImPlotSpec spec;
                spec.LineColor = ImGui::ColorConvertU32ToFloat4(line.color);
                ImPlot::PlotLine(label, x.data(), plotData, bufferSize, spec);
            }
            ImPlot::EndPlot();
        }
    }

private:
    void InitBuffers() {
        bufferSize = std::clamp(bufferSize, 1, static_cast<int>(BUFF_SIZE));
        data.resize(lines.size());
        smoothed.resize(lines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            data[i].assign(bufferSize, 0.0f);
            smoothed[i].assign(bufferSize, 0.0f);
        }
    }
};