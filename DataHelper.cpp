// DataHelper.cpp
// Converts physical addresses into offsets in IOMAP_Data
// All reads and writes go through DataWriteArray/DataReadArray

#include <Main.h>
#include "Editor/XMLSafety.h"
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
#include <chrono>
#include <deque>
#include <condition_variable>
#include "JLinkConnection.h"

using namespace tinyxml2;

// Checks that the physical address lies within the mapped MCU memory
#define CHECK_PHYSICAL_ADDR(addr, size) \
    do { \
        if ((addr) == 0 || (addr) < ctx->mcu_memory_base) return 0; \
        if ((uint64_t)(addr) + (uint64_t)(size) > (uint64_t)ctx->mcu_memory_base + (uint64_t)IOMAP_SIZE || \
            (uint64_t)(addr) + (uint64_t)(size) > UINT64_C(0x100000000)) return 0; \
    } while (0)

// Finds a child element by name
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

// Converts a string like "#01A4" (hex) or "0x..."/decimal into an integer; 0 if invalid
uint32_t AddressFromHex(XMLElement* e)
{
    uint32_t value = 0;
    if (!e || !ui_safety::ParseAddressText(e->GetText(), &value)) return 0;
    return value;
}

// Protect the bundled queue's node reclamation from concurrent access.
static std::mutex g_queueMutex;

// ------------------------------------------------------------------
// Read subscriptions
// Widgets call the read functions every frame. Instead of one J-Link request
// per call, each call marks the bytes it needs (one bit per IOMAP byte). The
// communication thread merges marked bytes into blocks and polls them in a loop,
// so the J-Link load no longer grows with the number of widgets or frames.
// ------------------------------------------------------------------
static constexpr uint32_t kBitmapWords = IOMAP_SIZE / 64;
// Ranges no widget asked for during this period stop being polled.
static constexpr auto kSubscriptionLifetime = std::chrono::milliseconds(500);
// Unrequested bytes between two ranges that may be read together with them.
static constexpr uint32_t kMaxMergeGap = 64;
// Largest single J-Link read; pending writes are sent between blocks.
static constexpr uint32_t kMaxBlockSize = 4096;
// Shortest polling cycle, to leave the debug port to the target between cycles.
static constexpr auto kMinCyclePeriod = std::chrono::milliseconds(1);

struct ReadBlock { uint32_t offset; uint32_t size; };

static std::mutex g_subscriptionMutex;
static std::vector<uint64_t> g_requested;  // bytes asked for since the last expiry
static std::vector<uint64_t> g_polled;     // bytes the communication thread reads
static bool g_polledChanged = false;
// Incremented on every write-through to the cache (under Datalock).
static std::atomic<uint32_t> g_writeSequence{0};
// Reject a producer that allocated a request in an earlier connection session.
static std::atomic<uint64_t> g_sessionSequence{0};

// Sets the bits [first, first + count); returns true if any of them was clear.
static bool MarkBits(std::vector<uint64_t>& bits, uint32_t first, uint32_t count)
{
    bool added = false;
    for (uint32_t pos = first, end = first + count; pos < end; ) {
        const uint32_t bit = pos % 64;
        const uint32_t n = std::min<uint32_t>(64 - bit, end - pos);
        const uint64_t mask = (n == 64 ? ~UINT64_C(0) : ((UINT64_C(1) << n) - 1)) << bit;
        uint64_t& word = bits[pos / 64];
        added |= (word & mask) != mask;
        word |= mask;
        pos += n;
    }
    return added;
}

static uint32_t LowestSetBit(uint64_t v)
{
    uint32_t n = 0;
    while (!(v & 1)) { v >>= 1; ++n; }
    return n;
}

// Index of the first bit at or after pos with the given value, or IOMAP_SIZE.
static uint32_t FindBit(const std::vector<uint64_t>& bits, uint32_t pos, bool set)
{
    uint32_t index = pos / 64;
    if (index >= kBitmapWords) return IOMAP_SIZE;
    uint64_t word = (set ? bits[index] : ~bits[index]) & (~UINT64_C(0) << (pos % 64));
    while (!word) {
        if (++index == kBitmapWords) return IOMAP_SIZE;
        word = set ? bits[index] : ~bits[index];
    }
    return index * 64 + LowestSetBit(word);
}

