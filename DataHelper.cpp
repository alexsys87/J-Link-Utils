// DataHelper.cpp
// Версия с преобразованием физических адресов в смещения для IOMAP_Data
// Все операции записи/чтения используют единый механизм через DataWriteArray/DataReadArray

#include <Main.h>
#include <cstring>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cctype>
#include <mutex>
#include <algorithm>

using namespace tinyxml2;

// Проверка, что физический адрес лежит в диапазоне памяти MCU
#define CHECK_PHYSICAL_ADDR(addr, size) \
    do { \
        if ((addr) == 0 || (addr) < ctx->mcu_memory_base) return 0; \
        if ((uint64_t)(addr) + (uint64_t)(size) > (uint64_t)ctx->mcu_memory_base + (uint64_t)IOMAP_SIZE || \
            (uint64_t)(addr) + (uint64_t)(size) > UINT64_C(0x100000000)) return 0; \
    } while (0)

// Поиск дочернего элемента по имени
XMLElement* TryFindElement(XMLElement* e, const char* name)
{
    if (!e || !name) return nullptr;
    for (XMLElement* elem = e->FirstChildElement(); elem; elem = elem->NextSiblingElement())
    {
        if (strcmp(elem->Name(), name) == 0)
            return elem;
    }
    return nullptr;
}

// Преобразование строки вида "#01A4" в целое число (hex)
uint32_t AddressFromHex(XMLElement* e)
{
    if (!e || !e->GetText()) return 0;
    const char* text = e->GetText();
    while (std::isspace(static_cast<unsigned char>(*text))) ++text;
    int base = 0;
    if (*text == '#') { ++text; base = 16; }
    if (*text == '-') return 0;
    errno = 0;
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, base);
    if (end == text || errno == ERANGE || value > UINT32_MAX) return 0;
    while (std::isspace(static_cast<unsigned char>(*end))) ++end;
    return *end == '\0' ? static_cast<uint32_t>(value) : 0;
}

// Protect the bundled queue's node reclamation from concurrent access.
static std::mutex g_queueMutex;

