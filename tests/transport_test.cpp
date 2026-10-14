#include <Main.h>
#include <cassert>
#include <climits>
#include <cerrno>
#include <iostream>

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
static std::atomic<int> opens{0}, closes{0}, reads{0}, writes{0};
static std::atomic<int> openFail{0}, commandFail{0}, tifFail{0}, connectFail{0};
static std::atomic<int> readResult{-2}, writeResult{-2};
static std::atomic<uint32_t> lastAddress{0};
static std::atomic<int> lastSize{0};
static uint8_t written[65536];
const char* JLINKARM_Open() { ++opens; return openFail ? "mock failure" : nullptr; }
void JLINKARM_Close() { ++closes; }
int JLINKARM_ExecCommand(const char*, char*, int) { return commandFail; }
int JLINKARM_TIF_Select(int) { return tifFail; }
int JLINKARM_Connect() { return connectFail; }
int JLINKARM_GetSpeed() { return 8000; }
void JLINKARM_SetMaxSpeed() {}
void JLINKARM_SetSpeed(int) {}
int JLINKARM_ReadMem(uint32_t address, uint32_t n, void* out) {
    lastAddress = address; lastSize = static_cast<int>(n);
    std::memset(out, 0xAB, n);
    ++reads;
    return readResult == -2 ? static_cast<int>(n) : readResult.load();
}
int JLINKARM_WriteMem(uint32_t address, uint32_t n, const void* in) {
    lastAddress = address; lastSize = static_cast<int>(n);
    std::memcpy(written, in, n);
    ++writes;
    return writeResult == -2 ? static_cast<int>(n) : writeResult.load();
}
template<class Predicate> void waitFor(Predicate predicate) {
    for (int i = 0; i < 2000 && !predicate(); ++i) SDL_Delay(1);
    assert(predicate());
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
int main() {
    addressTests();
    queueAllocationTests();
    MainContext ctx;
    SDL_mutex mutex; ctx.Datalock = &mutex;
    ctx.IOMAP_Data = static_cast<uint8_t*>(std::calloc(IOMAP_SIZE, 1));
    assert(ctx.IOMAP_Data);
    assert(lfqueue_init(&ctx.rx_Queue) == 0);
    assert(lfqueue_init(&ctx.tx_Queue) == 0);
    uint8_t value[] = {1, 2, 3, 4};
    assert(DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4) == 0);
    openFail = 1; StartCommunication(&ctx);
    assert(!ctx.StartComunication && !ctx.jlink_opened && closes == 0);
    openFail = 0; commandFail = -1; StartCommunication(&ctx);
    assert(!ctx.StartComunication && closes == 1);
    commandFail = 0; tifFail = -1; StartCommunication(&ctx);
    assert(!ctx.StartComunication && closes == 2);
    tifFail = 0; connectFail = -1; StartCommunication(&ctx);
    assert(!ctx.StartComunication && closes == 3);
    connectFail = 0; failThread = true; StartCommunication(&ctx);
    assert(!ctx.StartComunication && !ctx.jlink_opened && closes == 4);
    failThread = false; StartCommunication(&ctx);
    assert(ctx.StartComunication && ctx.jlink_opened);
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
    StartCommunication(&ctx);
    assert(DataReadArray(&ctx, ctx.mcu_memory_base + IOMAP_SIZE - 4, 4) == 4);
    waitFor([&] { return cached(ctx, IOMAP_SIZE - 1) == 0xAB; });
    assert(lastAddress == ctx.mcu_memory_base + IOMAP_SIZE - 4);
    StopCommunication(&ctx); assert(cached(ctx, IOMAP_SIZE - 1) == 0);
    // Failed/short reads must never publish incomplete memory to the GUI.
    StartCommunication(&ctx);
    { std::lock_guard<std::mutex> lock(mutex); ctx.IOMAP_Data[7] = 0x42; }
    readResult = 1;
    DataReadArray(&ctx, ctx.mcu_memory_base + 7, 4);
    waitFor([&] { return !ctx.StartComunication; });
    assert(cached(ctx, 7) == 0x42);
    const int oldOpens = opens;
    readResult = -2; StartCommunication(&ctx);
    assert(opens == oldOpens + 1 && ctx.StartComunication);
    assert(cached(ctx, 7) == 0);
    writeResult = 0;
    DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4);
    waitFor([&] { return !ctx.StartComunication; });
    StopCommunication(&ctx); writeResult = -2;
    // Explicitly queued stale writes/reads are released, not replayed.
    ctx.StartComunication = true;
    DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4);
    DataReadArray(&ctx, ctx.mcu_memory_base, 4);
    StopCommunication(&ctx);
    assert(lfqueue_size(&ctx.rx_Queue) == 0 && lfqueue_size(&ctx.tx_Queue) == 0);
    const int oldWrites = writes, oldReads = reads;
    StartCommunication(&ctx); SDL_Delay(10); StopCommunication(&ctx);
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
    StartCommunication(&ctx);
    uint64_t sent = u64;
    int before = writes;
    assert(DataWriteU64(&ctx, &sent, ctx.mcu_memory_base + 1) == u64);
    waitFor([&] { return writes == before + 1; });
    StopCommunication(&ctx);
    for (unsigned i = 0; i < 8; ++i) assert(written[i] == static_cast<uint8_t>(u64 >> (8 * i)));
    StartCommunication(&ctx);
    float sentFloat = real; before = writes;
    assert(DataWriteFT32(&ctx, &sentFloat, ctx.mcu_memory_base + 17) == real);
    waitFor([&] { return writes == before + 1; });
    StopCommunication(&ctx);
    assert(std::memcmp(written, &real, sizeof(real)) == 0);
    // Multiple producers exercise actual bundled queue under the application lock.
    StartCommunication(&ctx);
    std::vector<std::thread> producers;
    for (int t = 0; t < 4; ++t) producers.emplace_back([&] {
        for (int i = 0; i < 500; ++i) {
            DataReadArray(&ctx, ctx.mcu_memory_base, 4);
            DataWriteArray(&ctx, ctx.mcu_memory_base, value, 4);
        }
    });
    SDL_Delay(2); StopCommunication(&ctx);
    for (auto& producer : producers) producer.join();
    assert(lfqueue_size(&ctx.rx_Queue) == 0 && lfqueue_size(&ctx.tx_Queue) == 0);
    lfqueue_destroy(&ctx.rx_Queue); lfqueue_destroy(&ctx.tx_Queue);
    std::free(ctx.IOMAP_Data);
    std::cout << "Transport regression tests passed\n";
}