// Cortex-M code/SRAM and external RAM regions, where reading the bytes between
// two variables has no side effects. Peripheral and system regions are excluded:
// reading an unrequested register there can clear flags or pop FIFOs.
static bool IsPlainMemory(uint32_t address)
{
    return address < 0x40000000u || (address >= 0x60000000u && address < 0xA0000000u);
}

static void BuildReadBlocks(const std::vector<uint64_t>& bits, uint32_t base, std::vector<ReadBlock>& blocks)
{
    blocks.clear();
    for (uint32_t start = FindBit(bits, 0, true); start < IOMAP_SIZE; ) {
        const uint32_t end = FindBit(bits, start, false);
        if (!blocks.empty()) {
            ReadBlock& last = blocks.back();
            const uint32_t lastEnd = last.offset + last.size;
            if (start - lastEnd <= kMaxMergeGap && end - last.offset <= kMaxBlockSize &&
                IsPlainMemory(base + lastEnd) && IsPlainMemory(base + start - 1)) {
                last.size = end - last.offset;
                start = FindBit(bits, end, true);
                continue;
            }
        }
        for (uint32_t offset = start; offset < end; offset += kMaxBlockSize)
            blocks.push_back({ offset, std::min(kMaxBlockSize, end - offset) });
        start = FindBit(bits, end, true);
    }
}

// Called by the readers. Returns false when the exchange is stopped.
static bool Subscribe(MainContext* ctx, uint32_t offset, uint16_t size)
{
    std::lock_guard<std::mutex> guard(g_subscriptionMutex);
    // Checked under the lock so a subscription cannot survive StopCommunication().
    if (!ctx->StartComunication) return false;
    if (g_requested.empty()) {
        g_requested.assign(kBitmapWords, 0);
        g_polled.assign(kBitmapWords, 0);
    }
    MarkBits(g_requested, offset, size);
    // New ranges are polled at once instead of waiting for the next expiry.
    if (MarkBits(g_polled, offset, size)) g_polledChanged = true;
    return true;
}

// Rebuilds the block list after new subscriptions; on expiry the polled set is
// replaced by what was requested during the last period.
static void UpdateReadBlocks(uint32_t base, bool expired, std::vector<ReadBlock>& blocks)
{
    std::lock_guard<std::mutex> guard(g_subscriptionMutex);
    if (g_requested.empty()) {
        blocks.clear();
        return;
    }
    if (expired) {
        g_polled = g_requested;
        std::fill(g_requested.begin(), g_requested.end(), 0);
        g_polledChanged = true;
    }
    if (g_polledChanged) {
        BuildReadBlocks(g_polled, base, blocks);
        g_polledChanged = false;
    }
}

// ------------------------------------------------------------------
// Sample streams for the plots
// The communication thread records every read of a streamed range with a
// timestamp, so plots get one point per polling cycle instead of one per frame.
// ------------------------------------------------------------------
struct SampleStream {
    const void* owner;
    uint32_t offset;
    uint16_t size;
    bool requested;                  // asked for since the last expiry
    std::deque<DataSample> samples;  // not yet taken by the owner
};

// About a minute at 1 kHz; older samples are dropped if the owner stops reading.
static constexpr size_t kMaxSamplesPerStream = 65536;
static const auto g_clockStart = std::chrono::steady_clock::now();
static std::mutex g_sampleMutex;
static std::vector<SampleStream> g_streams;
// Polling cycles in which every block was read and published to the cache.
static std::atomic<uint32_t> g_completedCycles{0};

static double SecondsSinceStart(std::chrono::steady_clock::time_point t)
{
    return std::chrono::duration<double>(t - g_clockStart).count();
}

static void RecordSamples(uint32_t blockOffset, uint32_t blockSize, const uint8_t* data, double time)
{
    std::lock_guard<std::mutex> guard(g_sampleMutex);
    for (SampleStream& stream : g_streams) {
        if (stream.offset < blockOffset || stream.offset + stream.size > blockOffset + blockSize)
            continue;
        DataSample sample = {};
        sample.time = time;
        memcpy(sample.bytes, data + (stream.offset - blockOffset), stream.size);
        if (stream.samples.size() >= kMaxSamplesPerStream)
            stream.samples.pop_front();
        stream.samples.push_back(sample);
    }
}

