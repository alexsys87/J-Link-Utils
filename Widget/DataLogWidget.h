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
        switch (dataType) {
        case BOOLEAN:
        case S08_DEC:
        case U08_DEC:   return 1;
        case S16_DEC:
        case U16_DEC:   return 2;
        case S32_DEC:
        case U32_DEC:
        case FLOAT:     return 4;
        case S64_DEC:
        case U64_DEC:   return 8;
        default:        return 4; // float by default
        }
    }

    // Converts raw bytes to float using the data type and byte order
    float RawToFloat(const uint8_t* bytes, int dataType, bool le) const {
        switch (dataType) {
        case BOOLEAN: return (bytes[0] & 1) ? 1.0f : 0.0f;
        case S08_DEC: return static_cast<float>(static_cast<int8_t>(bytes[0]));
        case U08_DEC: return static_cast<float>(bytes[0]);
        case S16_DEC: {
            uint16_t v = le ? (bytes[0] | (bytes[1] << 8)) : (bytes[1] | (bytes[0] << 8));
            return static_cast<float>(static_cast<int16_t>(v));
        }
        case U16_DEC: {
            uint16_t v = le ? (bytes[0] | (bytes[1] << 8)) : (bytes[1] | (bytes[0] << 8));
            return static_cast<float>(v);
        }
        case S32_DEC: {
            uint32_t v;
            if (le) v = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            else    v = bytes[3] | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
            return static_cast<float>(static_cast<int32_t>(v));
        }
        case U32_DEC: {
            uint32_t v;
            if (le) v = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            else    v = bytes[3] | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
            return static_cast<float>(v);
        }
        case FLOAT: {
            uint32_t v;
            if (le) v = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            else    v = bytes[3] | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
            return *reinterpret_cast<float*>(&v);
        }
        case S64_DEC: {
            uint64_t v;
            if (le) {
                v = (uint64_t)bytes[0] | ((uint64_t)bytes[1] << 8) | ((uint64_t)bytes[2] << 16) | ((uint64_t)bytes[3] << 24) |
                    ((uint64_t)bytes[4] << 32) | ((uint64_t)bytes[5] << 40) | ((uint64_t)bytes[6] << 48) | ((uint64_t)bytes[7] << 56);
            }
            else {
                v = (uint64_t)bytes[7] | ((uint64_t)bytes[6] << 8) | ((uint64_t)bytes[5] << 16) | ((uint64_t)bytes[4] << 24) |
                    ((uint64_t)bytes[3] << 32) | ((uint64_t)bytes[2] << 40) | ((uint64_t)bytes[1] << 48) | ((uint64_t)bytes[0] << 56);
            }
            return static_cast<float>(static_cast<int64_t>(v));
        }
        case U64_DEC: {
            uint64_t v;
            if (le) {
                v = (uint64_t)bytes[0] | ((uint64_t)bytes[1] << 8) | ((uint64_t)bytes[2] << 16) | ((uint64_t)bytes[3] << 24) |
                    ((uint64_t)bytes[4] << 32) | ((uint64_t)bytes[5] << 40) | ((uint64_t)bytes[6] << 48) | ((uint64_t)bytes[7] << 56);
            }
            else {
                v = (uint64_t)bytes[7] | ((uint64_t)bytes[6] << 8) | ((uint64_t)bytes[5] << 16) | ((uint64_t)bytes[4] << 24) |
                    ((uint64_t)bytes[3] << 32) | ((uint64_t)bytes[2] << 40) | ((uint64_t)bytes[1] << 48) | ((uint64_t)bytes[0] << 56);
            }
            return static_cast<float>(v);
        }
        default: return 0.0f;
        }
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