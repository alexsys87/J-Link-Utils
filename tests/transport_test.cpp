#include <Main.h>
#include <cassert>
#include <climits>
#include <cerrno>
#include <iostream>
#include <unordered_map>

using namespace tinyxml2;
extern uint32_t AddressFromHex(XMLElement*);
extern std::vector<float> smooth(const std::vector<float>&, int);
extern "C" {
uint16_t DataReadArray(MainContext*, uint32_t, uint16_t);
uint16_t DataWriteArray(MainContext*, uint32_t, const uint8_t*, uint16_t);
uint64_t DataReadU64(MainContext*, uint32_t);
uint64_t DataWriteU64(MainContext*, uint64_t*, uint32_t);
float DataReadFT32(MainContext*, uint32_t);
float DataWriteFT32(MainContext*, float*, uint32_t);
}
size_t DataReadSamples(MainContext*, const void*, uint32_t, uint16_t, std::vector<DataSample>&);
uint32_t DataCompletedCycles();
double DataTimeNow();
static std::atomic<int> opens{0}, closes{0}, reads{0}, writes{0};
static std::atomic<int> openFail{0}, commandFail{0}, tifFail{0}, connectFail{0};
static std::atomic<int> failNextOpens{0};
static std::atomic<int> readResult{-2}, writeResult{-2};
static std::atomic<uint32_t> lastAddress{0};
static std::atomic<int> lastSize{0};
static std::atomic<bool> holdOpen{false};
static std::atomic<int> enteredOpens{0}, heartbeats{0}, connectedChecks{0};
static std::atomic<int> hwResult{0}, targetVoltage{3300}, connectedResult{1};
static std::atomic<int> probeSerial{12345678}, serialReads{0};
static std::atomic<int> selectResult{0}, selectedSerial{0}, selections{0};
static std::atomic<int> sdkActive{0}, sdkPeak{0};
// Every SDK entry point (except the legacy speed helper) must share one lock,
// including discovery from the UI while the worker is connecting or polling.
struct SDKCall {
    SDKCall() {
        const int active = ++sdkActive;
        int peak = sdkPeak.load();
        while (active > peak && !sdkPeak.compare_exchange_weak(peak, active)) {}
        assert(active == 1);
    }
    ~SDKCall() { --sdkActive; }
};
static uint8_t written[65536];
const char* JLINKARM_Open() {
    SDKCall call;
    ++opens; ++enteredOpens;
    while (holdOpen) SDL_Delay(1);
    int remaining = failNextOpens.load();
    while (remaining > 0) {
        if (failNextOpens.compare_exchange_weak(remaining, remaining - 1)) return "temporary mock failure";
    }
    return openFail ? "mock failure" : nullptr;
}
void JLINKARM_Close() { SDKCall call; ++closes; }
int JLINKARM_ExecCommand(const char*, char*, int) { SDKCall call; return commandFail; }
int JLINKARM_TIF_Select(int) { SDKCall call; return tifFail; }
int JLINKARM_Connect() { SDKCall call; return connectFail; }
int JLINKARM_GetSpeed() { SDKCall call; return 8000; }
void JLINKARM_SetMaxSpeed() {}
void JLINKARM_SetSpeed(int) { SDKCall call; }
int JLINKARM_GetHWStatus(JLINKARM_HW_STATUS* status) {
    SDKCall call;
    status->VTarget = static_cast<uint16_t>(targetVoltage.load());
    ++heartbeats;
    return hwResult;
}
char JLINKARM_IsConnected() { SDKCall call; ++connectedChecks; return static_cast<char>(connectedResult.load()); }
int JLINKARM_GetSN() { SDKCall call; ++serialReads; return probeSerial; }
int JLINKARM_EMU_SelectByUSBSN(uint32_t serial) {
    SDKCall call; selectedSerial = static_cast<int>(serial); ++selections;
    return selectResult;
}
int JLINKARM_DEVICE_GetInfo(int index, JLINKARM_DEVICE_INFO* info) {
    SDKCall call;
    if (!info || index < 0) return 2;
    if (index >= 2) return -1;
    if (info) info->sName = index == 0 ? "Cortex-M0" : "Cortex-M4";
    return 0;
}
// Simulated target memory: bytes never written read as 0xAB.
static std::mutex targetMutex;
static std::unordered_map<uint32_t, uint8_t> target;
struct ReadCall { uint32_t address, size; };
static std::vector<ReadCall> readCalls;
static std::atomic<bool> holdWrites{false};
static std::atomic<int> enteredWrites{0};
int JLINKARM_ReadMemEx(uint32_t address, uint32_t n, void* out, uint32_t) {
    SDKCall call;
    lastAddress = address; lastSize = static_cast<int>(n);
    {
        std::lock_guard<std::mutex> lock(targetMutex);
        for (uint32_t i = 0; i < n; ++i) {
            auto it = target.find(address + i);
            static_cast<uint8_t*>(out)[i] = it == target.end() ? 0xAB : it->second;
        }
        readCalls.push_back({address, n});
    }
    ++reads;
    return readResult == -2 ? static_cast<int>(n) : readResult.load();
}
int JLINKARM_WriteMemEx(uint32_t address, uint32_t n, const void* in, uint32_t) {
    SDKCall call;
    ++enteredWrites;
    while (holdWrites) SDL_Delay(1);
    lastAddress = address; lastSize = static_cast<int>(n);
    std::memcpy(written, in, n);
    {
        std::lock_guard<std::mutex> lock(targetMutex);
        for (uint32_t i = 0; i < n; ++i) target[address + i] = static_cast<const uint8_t*>(in)[i];
    }
    ++writes;
    return writeResult == -2 ? static_cast<int>(n) : writeResult.load();
}
static std::vector<ReadCall> takeReadCalls() {
    std::lock_guard<std::mutex> lock(targetMutex);
    std::vector<ReadCall> calls;
    calls.swap(readCalls);
    return calls;
}
template<class Predicate> void waitForImpl(Predicate predicate, int line) {
    for (int i = 0; i < 2000 && !predicate(); ++i) SDL_Delay(1);
    if (!predicate()) {
        const auto status = GetJLinkConnectionStatus();
        std::cerr << "Timed out at transport_test.cpp:" << line
                  << " state=" << static_cast<int>(status.state) << " attempt=" << status.attempt
                  << " opens=" << opens << " reads=" << reads << " writes=" << writes
                  << " error=" << status.lastError << '\n';
    }
    assert(predicate());
}
#define waitFor(...) waitForImpl((__VA_ARGS__), __LINE__)
static void startAndWait(MainContext& ctx) {
    StartCommunication(&ctx);
    waitFor([&] {
        return ctx.StartComunication.load() && GetJLinkConnectionStatus().state == JLinkConnectionState::Connected;
    });
    assert(ctx.jlink_opened);
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Connected);
    assert(IsCommunicationRequested());
}
static void waitForFailure(MainContext& ctx) {
    waitFor([] { return GetJLinkConnectionStatus().state == JLinkConnectionState::Failed; });
    assert(!ctx.StartComunication && !ctx.jlink_opened);
    assert(GetJLinkConnectionStatus().attempt == 5);
    assert(!GetJLinkConnectionStatus().lastError.empty());
}
static uint8_t cached(MainContext& ctx, unsigned offset) {
    std::lock_guard<std::mutex> lock(*ctx.Datalock);
    return ctx.IOMAP_Data[offset];
}
static void addressTests() {
    XMLDocument doc;
    auto e = doc.NewElement("Address"); doc.InsertEndChild(e);
    assert(AddressFromHex(nullptr) == 0);
    assert(AddressFromHex(e) == 0);
    e->SetText(" #20000010 "); assert(AddressFromHex(e) == 0x20000010);
    e->SetText("0xFFFFFFFF"); assert(AddressFromHex(e) == UINT32_MAX);
    e->SetText("536870912"); assert(AddressFromHex(e) == 0x20000000);
    for (const char* bad : {"-1", "0x100000000", "#", "123oops", "99999999999999999999999999"}) {
        e->SetText(bad); assert(AddressFromHex(e) == 0);
    }
    const auto result = smooth({1, 2, 3}, INT_MAX);
    assert(result == std::vector<float>({2, 2, 2}));
    assert(smooth({1, 2, 3}, 2) == std::vector<float>({1.5f, 2, 2.5f}));
}
struct AllocationState { int calls = 0, live = 0, failAt = 0; };
static void* queueAlloc(void* user, size_t n) {
    auto& state = *static_cast<AllocationState*>(user);
    if (++state.calls == state.failAt) { errno = 0; return nullptr; }
    void* p = std::malloc(n);
    if (p) ++state.live;
    return p;
}
static void queueFree(void* user, void* p) {
    if (p) --static_cast<AllocationState*>(user)->live;
    std::free(p);
}
static void queueAllocationTests() {
    for (int failedAllocation : {1, 2}) {
        lfqueue_t queue{};
        AllocationState state; state.failAt = failedAllocation;
        assert(lfqueue_init_mf(&queue, &state, queueAlloc, queueFree) != 0);
        assert(state.live == 0);
    }
    lfqueue_t queue{};
    AllocationState state;
    assert(lfqueue_init_mf(&queue, &state, queueAlloc, queueFree) == 0);
    state.failAt = 3;
    int payload = 42;
    assert(lfqueue_enq(&queue, &payload) != 0);
    assert(lfqueue_size(&queue) == 0);
    lfqueue_destroy(&queue); assert(state.live == 0);
}
static uint8_t cachedAt(MainContext& ctx, uint32_t address) { return cached(ctx, address - ctx.mcu_memory_base); }
// Reads are subscriptions polled in merged blocks; writes go through the cache.
static void pollingTests(MainContext& ctx) {
    const uint32_t base = ctx.mcu_memory_base;
    // 100 adjacent variables plus one 32 bytes further are read as one block,
    // a distant variable as a second one, and both keep being polled.
    startAndWait(ctx);
    for (int i = 0; i < 100; ++i) assert(DataReadArray(&ctx, base + 0x100 + 4 * i, 4) == 4);
    DataReadArray(&ctx, base + 0x100 + 400 + 32, 4);
    DataReadArray(&ctx, base + 0x2000, 4);
    takeReadCalls();
    SDL_Delay(30);
    StopCommunication(&ctx);
    auto calls = takeReadCalls();
    int merged = 0, distant = 0;
    for (const auto& c : calls) {
        if (c.address == base + 0x100 && c.size == 436) ++merged;
        else if (c.address == base + 0x2000 && c.size == 4) ++distant;
        else assert(!"unexpected read block");
    }
    assert(merged >= 3 && distant >= 3);
    // Peripheral space: unrequested registers between two variables are never read.
    ctx.mcu_memory_base = 0x40000000;
    startAndWait(ctx);
    DataReadArray(&ctx, 0x40000000, 4);
    DataReadArray(&ctx, 0x40000008, 4);
    SDL_Delay(20);
    StopCommunication(&ctx);
    calls = takeReadCalls();
    assert(!calls.empty());
    for (const auto& c : calls) assert(c.size == 4 && (c.address == 0x40000000 || c.address == 0x40000008));
    ctx.mcu_memory_base = base;
    // A range nobody asks for again stops being polled after its lifetime.
    startAndWait(ctx);
    DataReadArray(&ctx, base + 0x500, 4);
    SDL_Delay(1200);
    takeReadCalls();
    SDL_Delay(100);
    assert(takeReadCalls().empty());
    // A write shows up in the cache at once and is not rolled back by a block
    // read that started before the write reached the target.
    DataReadArray(&ctx, base + 0x600, 4);
    waitFor([&] { return cachedAt(ctx, base + 0x600) == 0xAB; });
    uint8_t value[] = {1, 2, 3, 4};
    assert(DataWriteArray(&ctx, base + 0x600, value, 4) == 4);
    for (int i = 0; i < 50; ++i) {
        DataReadArray(&ctx, base + 0x600, 4);
        assert(cachedAt(ctx, base + 0x600) == 1 && cachedAt(ctx, base + 0x603) == 4);
        SDL_Delay(1);
    }
    // Consecutive writes to one range while the target is busy send only the last value.
    holdWrites = true;
    const int entered = enteredWrites, before = writes;
    uint8_t v = 0;
    DataWriteArray(&ctx, base + 0x700, &v, 1);
    waitFor([&] { return enteredWrites == entered + 1; });
    for (v = 1; v <= 9; ++v) DataWriteArray(&ctx, base + 0x700, &v, 1);
    holdWrites = false;
    waitFor([&] { return writes == before + 2; });
    SDL_Delay(10);
    assert(writes == before + 2 && written[0] == 9);
    StopCommunication(&ctx);
    // Plot sample streams: every polling cycle adds a timestamped sample.
    {
        std::lock_guard<std::mutex> lock(targetMutex);
        target[base + 0x800] = 0x34; target[base + 0x801] = 0x12;
    }
    startAndWait(ctx);
    std::vector<DataSample> samples;
    int owner = 0;
    assert(DataReadSamples(&ctx, &owner, base + 0x800, 2, samples) == 0);
    SDL_Delay(30);
    DataReadSamples(&ctx, &owner, base + 0x800, 2, samples);
    assert(samples.size() >= 5);
    for (size_t i = 0; i < samples.size(); ++i) {
        assert(samples[i].bytes[0] == 0x34 && samples[i].bytes[1] == 0x12);
        if (i) assert(samples[i].time > samples[i - 1].time);
    }
    assert(samples.back().time <= DataTimeNow());
    const uint32_t cycles = DataCompletedCycles();
    SDL_Delay(10);
    assert(DataCompletedCycles() > cycles);
    assert(DataReadSamples(&ctx, &owner, base + 0x800, 9, samples) == 0);  // too wide for a sample
    StopCommunication(&ctx);
    assert(DataReadSamples(&ctx, &owner, base + 0x800, 2, samples) == 0 && samples.empty());
}
static void startupTests(MainContext& ctx) {
    // An Open that is stuck in USB I/O cannot block Start or status/discovery.
    holdOpen = true;
    const int entered = enteredOpens;
    const auto started = std::chrono::steady_clock::now();
    StartCommunication(&ctx);
    assert(std::chrono::steady_clock::now() - started < std::chrono::milliseconds(100));
    waitFor([&] { return enteredOpens == entered + 1; });
    assert(!ctx.StartComunication && IsCommunicationRequested());
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Connecting);
    std::vector<std::string> names;
    const auto queried = std::chrono::steady_clock::now();
    assert(!TryGetJLinkDeviceNames(names));
    assert(std::chrono::steady_clock::now() - queried < std::chrono::milliseconds(100));
    const int count = opens;
    StartCommunication(&ctx); // repeated Start must not launch another worker
    assert(opens == count);
    holdOpen = false;
    waitFor([&] { return ctx.StartComunication.load(); });
    StopCommunication(&ctx);
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Disconnected);
    assert(!IsCommunicationRequested());
    assert(TryGetJLinkDeviceNames(names));
    assert(names == std::vector<std::string>({"Cortex-M0", "Cortex-M4"}));

    // Exactly five attempts; only successfully opened sessions may be closed.
    int beforeOpen = opens, beforeClose = closes;
    openFail = 1; StartCommunication(&ctx); waitForFailure(ctx);
    assert(opens == beforeOpen + 5 && closes == beforeClose);
    StopCommunication(&ctx); openFail = 0;
    for (auto* failure : {&commandFail, &tifFail, &connectFail}) {
        beforeOpen = opens; beforeClose = closes;
        *failure = -1;
        StartCommunication(&ctx); waitForFailure(ctx);
        assert(opens == beforeOpen + 5 && closes == beforeClose + 5);
        StopCommunication(&ctx); *failure = 0;
    }
    beforeOpen = opens; beforeClose = closes;
    targetVoltage = 0;
    StartCommunication(&ctx); waitForFailure(ctx);
    assert(opens == beforeOpen + 5 && closes == beforeClose + 5);
    StopCommunication(&ctx); targetVoltage = 3300;
    beforeOpen = opens; beforeClose = closes;
    failThread = true; StartCommunication(&ctx);
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Failed);
    assert(!ctx.StartComunication && !ctx.jlink_opened);
    assert(opens == beforeOpen && closes == beforeClose);
    failThread = false; StopCommunication(&ctx);

    // Stop interrupts retry backoff and there are no late opens after it returns.
    openFail = 1; beforeOpen = opens;
    StartCommunication(&ctx);
    waitFor([&] { return opens >= beforeOpen + 2; });
    const auto stopping = std::chrono::steady_clock::now();
    StopCommunication(&ctx);
    assert(std::chrono::steady_clock::now() - stopping < std::chrono::milliseconds(100));
    const int stoppedOpens = opens;
    SDL_Delay(180);
    assert(opens == stoppedOpens && stoppedOpens < beforeOpen + 5);
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Disconnected);
    assert(!IsCommunicationRequested());
    openFail = 0;

    // SDK Open itself cannot be interrupted, but a concurrent Stop safely
    // joins it, closes the acquired session and never publishes it as connected.
    holdOpen = true;
    const int heldEntered = enteredOpens;
    beforeClose = closes;
    StartCommunication(&ctx);
    waitFor([&] { return enteredOpens == heldEntered + 1; });
    std::atomic<bool> stopped{false};
    std::thread stopper([&] { StopCommunication(&ctx); stopped = true; });
    waitFor([] { return GetJLinkConnectionStatus().state == JLinkConnectionState::Stopping; });
    assert(!stopped && !ctx.StartComunication);
    holdOpen = false;
    stopper.join();
    assert(stopped && closes == beforeClose + 1);
    assert(!ctx.StartComunication && !ctx.jlink_opened && !IsCommunicationRequested());
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Disconnected);
}
static void recoveryTests(MainContext& ctx) {
    const uint32_t base = ctx.mcu_memory_base;
    uint8_t value[] = {1, 2, 3, 4};
    startAndWait(ctx);
    assert(serialReads > 0);
    const int originalSerial = probeSerial;
    // Keep reconnect Open blocked so the disconnected cache and write gate can
    // be inspected deterministically, rather than racing a successful reconnect.
    DataReadArray(&ctx, base + 0x47, 4);
    waitFor([&] { return cached(ctx, 0x47) == 0xAB; });
    std::vector<DataSample> samples;
    int owner = 0;
    DataReadSamples(&ctx, &owner, base + 0x80, 2, samples);
    waitFor([&] {
        if (samples.empty()) DataReadSamples(&ctx, &owner, base + 0x80, 2, samples);
        return !samples.empty();
    });
    const int oldOpen = opens, oldSelections = selections;
    holdOpen = true; readResult = 1;
    waitFor([&] { return opens > oldOpen; });
    assert(!ctx.StartComunication);
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Reconnecting);
    assert(cached(ctx, 0x47) == 0);
    assert(lfqueue_size(&ctx.tx_Queue) == 0);
    assert(DataWriteArray(&ctx, base, value, 4) == 0);
    assert(DataReadArray(&ctx, base + 0x47, 4) == 0);
    assert(DataReadSamples(&ctx, &owner, base + 0x80, 2, samples) == 0 && samples.empty());
    assert(selections > oldSelections && selectedSerial == originalSerial);
    readResult = -2;
    takeReadCalls();
    const int oldReads = reads;
    holdOpen = false;
    waitFor([&] { return ctx.StartComunication.load(); });
    SDL_Delay(25);
    // Previous subscriptions and plot owners are not resurrected after recovery.
    assert(reads == oldReads && takeReadCalls().empty());
    DataReadArray(&ctx, base + 0x47, 4);
    waitFor([&] { return cached(ctx, 0x47) == 0xAB; });
    StopCommunication(&ctx);

    // A short/failed write and another queued write must never be replayed.
    for (int failedWriteResult : {1, 0}) {
        startAndWait(ctx);
        assert(!GetJLinkConnectionStatus().writeMayHaveCompleted);
        holdWrites = true;
        const int entered = enteredWrites, oldWrites = writes;
        writeResult = failedWriteResult;
        assert(DataWriteArray(&ctx, base + 0x900, value, 4) == 4);
        waitFor([&] { return enteredWrites == entered + 1; });
        assert(DataWriteArray(&ctx, base + 0xA00, value, 4) == 4);
        const int beforeReconnect = opens;
        holdOpen = true; holdWrites = false;
        waitFor([&] { return opens > beforeReconnect; });
        assert(!ctx.StartComunication && cachedAt(ctx, base + 0x900) == 0);
        assert(cachedAt(ctx, base + 0xA00) == 0 && lfqueue_size(&ctx.tx_Queue) == 0);
        assert(writes == oldWrites + 1);
        assert(GetJLinkConnectionStatus().writeMayHaveCompleted);
        assert(DataWriteArray(&ctx, base + 0xB00, value, 4) == 0);
        writeResult = -2; holdOpen = false;
        waitFor([&] { return ctx.StartComunication.load(); });
        SDL_Delay(30);
        assert(writes == oldWrites + 1);
        assert(GetJLinkConnectionStatus().writeMayHaveCompleted);
        assert(DataWriteArray(&ctx, base + 0xB00, value, 4) == 4);
        waitFor([&] { return writes == oldWrites + 2; });
        StopCommunication(&ctx);
        assert(GetJLinkConnectionStatus().writeMayHaveCompleted);
    }

    // Even a completely idle connection checks target power and connectivity.
    // Recover on the third attempt, then fail the next outage five times: a
    // successful automatic reconnect must reset (not consume) the retry budget.
    startAndWait(ctx);
    const int beforeHW = heartbeats, beforeConnected = connectedChecks;
    waitFor([&] { return heartbeats > beforeHW && connectedChecks > beforeConnected; });
    int beforeOutage = opens;
    holdOpen = true; connectedResult = 0;
    waitFor([&] { return opens > beforeOutage; });
    assert(!ctx.StartComunication);
    connectedResult = 1; failNextOpens = 2; holdOpen = false;
    waitFor([&] { return opens == beforeOutage + 3 && ctx.StartComunication.load(); });
    beforeOutage = opens;
    openFail = 1; hwResult = -1;
    waitForFailure(ctx);
    assert(opens == beforeOutage + 5);
    assert(lfqueue_size(&ctx.tx_Queue) == 0);
    assert(DataWriteArray(&ctx, base, value, 4) == 0);
    StopCommunication(&ctx);
    openFail = 0; hwResult = 0;

    // Heartbeat loss also recovers automatically, still selecting the first
    // probe's serial rather than attaching to another attached emulator.
    startAndWait(ctx);
    const int beforeOpen = opens, beforeSelect = selections;
    probeSerial = originalSerial + 1;
    holdOpen = true; targetVoltage = 0;
    waitFor([&] { return opens > beforeOpen; });
    assert(!ctx.StartComunication);
    assert(selections > beforeSelect && selectedSerial == originalSerial);
    targetVoltage = 3300; probeSerial = originalSerial; holdOpen = false;
    waitFor([&] { return ctx.StartComunication.load(); });
    const int beforeClose = closes;
    StopCommunication(&ctx);
    assert(closes == beforeClose + 1);
    StopCommunication(&ctx);
    assert(closes == beforeClose + 1);
    const int stoppedOpens = opens, stoppedReads = reads, stoppedWrites = writes, stoppedHW = heartbeats;
    SDL_Delay(30);
    assert(opens == stoppedOpens && reads == stoppedReads && writes == stoppedWrites && heartbeats == stoppedHW);
    assert(!ctx.jlink_opened && !ctx.StartComunication && !IsCommunicationRequested());
    assert(GetJLinkConnectionStatus().state == JLinkConnectionState::Disconnected);
    assert(sdkActive == 0 && sdkPeak == 1);
}
int main() {
    addressTests();
    queueAllocationTests();
    MainContext ctx;
    SDL_mutex mutex; ctx.Datalock = &mutex;
    ctx.IOMAP_Data = static_cast<uint8_t*>(std::calloc(IOMAP_SIZE, 1));
    assert(ctx.IOMAP_Data);
    assert(lfqueue_init(&ctx.tx_Queue) == 0);
    uint8_t value[] = {1, 2, 3, 4};
    assert(DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4) == 0);
    startupTests(ctx);
    startAndWait(ctx);
    const int count = opens; StartCommunication(&ctx); assert(opens == count);
    assert(DataReadArray(&ctx, 0, 1) == 0);
    assert(DataReadArray(&ctx, ctx.mcu_memory_base - 1, 1) == 0);
    assert(DataReadArray(&ctx, ctx.mcu_memory_base + IOMAP_SIZE - 1, 2) == 0);
    assert(DataReadArray(&ctx, ctx.mcu_memory_base, 0) == 0);
    assert(DataWriteArray(&ctx, ctx.mcu_memory_base, nullptr, 4) == 0);
    assert(DataWriteArray(&ctx, ctx.mcu_memory_base + 5, value, 4) == 4);
    waitFor([&] { return writes == 1; });
    StopCommunication(&ctx);
    assert(lastAddress == ctx.mcu_memory_base + 5 && lastSize == 4);
    assert(std::memcmp(written, value, 4) == 0);
    startAndWait(ctx);
    assert(DataReadArray(&ctx, ctx.mcu_memory_base + IOMAP_SIZE - 4, 4) == 4);
    waitFor([&] { return cached(ctx, IOMAP_SIZE - 1) == 0xAB; });
    assert(lastAddress == ctx.mcu_memory_base + IOMAP_SIZE - 4);
    StopCommunication(&ctx); assert(cached(ctx, IOMAP_SIZE - 1) == 0);
    recoveryTests(ctx);
    // Explicitly queued stale writes/reads are released, not replayed.
    ctx.StartComunication = true;
    DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4);
    DataReadArray(&ctx, ctx.mcu_memory_base, 4);
    StopCommunication(&ctx);
    assert(lfqueue_size(&ctx.tx_Queue) == 0);
    const int oldWrites = writes, oldReads = reads;
    startAndWait(ctx); SDL_Delay(10); StopCommunication(&ctx);
    assert(writes == oldWrites && reads == oldReads);
    // Upper edge of the 32-bit address space, including wraparound rejection.
    ctx.mcu_memory_base = 0xFFFFFFF0;
    ctx.StartComunication = true;
    assert(DataReadArray(&ctx, UINT32_MAX, 1) == 1);
    assert(DataReadArray(&ctx, UINT32_MAX, 2) == 0);
    StopCommunication(&ctx);
    // Unaligned little-endian cache reads and floating-point bit preservation.
    ctx.mcu_memory_base = 0x20000000;
    const uint64_t u64 = UINT64_C(0xFEDCBA9876543210);
    for (unsigned i = 0; i < 8; ++i) ctx.IOMAP_Data[1 + i] = static_cast<uint8_t>(u64 >> (8 * i));
    assert(DataReadU64(&ctx, ctx.mcu_memory_base + 1) == u64);
    const float real = -123.5f;
    std::memcpy(ctx.IOMAP_Data + 17, &real, sizeof(real));
    assert(DataReadFT32(&ctx, ctx.mcu_memory_base + 17) == real);
    // Typed writes preserve all 64-bit/float bits, including unaligned offsets.
    startAndWait(ctx);
    uint64_t sent = u64;
    int before = writes;
    assert(DataWriteU64(&ctx, &sent, ctx.mcu_memory_base + 1) == u64);
    waitFor([&] { return writes == before + 1; });
    StopCommunication(&ctx);
    for (unsigned i = 0; i < 8; ++i) assert(written[i] == static_cast<uint8_t>(u64 >> (8 * i)));
    startAndWait(ctx);
    float sentFloat = real; before = writes;
    assert(DataWriteFT32(&ctx, &sentFloat, ctx.mcu_memory_base + 17) == real);
    waitFor([&] { return writes == before + 1; });
    StopCommunication(&ctx);
    assert(std::memcmp(written, &real, sizeof(real)) == 0);
    // Multiple producers exercise actual bundled queue under the application lock.
    startAndWait(ctx);
    std::vector<std::thread> producers;
    for (int t = 0; t < 4; ++t) producers.emplace_back([&] {
        for (int i = 0; i < 500; ++i) {
            DataReadArray(&ctx, ctx.mcu_memory_base, 4);
            DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4);
        }
    });
    SDL_Delay(2); StopCommunication(&ctx);
    for (auto& producer : producers) producer.join();
    assert(lfqueue_size(&ctx.tx_Queue) == 0);
    pollingTests(ctx);
    lfqueue_destroy(&ctx.tx_Queue);
    std::free(ctx.IOMAP_Data);
    std::cout << "Transport regression tests passed\n";
}