// Drops the streams nobody read during the last period, like the subscriptions.
static void ExpireSampleStreams()
{
    std::lock_guard<std::mutex> guard(g_sampleMutex);
    g_streams.erase(std::remove_if(g_streams.begin(), g_streams.end(),
        [](const SampleStream& s) { return !s.requested; }), g_streams.end());
    for (SampleStream& stream : g_streams) stream.requested = false;
}

static void ClearSubscriptions()
{
    {
        std::lock_guard<std::mutex> guard(g_subscriptionMutex);
        std::fill(g_requested.begin(), g_requested.end(), 0);
        std::fill(g_polled.begin(), g_polled.end(), 0);
        g_polledChanged = true;
    }
    std::lock_guard<std::mutex> guard(g_sampleMutex);
    g_streams.clear();
}

// The worker owns the session. The mutex also excludes device enumeration in
// Settings; that UI path uses try_lock and never waits for a USB transaction.
static std::mutex g_sdkMutex;
static std::mutex g_statusMutex;
static JLinkConnectionStatus g_status;
static std::atomic<bool> g_commRunning{false};
static std::condition_variable g_wake;
static std::mutex g_waitMutex;
static std::chrono::steady_clock::time_point g_retryAt;

JLinkConnectionStatus GetJLinkConnectionStatus()
{
    std::lock_guard<std::mutex> lock(g_statusMutex);
    auto status = g_status;
    if (status.state == JLinkConnectionState::Reconnecting) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            g_retryAt - std::chrono::steady_clock::now()).count();
        status.retryInMs = remaining > 0 ? static_cast<unsigned>(remaining) : 0;
    }
    return status;
}

bool IsCommunicationRequested() { return g_commRunning; }

bool TryGetJLinkDeviceNames(std::vector<std::string>& names)
{
    std::unique_lock<std::mutex> lock(g_sdkMutex, std::try_to_lock);
    if (!lock.owns_lock()) return false;
    names.clear();
    const int count = JLINKARM_DEVICE_GetInfo(-1, nullptr);
    for (int i = 0; i < count; ++i) {
        JLINKARM_DEVICE_INFO info{};
        info.SizeOfStruct = sizeof(info);
        if (JLINKARM_DEVICE_GetInfo(i, &info) == 0 && info.sName)
            names.emplace_back(info.sName);
    }
    return true;
}

static void SetConnectionStatus(JLinkConnectionState state, unsigned attempt = 0,
    const std::string& error = {}, unsigned retryMs = 0)
{
    std::lock_guard<std::mutex> lock(g_statusMutex);
    g_status.state = state;
    g_status.attempt = attempt;
    g_status.retryInMs = retryMs;
    g_retryAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(retryMs);
    // Keep the last failure visible after recovery, especially ambiguous writes.
    if (!error.empty()) g_status.lastError = error;
}

// Unlike SDL_Delay, retry waits are interrupted by Disconnect/exit immediately.
static bool WaitForCommunication(unsigned ms)
{
    std::unique_lock<std::mutex> lock(g_waitMutex);
    g_wake.wait_for(lock, std::chrono::milliseconds(ms), [] { return !g_commRunning.load(); });
    return g_commRunning;
}