#ifdef __cplusplus
extern "C" {
#endif

    uint16_t DataWriteArray(MainContext* ctx, uint32_t phys_addr, const uint8_t* data, uint16_t size);
    uint16_t DataReadArray(MainContext* ctx, uint32_t phys_addr, uint16_t size);

    // ------------------------------------------------------------------
    // Запись (помещают запрос в tx_Queue)
    // Принимают физический адрес, преобразуют в смещение
    // ------------------------------------------------------------------

    bool DataWriteBolean(MainContext* ctx, uint8_t* val, uint32_t phys_addr)
    {
        if (!ctx) return false;
        CHECK_PHYSICAL_ADDR(phys_addr, 1);
        uint8_t v = val ? (*val & 0x01) : 0;
        return DataWriteArray(ctx, phys_addr, &v, 1) == 1;
    }

    uint8_t DataWriteU08(MainContext* ctx, uint8_t* val, uint32_t phys_addr)
    {
        if (!ctx) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 1);
        uint8_t v = val ? *val : 0;
        DataWriteArray(ctx, phys_addr, &v, 1);
        return v;
    }

    uint16_t DataWriteU16(MainContext* ctx, uint16_t* val, uint32_t phys_addr)
    {
        if (!ctx) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 2);
        uint16_t v = val ? *val : 0;
        uint8_t buf[2];
        buf[0] = v & 0xFF;
        buf[1] = (v >> 8) & 0xFF;
        DataWriteArray(ctx, phys_addr, buf, 2);
        return v;
    }

    uint32_t DataWriteU32(MainContext* ctx, uint32_t* val, uint32_t phys_addr)
    {
        if (!ctx) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 4);
        uint32_t v = val ? *val : 0;
        uint8_t buf[4];
        buf[0] = v & 0xFF;
        buf[1] = (v >> 8) & 0xFF;
        buf[2] = (v >> 16) & 0xFF;
        buf[3] = (v >> 24) & 0xFF;
        DataWriteArray(ctx, phys_addr, buf, 4);
        return v;
    }

    float DataWriteFT32(MainContext* ctx, float* val, uint32_t phys_addr)
    {
        if (!ctx) return 0.0f;
        CHECK_PHYSICAL_ADDR(phys_addr, 4);
        FT32 f;
        const float value = val ? *val : 0.0f;
        memcpy(&f.hex, &value, sizeof(value));
        uint8_t buf[4];
        buf[0] = f.hex & 0xFF;
        buf[1] = (f.hex >> 8) & 0xFF;
        buf[2] = (f.hex >> 16) & 0xFF;
        buf[3] = (f.hex >> 24) & 0xFF;
        DataWriteArray(ctx, phys_addr, buf, 4);
        return value;
    }

    uint64_t DataWriteU64(MainContext* ctx, uint64_t* val, uint32_t phys_addr)
    {
        if (!ctx) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 8);
        uint64_t v = val ? *val : 0;
        uint8_t buf[8];
        buf[0] = v & 0xFF;
        buf[1] = (v >> 8) & 0xFF;
        buf[2] = (v >> 16) & 0xFF;
        buf[3] = (v >> 24) & 0xFF;
        buf[4] = (v >> 32) & 0xFF;
        buf[5] = (v >> 40) & 0xFF;
        buf[6] = (v >> 48) & 0xFF;
        buf[7] = (v >> 56) & 0xFF;
        DataWriteArray(ctx, phys_addr, buf, 8);
        return v;
    }

    // Запись массива данных
    uint16_t DataWriteArray(MainContext* ctx, uint32_t phys_addr, const uint8_t* data, uint16_t size)
    {
        if (!ctx || !ctx->StartComunication || !data || size == 0) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, size);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        iodata_t* io_mem = (iodata_t*)SDL_malloc(sizeof(iodata_t));
        if (!io_mem) return 0;
        // Копируем данные
        uint8_t* buf = (uint8_t*)SDL_malloc(size);
        if (!buf) { SDL_free(io_mem); return 0; }
        memcpy(buf, data, size);
        io_mem->q.addr = offset;
        io_mem->q.size = size;
        io_mem->value = buf; // признак записи + данные
        std::lock_guard<std::mutex> guard(g_queueMutex);
        if (!ctx->StartComunication || lfqueue_enq(&ctx->tx_Queue, io_mem) != 0) {
            SDL_free(buf);
            SDL_free(io_mem);
            return 0;
        }
        return size; // возвращаем количество запрошенных байт
    }

    // ------------------------------------------------------------------
    // Чтение (читают из IOMAP_Data и ставят запрос в rx_Queue)
    // Принимают физический адрес, преобразуют в смещение
    // ------------------------------------------------------------------

    bool DataReadBolean(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return false;
        CHECK_PHYSICAL_ADDR(phys_addr, 1);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        bool b = (ctx->IOMAP_Data[offset] & 0x01) != 0;
        SDL_UnlockMutex(ctx->Datalock);
        DataReadArray(ctx, phys_addr, 1); // асинхронное обновление
        return b;
    }

    uint8_t DataReadU08(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 1);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        uint8_t val = ctx->IOMAP_Data[offset];
        SDL_UnlockMutex(ctx->Datalock);
        DataReadArray(ctx, phys_addr, 1);
        return val;
    }

    uint16_t DataReadU16(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 2);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        uint16_t val = (uint16_t)ctx->IOMAP_Data[offset] |
            ((uint16_t)ctx->IOMAP_Data[offset + 1] << 8);
        SDL_UnlockMutex(ctx->Datalock);
        DataReadArray(ctx, phys_addr, 2);
        return val;
    }

    uint32_t DataReadU32(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 4);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        uint32_t val = (uint32_t)ctx->IOMAP_Data[offset] |
            ((uint32_t)ctx->IOMAP_Data[offset + 1] << 8) |
            ((uint32_t)ctx->IOMAP_Data[offset + 2] << 16) |
            ((uint32_t)ctx->IOMAP_Data[offset + 3] << 24);
        SDL_UnlockMutex(ctx->Datalock);
        DataReadArray(ctx, phys_addr, 4);
        return val;
    }

    float DataReadFT32(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return 0.0f;
        CHECK_PHYSICAL_ADDR(phys_addr, 4);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        FT32 f;
        f.hex = (uint32_t)ctx->IOMAP_Data[offset] |
            ((uint32_t)ctx->IOMAP_Data[offset + 1] << 8) |
            ((uint32_t)ctx->IOMAP_Data[offset + 2] << 16) |
            ((uint32_t)ctx->IOMAP_Data[offset + 3] << 24);
        SDL_UnlockMutex(ctx->Datalock);
        DataReadArray(ctx, phys_addr, 4);
        float value;
        memcpy(&value, &f.hex, sizeof(value));
        return value;
    }

    uint64_t DataReadU64(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, 8);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        uint32_t low = (uint32_t)ctx->IOMAP_Data[offset] |
            ((uint32_t)ctx->IOMAP_Data[offset + 1] << 8) |
            ((uint32_t)ctx->IOMAP_Data[offset + 2] << 16) |
            ((uint32_t)ctx->IOMAP_Data[offset + 3] << 24);
        uint32_t high = (uint32_t)ctx->IOMAP_Data[offset + 4] |
            ((uint32_t)ctx->IOMAP_Data[offset + 5] << 8) |
            ((uint32_t)ctx->IOMAP_Data[offset + 6] << 16) |
            ((uint32_t)ctx->IOMAP_Data[offset + 7] << 24);
        SDL_UnlockMutex(ctx->Datalock);
        uint64_t v = ((uint64_t)high << 32) | low;
        DataReadArray(ctx, phys_addr, 8);
        return v;
    }

    // Чтение массива данных (заполняет IOMAP_Data)
    uint16_t DataReadArray(MainContext* ctx, uint32_t phys_addr, uint16_t size)
    {
        if (!ctx || !ctx->StartComunication || size == 0) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, size);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        iodata_t* io_mem = (iodata_t*)SDL_malloc(sizeof(iodata_t));
        if (!io_mem) return 0;
        io_mem->q.addr = offset;
        io_mem->q.size = size;
        io_mem->value = nullptr; // признак чтения
        std::lock_guard<std::mutex> guard(g_queueMutex);
        if (!ctx->StartComunication || lfqueue_enq(&ctx->rx_Queue, io_mem) != 0) {
            SDL_free(io_mem);
            return 0;
        }
        return size;
    }

    // ------------------------------------------------------------------
    // Универсальные функции (используются в виджетах)
    // Они принимают физический адрес через указатель, передают дальше
    // ------------------------------------------------------------------

    bool DataReadWriteBoolean(MainContext* ctx, XMLElement* e, uint8_t* val, uint32_t* phys_addr)
    {
        if (!ctx || !ctx->StartComunication) return false;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return false;
        if (val != nullptr)
            return DataWriteBolean(ctx, val, reg);
        else
            return DataReadBolean(ctx, reg);
    }

    uint8_t DataReadWriteU08(MainContext* ctx, XMLElement* e, uint8_t* val, uint32_t* phys_addr)
    {
        if (!ctx || !ctx->StartComunication) return 0;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return 0;
        if (val != nullptr)
            return DataWriteU08(ctx, val, reg);
        else
            return DataReadU08(ctx, reg);
    }

    uint16_t DataReadWriteU16(MainContext* ctx, XMLElement* e, uint16_t* val, uint32_t* phys_addr)
    {
        if (!ctx || !ctx->StartComunication) return 0;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return 0;
        if (val != nullptr)
            return DataWriteU16(ctx, val, reg);
        else
            return DataReadU16(ctx, reg);
    }

    uint32_t DataReadWriteU32(MainContext* ctx, XMLElement* e, uint32_t* val, uint32_t* phys_addr)
    {
        if (!ctx || !ctx->StartComunication) return 0;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return 0;
        if (val != nullptr)
            return DataWriteU32(ctx, val, reg);
        else
            return DataReadU32(ctx, reg);
    }

    float DataReadWriteFT32(MainContext* ctx, XMLElement* e, float* val, uint32_t* phys_addr)
    {
        if (!ctx || !ctx->StartComunication) return 0.0f;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return 0.0f;
        if (val != nullptr)
            return DataWriteFT32(ctx, val, reg);
        else
            return DataReadFT32(ctx, reg);
    }

    uint64_t DataReadWriteU64(MainContext* ctx, XMLElement* e, uint64_t* val, uint32_t* phys_addr)
    {
        if (!ctx || !ctx->StartComunication) return 0;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return 0;
        if (val != nullptr)
            return DataWriteU64(ctx, val, reg);
        else
            return DataReadU64(ctx, reg);
    }

    // Универсальная функция для чтения/записи массива (используется в виджетах)
    // data != nullptr – запись, иначе чтение
    uint16_t DataReadWriteArray(MainContext* ctx, XMLElement* e, uint8_t* data, uint32_t* phys_addr, uint16_t size)
    {
        if (!ctx || !ctx->StartComunication) return 0;
        uint32_t reg = 0;
        if (phys_addr != nullptr)
            reg = *phys_addr;
        else if (TryFindElement(e, "Address"))
            reg = AddressFromHex(e->FirstChildElement("Address"));
        else
            return 0;

        if (data != nullptr)
            return DataWriteArray(ctx, reg, data, size);
        else
            return DataReadArray(ctx, reg, size);
    }

    // ------------------------------------------------------------------
    // Вспомогательная функция для установки скорости J‑Link
    // ------------------------------------------------------------------
    void GetSetMaxSpeed(void) {
        int v1, v2, v3;
        v1 = JLINKARM_GetSpeed();
        JLINKARM_SetMaxSpeed();
        v2 = JLINKARM_GetSpeed();
        JLINKARM_SetSpeed(8000);
        v3 = JLINKARM_GetSpeed();
        printf("Start speed: %i | Max. speed: %i | Chosen speed: %i\n", v1, v2, v3);
    }

    // ------------------------------------------------------------------
    // Поток обработки очередей (только J‑Link)
    // В очереди лежат смещения (offset). Для J‑Link используем физический адрес = база + offset
    // ------------------------------------------------------------------
    static SDL_Thread* g_commThread = nullptr;
    static std::atomic<bool> g_commRunning{false};

    // Queue nodes do not own their payloads. Always release both allocations.
    static void DrainQueue(lfqueue_t* queue)
    {
        while (void* item = lfqueue_deq(queue)) {
            iodata_t* io = static_cast<iodata_t*>(item);
            SDL_free(io->value);
            SDL_free(io);
        }
    }

    static void* Dequeue(lfqueue_t* queue)
    {
        std::lock_guard<std::mutex> guard(g_queueMutex);
        return lfqueue_deq(queue);
    }

    static int CommunicationThreadFunc(void* data)
    {
        MainContext* ctx = (MainContext*)data;

        // --- Устанавливаем высокий приоритет для потока ---
        if (SDL_SetThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL) != 0) {
            SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
        }

        while (g_commRunning)
        {
            if (!ctx->StartComunication || !ctx->jlink_opened) {
                SDL_Delay(50);
                continue;
            }
            // Обработка очереди записи
            void* item = Dequeue(&ctx->tx_Queue);
            if (item)
            {
                iodata_t* io = (iodata_t*)item;
                if (ctx->jlink_opened) {
                    uint32_t real_addr = ctx->mcu_memory_base + io->q.addr;
                    if (io->value != nullptr) {
                        int err = JLINKARM_WriteMem(real_addr, io->q.size, io->value);
                        if (err != io->q.size) {
                            ctx->StartComunication = false;
                        }
                        SDL_free(io->value);
                    }
                }
                else if (io->value) {
                    SDL_free(io->value);
                }
                SDL_free(item);
            }

            // Обработка очереди чтения
            item = Dequeue(&ctx->rx_Queue);
            if (item)
            {
                iodata_t* io = (iodata_t*)item;
                if (ctx->StartComunication && ctx->jlink_opened && io->value == nullptr) {
                    uint32_t real_addr = ctx->mcu_memory_base + io->q.addr;
                    uint8_t* tmp = (uint8_t*)SDL_malloc(io->q.size);
                    if (tmp) {
                        int err = JLINKARM_ReadMem(real_addr, io->q.size, tmp);
                        if (err != io->q.size) {
                            ctx->StartComunication = false;
                        }
                        else {
                            SDL_LockMutex(ctx->Datalock);
                            memcpy(ctx->IOMAP_Data + io->q.addr, tmp, io->q.size);
                            SDL_UnlockMutex(ctx->Datalock);
                        }
                        SDL_free(tmp);
                    }
                }
                else if (io->value) {
                    SDL_free(io->value);
                }
                SDL_free(item);
            }

            SDL_Delay(1);
        }
        return 0;
    }

    void StartCommunication(MainContext* ctx)
    {
        if (!ctx || !ctx->Datalock || !ctx->IOMAP_Data) return;
        if (ctx->StartComunication && g_commThread) return;
        // Reconnect after an I/O failure must not reuse the failed session.
        StopCommunication(ctx);

        if (!ctx->jlink_opened) {
            const char* err = JLINKARM_Open();
            if (err) {
                printf("J-Link open failed: %s\n", err);
                ctx->jlink_opened = false;
                ctx->StartComunication = false;
                return;
            }
            char acOut[1024];
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "device = %s", ctx->jlink_device);
            if (JLINKARM_ExecCommand(cmd, acOut, sizeof(acOut)) != 0 ||
                JLINKARM_TIF_Select(JLINKARM_TIF_SWD) < 0) {
                JLINKARM_Close();
                return;
            }
            GetSetMaxSpeed();
            if (JLINKARM_Connect() < 0) {
                JLINKARM_Close();
                return;
            }
            ctx->jlink_opened = true;
            ctx->StartComunication = true;
        }
        else {
            ctx->StartComunication = true;
        }

        if (!g_commThread) {
            g_commRunning = true;
            g_commThread = SDL_CreateThread(CommunicationThreadFunc, "CommThread", ctx);
            if (!g_commThread) {
                g_commRunning = false;
                ctx->StartComunication = false;
                JLINKARM_Close();
                ctx->jlink_opened = false;
            }
        }
    }

    void StopCommunication(MainContext* ctx)
    {
        if (ctx) ctx->StartComunication = false;
        g_commRunning = false;
        if (g_commThread) {
            SDL_WaitThread(g_commThread, NULL);
            g_commThread = nullptr;
        }

        if (ctx && ctx->jlink_opened) {
            JLINKARM_Close();
            ctx->jlink_opened = false;
        }

        if (ctx) {
            std::lock_guard<std::mutex> guard(g_queueMutex);
            DrainQueue(&ctx->rx_Queue);
            DrainQueue(&ctx->tx_Queue);
            if (ctx->IOMAP_Data && ctx->Datalock) {
                SDL_LockMutex(ctx->Datalock);
                memset(ctx->IOMAP_Data, 0, IOMAP_SIZE);
                SDL_UnlockMutex(ctx->Datalock);
            }
        }
    }

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------------------
// Среднее арифметическое
// ------------------------------------------------------------------
float average(const std::vector<float>& v) {
    if (v.empty()) return 0.0f;
    float sum = 0.0f;
    for (float x : v) {
        sum += x;
    }
    return sum / v.size();
}

// ------------------------------------------------------------------
// Сглаживание скользящим окном (окно приводится к нечётному)
// ------------------------------------------------------------------
std::vector<float> smooth(const std::vector<float>& input, int window) {
    if (input.empty() || window <= 0) return {};

    const std::size_t hw = static_cast<std::size_t>(window / 2);

    std::vector<float> output(input.size());

    for (std::size_t i = 0; i < input.size(); ++i) {
        const std::size_t start = i > hw ? i - hw : 0;
        const std::size_t end = i + std::min(hw, input.size() - 1 - i);
        const std::size_t count = end - start + 1;
        float sum = 0.0f;

        for (std::size_t j = start; j <= end; ++j) {
            sum += input[j];
        }

        output[i] = sum / static_cast<float>(count);
    }

    return output;
}