// HistogramWidget.h
#pragma once
#include "widget_common.h"
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
    float barWidth;                     // ширина одного столбца (в долях от шага по X)

    std::vector<LineConfig> lines;
    std::vector<std::vector<float>> data;      // сырые данные после масштабирования
    std::vector<std::vector<float>> smoothed;  // сглаженные данные

    // Размер элемента в байтах для данного типа данных
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
        default:        return 4;
        }
    }

    // Преобразование сырых байт в float с учётом типа и порядка байт
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
    // Конструктор для одной линии (старый стиль)
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

    // Конструктор для нескольких линий
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
        if (bufferSize > BUFF_SIZE) return;

        // Проверка флага готовности
        bool ready = true;
        if (doneAddr != 0) {
            uint32_t doneVal = DataReadWriteU32(ctx, nullptr, nullptr, &doneAddr);
            ready = (doneVal != 0);
        }

        // Если флаг готовности установлен, инициируем чтение всех массивов
        if (ctx->StartComunication && ready) {
            for (size_t l = 0; l < lines.size(); ++l) {
                const auto& line = lines[l];
                int elementSize = GetElementSize(line.dataType);
                DataReadWriteArray(ctx, nullptr, nullptr, (uint32_t*)&line.baseAddr, bufferSize * elementSize);
            }

            // Сброс флага готовности
            if (doneAddr != 0) {
                uint32_t zero = 0;
                DataReadWriteU32(ctx, nullptr, &zero, &doneAddr);
            }
        }

        // Обновление внутренних буферов из IOMAP_Data
        for (size_t l = 0; l < lines.size(); ++l) {
            const auto& line = lines[l];
            int elementSize = GetElementSize(line.dataType);
            uint32_t offset = line.baseAddr - ctx->mcu_memory_base;

            SDL_LockMutex(ctx->Datalock);
            for (int i = 0; i < bufferSize; ++i) {
                uint32_t byteOffset = offset + i * elementSize;
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&ctx->IOMAP_Data[byteOffset]);
                float val = RawToFloat(bytes, line.dataType, littleEndian);
                data[l][i] = val * line.yScale;
            }
            SDL_UnlockMutex(ctx->Datalock);
        }

        // Применяем сглаживание, если нужно
        if (smoothWindow > 0) {
            for (size_t l = 0; l < lines.size(); ++l) {
                smoothed[l] = smooth(data[l], smoothWindow);
            }
        }

        // Подготовка оси X
        std::vector<double> x(bufferSize);
        for (int i = 0; i < bufferSize; i++) {
            x[i] = i * xScale;
        }

        // Определяем пределы Y (если AutoSize)
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

        // Построение гистограммы с группированными столбцами
        if (ImPlot::BeginPlot(text.c_str(), size, ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, nullptr);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0 - barWidth, (bufferSize - 1) * xScale + barWidth, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, plotMin, plotMax, ImGuiCond_Always);

            int nLines = (int)lines.size();
            for (int l = 0; l < nLines; ++l) {
                const auto& line = lines[l];
                const float* plotData = (smoothWindow > 0) ? smoothed[l].data() : data[l].data();

                // Вычисляем смещение для группировки
                double offset = (l - (nLines - 1) / 2.0) * barWidth;

                // Формируем подпись
                char label[64];
                if (!line.name.empty()) {
                    snprintf(label, sizeof(label), "%s", line.name.c_str());
                }
                else {
                    float avg = ::average({ plotData, plotData + bufferSize });  // если average принимает vector
                    snprintf(label, sizeof(label), format.c_str(), avg);
                }

                // Устанавливаем цвет
                ImPlotSpec spec;
                spec.LineColor = ImVec4(
                    ((line.color >> 16) & 0xFF) / 255.0f,
                    ((line.color >> 8) & 0xFF) / 255.0f,
                    (line.color & 0xFF) / 255.0f,
                    ((line.color >> 24) & 0xFF) / 255.0f
                );
                // Рисуем столбцы
                ImPlot::PlotBars(label, plotData, bufferSize, barWidth, offset, spec);
            }
            ImPlot::EndPlot();
        }
    }

private:
    void InitBuffers() {
        data.resize(lines.size());
        smoothed.resize(lines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            data[i].assign(bufferSize, 0.0f);
            smoothed[i].assign(bufferSize, 0.0f);
        }
    }
};