#ifdef __cplusplus
extern "C" {
#endif

    uint16_t DataWriteArray(MainContext* ctx, uint32_t phys_addr, const uint8_t* data, uint16_t size);
    uint16_t DataReadArray(MainContext* ctx, uint32_t phys_addr, uint16_t size);

    // ------------------------------------------------------------------
    // Writing (requests go to tx_Queue)
    // Take a physical address and convert it into an offset
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

    // Writing: queued for the communication thread and copied into the cache at
    // once, so the widget shows the new value instead of jumping back until the
    // next read completes.
    uint16_t DataWriteArray(MainContext* ctx, uint32_t phys_addr, const uint8_t* data, uint16_t size)
    {
        const uint64_t session = g_sessionSequence;
        if (!ctx || !ctx->StartComunication || !data || size == 0) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, size);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        iodata_t* io_mem = (iodata_t*)SDL_malloc(sizeof(iodata_t));
        if (!io_mem) return 0;
        uint8_t* buf = (uint8_t*)SDL_malloc(size);
        if (!buf) { SDL_free(io_mem); return 0; }
        memcpy(buf, data, size);
        io_mem->q.addr = offset;
        io_mem->q.size = size;
        io_mem->value = buf;
        {
            std::lock_guard<std::mutex> guard(g_queueMutex);
            if (!ctx->StartComunication || g_sessionSequence != session ||
                lfqueue_enq(&ctx->tx_Queue, io_mem) != 0) {
                SDL_free(buf);
                SDL_free(io_mem);
                return 0;
            }
            SDL_LockMutex(ctx->Datalock);
            memcpy(ctx->IOMAP_Data + offset, data, size);
            ++g_writeSequence;
            SDL_UnlockMutex(ctx->Datalock);
        }
        return size;
    }

    // ------------------------------------------------------------------
    // Reading (from IOMAP_Data; the range stays subscribed for polling)
    // Take a physical address and convert it into an offset
    // ------------------------------------------------------------------

    bool DataReadBolean(MainContext* ctx, uint32_t phys_addr)
    {
        if (!ctx || !ctx->IOMAP_Data) return false;
        CHECK_PHYSICAL_ADDR(phys_addr, 1);
        uint32_t offset = phys_addr - ctx->mcu_memory_base;
        SDL_LockMutex(ctx->Datalock);
        bool b = (ctx->IOMAP_Data[offset] & 0x01) != 0;
        SDL_UnlockMutex(ctx->Datalock);
        DataReadArray(ctx, phys_addr, 1); // asynchronous refresh
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

    // Reading: the value comes from the cache filled by the communication thread.
    // The call only (re)subscribes the range, so it keeps being polled while the
    // widget is drawn; ranges nobody asks for expire after kSubscriptionLifetime.
    uint16_t DataReadArray(MainContext* ctx, uint32_t phys_addr, uint16_t size)
    {
        if (!ctx || size == 0) return 0;
        CHECK_PHYSICAL_ADDR(phys_addr, size);
        return Subscribe(ctx, phys_addr - ctx->mcu_memory_base, size) ? size : 0;
    }

    // ------------------------------------------------------------------
    // Generic functions used by the widgets
    // They take the physical address through a pointer and pass it on
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

    // Generic array read/write used by the widgets
    // data != nullptr - write, otherwise read
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
    // Helper that sets the J-Link interface speed
    // ------------------------------------------------------------------
    void GetSetMaxSpeed(void) {
        std::lock_guard<std::mutex> lock(g_sdkMutex);
        int v1, v2, v3;
        v1 = JLINKARM_GetSpeed();
        JLINKARM_SetMaxSpeed();
        v2 = JLINKARM_GetSpeed();
        JLINKARM_SetSpeed(8000);
        v3 = JLINKARM_GetSpeed();
        printf("Start speed: %i | Max. speed: %i | Chosen speed: %i\n", v1, v2, v3);
    }

    // ------------------------------------------------------------------
    // Communication thread (J-Link only)
    // Queued writes and read blocks hold offsets; J-Link gets base + offset.
    // ------------------------------------------------------------------
    static SDL_Thread* g_commThread = nullptr;

    // Queue nodes do not own their payloads. Always release both allocations.
    static void DrainQueue(lfqueue_t* queue)
    {
        while (void* item = lfqueue_deq(queue)) {
            iodata_t* io = static_cast<iodata_t*>(item);
            SDL_free(io->value);
            SDL_free(io);
        }
    }

    // Executes queued writes in order. Of consecutive writes to the same range
    // (e.g. a dragged slider) only the last value is sent. Returns false on a
    // failed or short write.
    static bool FlushWrites(MainContext* ctx, std::vector<iodata_t*>& batch, std::string& error)
    {
        batch.clear();
        {
            std::lock_guard<std::mutex> guard(g_queueMutex);
            while (void* item = lfqueue_deq(&ctx->tx_Queue))
                batch.push_back(static_cast<iodata_t*>(item));
        }
        bool ok = true;
        for (size_t i = 0; i < batch.size(); ++i) {
            iodata_t* io = batch[i];
            const bool superseded = i + 1 < batch.size() &&
                batch[i + 1]->q.addr == io->q.addr && batch[i + 1]->q.size == io->q.size;
            if (ok && !superseded && g_commRunning) {
                std::lock_guard<std::mutex> lock(g_sdkMutex);
                const int written = JLINKARM_WriteMemEx(ctx->mcu_memory_base + io->q.addr, io->q.size, io->value, 0);
                ok = (written == io->q.size);
                if (!ok) {
                    std::lock_guard<std::mutex> statusLock(g_statusMutex);
                    g_status.writeMayHaveCompleted = true;
                }
                if (!ok) error = "Write failed at " + std::to_string(ctx->mcu_memory_base + io->q.addr) +
                    " (" + std::to_string(written) + "/" + std::to_string(io->q.size) +
                    " bytes). Target may have been modified; pending writes discarded, not replayed.";
            }
            SDL_free(io->value);
            SDL_free(io);
        }
        return ok;
    }

    // Block producers before draining: no write may cross a session boundary.
    static void InvalidateSessionData(MainContext* ctx)
    {
        {
            std::lock_guard<std::mutex> lock(g_queueMutex);
            ctx->StartComunication = false;
            ++g_sessionSequence;
            DrainQueue(&ctx->tx_Queue);
            SDL_LockMutex(ctx->Datalock);
            memset(ctx->IOMAP_Data, 0, IOMAP_SIZE);
            ++g_writeSequence;
            SDL_UnlockMutex(ctx->Datalock);
        }
        ClearSubscriptions();
    }

    static void CloseSession(MainContext* ctx)
    {
        if (ctx->jlink_opened.exchange(false)) {
            std::lock_guard<std::mutex> lock(g_sdkMutex);
            JLINKARM_Close();
        }
    }

    // No reset/halt is performed. A session always reapplies the selected target,
    // interface and speed; reconnect never reuses a possibly broken DLL session.
    static bool OpenSession(MainContext* ctx, int& serial, std::string& error)
    {
        std::lock_guard<std::mutex> lock(g_sdkMutex);
        if (serial > 0 && JLINKARM_EMU_SelectByUSBSN(static_cast<uint32_t>(serial)) < 0) {
            error = "Cannot select the original J-Link (serial " + std::to_string(serial) + ").";
            return false;
        }
        const char* openError = JLINKARM_Open();
        if (openError) { error = std::string("J-Link open failed: ") + openError; return false; }
        ctx->jlink_opened = true;
        // Pin even a failed target connection to this probe, not another USB probe.
        const int openedSerial = JLINKARM_GetSN();
        if (serial > 0 && openedSerial != serial) {
            error = "J-Link serial does not match the original probe; refusing to use a different device.";
            return false;
        }
        if (openedSerial > 0) serial = openedSerial;
        if (!g_commRunning) return false;
        char output[1024] = {};
        char command[128];
        snprintf(command, sizeof(command), "device = %s", ctx->jlink_device);
        const int configured = JLINKARM_ExecCommand(command, output, sizeof(output));
        output[sizeof(output) - 1] = 0;
        if (configured != 0) {
            error = "Target configuration failed (" + std::to_string(configured) + "): " + output;
            return false;
        }
        const int interfaceResult = JLINKARM_TIF_Select(JLINKARM_TIF_SWD);
        if (interfaceResult < 0) {
            error = "Cannot select SWD (" + std::to_string(interfaceResult) + ").";
            return false;
        }
        JLINKARM_SetSpeed(8000);
        if (!g_commRunning) return false;
        const int connected = JLINKARM_Connect();
        if (connected < 0) {
            error = "Target connection failed (" + std::to_string(connected) +
                "). Check target power, SWD wiring and selected device.";
            return false;
        }
        JLINKARM_HW_STATUS hw{};
        const int health = JLINKARM_GetHWStatus(&hw);
        if (health != 0 || !JLINKARM_IsConnected() || hw.VTarget == 0) {
            error = health == 0 && hw.VTarget == 0 ?
                "Target reference voltage is zero. Check target power and VTref wiring." :
                "Target did not pass the connection health check (" + std::to_string(health) + ").";
            return false;
        }
        return true;
    }

#ifndef JLINK_RETRY_BASE_MS
#define JLINK_RETRY_BASE_MS 500
#endif
    static constexpr unsigned kMaxConnectionAttempts = 5;
    static constexpr unsigned kHealthCheckMs = 250;

    static int CommunicationThreadFunc(void* data)
    {
        MainContext* ctx = static_cast<MainContext*>(data);
        SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
        std::vector<iodata_t*> writes;
        std::vector<ReadBlock> blocks;
        std::vector<uint8_t> buffer(kMaxBlockSize);
        auto expiry = std::chrono::steady_clock::now() + kSubscriptionLifetime;
        auto healthAt = std::chrono::steady_clock::now();
        unsigned attempt = 0;
        int serial = 0;
        bool recovering = false;
        std::string error;

        while (g_commRunning) {
            if (!ctx->StartComunication) {
                SetConnectionStatus(recovering ? JLinkConnectionState::Reconnecting :
                    JLinkConnectionState::Connecting, ++attempt);
                if (!OpenSession(ctx, serial, error)) {
                    CloseSession(ctx);
                    if (!g_commRunning) break;
                    if (attempt >= kMaxConnectionAttempts) {
                        SetConnectionStatus(JLinkConnectionState::Failed, attempt, error);
                        break;
                    }
                    const unsigned delay = std::min(4000u, static_cast<unsigned>(JLINK_RETRY_BASE_MS) << (attempt - 1));
                    SetConnectionStatus(JLinkConnectionState::Reconnecting, attempt, error, delay);
                    if (!WaitForCommunication(delay)) break;
                    recovering = true;
                    continue;
                }
                if (!g_commRunning) break;
                ctx->StartComunication = true;
                attempt = 0;
                SetConnectionStatus(JLinkConnectionState::Connected);
                healthAt = std::chrono::steady_clock::now();
                expiry = healthAt + kSubscriptionLifetime;
                blocks.clear();
            }
            const auto cycleStart = std::chrono::steady_clock::now();
            const bool expired = cycleStart >= expiry;
            if (expired) {
                expiry = cycleStart + kSubscriptionLifetime;
                ExpireSampleStreams();
            }
            UpdateReadBlocks(ctx->mcu_memory_base, expired, blocks);

            bool ok = true;
            bool published = true;
            if (cycleStart >= healthAt) {
                // Probe health even when there are no visible widgets/subscriptions.
                // This does not read arbitrary target memory/peripheral registers.
                std::lock_guard<std::mutex> lock(g_sdkMutex);
                JLINKARM_HW_STATUS hw{};
                const int result = JLINKARM_GetHWStatus(&hw);
                ok = result == 0 && JLINKARM_IsConnected() && hw.VTarget > 0;
                if (!ok) error = result == 0 && hw.VTarget == 0 ?
                    "Target reference voltage is zero. Check target power and VTref wiring." :
                    "J-Link/target connection lost (health check " + std::to_string(result) + ").";
                healthAt = cycleStart + std::chrono::milliseconds(kHealthCheckMs);
            }
            if (ok && blocks.empty()) ok = FlushWrites(ctx, writes, error);
            for (const ReadBlock& block : blocks) {
                if (!ok || !g_commRunning) break;
                const uint32_t sequence = g_writeSequence;
                ok = FlushWrites(ctx, writes, error);
                if (!ok || !g_commRunning) break;
                const auto readStart = std::chrono::steady_clock::now();
                int read;
                {
                    std::lock_guard<std::mutex> lock(g_sdkMutex);
                    read = JLINKARM_ReadMemEx(ctx->mcu_memory_base + block.offset, block.size, buffer.data(), 0);
                }
                if (read != static_cast<int>(block.size)) {
                    error = "Read failed at " + std::to_string(ctx->mcu_memory_base + block.offset) +
                        " (" + std::to_string(read) + "/" + std::to_string(block.size) +
                        " bytes). Cache invalidated; pending writes discarded.";
                    ok = false;
                    break;
                }
                const auto readEnd = std::chrono::steady_clock::now();
                RecordSamples(block.offset, block.size, buffer.data(),
                    SecondsSinceStart(readStart + (readEnd - readStart) / 2));
                SDL_LockMutex(ctx->Datalock);
                if (g_writeSequence == sequence)
                    memcpy(ctx->IOMAP_Data + block.offset, buffer.data(), block.size);
                else published = false;
                SDL_UnlockMutex(ctx->Datalock);
            }
            if (!ok) {
                InvalidateSessionData(ctx);
                blocks.clear();
                CloseSession(ctx);
                recovering = true;
                SetConnectionStatus(JLinkConnectionState::Reconnecting, 0, error, JLINK_RETRY_BASE_MS);
                if (!WaitForCommunication(JLINK_RETRY_BASE_MS)) break;
                continue;
            }
            if (published && !blocks.empty() && g_commRunning) ++g_completedCycles;
            if (std::chrono::steady_clock::now() - cycleStart < kMinCyclePeriod)
                WaitForCommunication(blocks.empty() ? 10 : 1);
        }
        InvalidateSessionData(ctx);
        CloseSession(ctx);
        g_commRunning = false;
        return 0;
    }

    // Lifecycle calls are UI-thread only. SDK Open/Connect/Close never run here.
    void StartCommunication(MainContext* ctx)
    {
        if (!ctx || !ctx->Datalock || !ctx->IOMAP_Data || g_commRunning) return;
        StopCommunication(ctx);
        {
            std::lock_guard<std::mutex> lock(g_statusMutex);
            g_status = {};
            g_status.state = JLinkConnectionState::Connecting;
        }
        g_commRunning = true;
        g_commThread = SDL_CreateThread(CommunicationThreadFunc, "CommThread", ctx);
        if (!g_commThread) {
            g_commRunning = false;
            SetConnectionStatus(JLinkConnectionState::Failed, 0, "Cannot create J-Link communication thread.");
        }
    }

    void StopCommunication(MainContext* ctx)
    {
        if (g_commRunning) SetConnectionStatus(JLinkConnectionState::Stopping);
        {
            std::lock_guard<std::mutex> lock(g_waitMutex);
            g_commRunning = false;
        }
        if (ctx) ctx->StartComunication = false;
        g_wake.notify_all();
        if (g_commThread) {
            // Never close the DLL concurrently with an in-flight SDK call.
            SDL_WaitThread(g_commThread, nullptr);
            g_commThread = nullptr;
        }
        if (ctx && ctx->IOMAP_Data && ctx->Datalock) InvalidateSessionData(ctx);
        SetConnectionStatus(JLinkConnectionState::Disconnected);
    }

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------------------
// Plot sampling API (see widget_common.h)
// ------------------------------------------------------------------
size_t DataReadSamples(MainContext* ctx, const void* owner, uint32_t phys_addr, uint16_t size, std::vector<DataSample>& out)
{
    out.clear();
    if (!ctx || !owner || size == 0 || size > sizeof(DataSample::bytes)) return 0;
    CHECK_PHYSICAL_ADDR(phys_addr, size);
    // Keep the range polled; this also fails while the exchange is stopped.
    if (!Subscribe(ctx, phys_addr - ctx->mcu_memory_base, size)) return 0;
    const uint32_t offset = phys_addr - ctx->mcu_memory_base;
    std::lock_guard<std::mutex> guard(g_sampleMutex);
    for (SampleStream& stream : g_streams) {
        if (stream.owner == owner && stream.offset == offset && stream.size == size) {
            stream.requested = true;
            out.assign(stream.samples.begin(), stream.samples.end());
            stream.samples.clear();
            return out.size();
        }
    }
    g_streams.push_back({ owner, offset, size, true, {} });
    return 0;
}

uint32_t DataCompletedCycles(void)
{
    return g_completedCycles;
}

double DataTimeNow(void)
{
    return SecondsSinceStart(std::chrono::steady_clock::now());
}

// ------------------------------------------------------------------
// Arithmetic mean
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
// Moving-window smoothing (the window is made odd)
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