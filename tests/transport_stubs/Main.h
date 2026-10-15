#pragma once
#include "../../JLinkConnection.h"
// Test-only SDL/J-Link facade. Production DataHelper.cpp is compiled unchanged.
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <tinyxml2/tinyxml2.h>
#include <lfqueue.h>
#define IOMAP_SIZE (1024 * 1024)
using SDL_mutex = std::mutex;
struct SDL_Thread { std::thread thread; };
enum { SDL_THREAD_PRIORITY_TIME_CRITICAL, SDL_THREAD_PRIORITY_NORMAL };
inline std::atomic<bool> failThread{false};
inline void* SDL_malloc(std::size_t n) { return std::malloc(n); }
inline void SDL_free(void* p) { std::free(p); }
inline int SDL_LockMutex(SDL_mutex* p) { p->lock(); return 0; }
inline int SDL_UnlockMutex(SDL_mutex* p) { p->unlock(); return 0; }
inline int SDL_SetThreadPriority(int) { return 0; }
inline void SDL_Delay(unsigned ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline SDL_Thread* SDL_CreateThread(int (*fn)(void*), const char*, void* data) {
    if (failThread) return nullptr;
    return new SDL_Thread{std::thread([=] { fn(data); })};
}
inline void SDL_WaitThread(SDL_Thread* p, int*) { p->thread.join(); delete p; }
struct FT32 { union { uint32_t hex; float real; }; };
struct q_t { uint32_t addr; uint16_t size; };
struct iodata_t { union { q_t q; uint64_t all; }; uint8_t* value; };
struct DataSample { double time; uint8_t bytes[8]; };
struct MainContext {
    SDL_mutex* Datalock{};
    uint8_t* IOMAP_Data{};
    lfqueue_t tx_Queue{};
    std::atomic<bool> StartComunication{false};
    std::atomic<bool> jlink_opened{false};
    uint32_t mcu_memory_base = 0x20000000;
    char jlink_device[64] = "Cortex-M0";
};
enum { JLINKARM_TIF_SWD = 1 };
const char* JLINKARM_Open();
void JLINKARM_Close();
int JLINKARM_ExecCommand(const char*, char*, int);
int JLINKARM_TIF_Select(int);
int JLINKARM_Connect();
int JLINKARM_GetSpeed();
void JLINKARM_SetMaxSpeed();
void JLINKARM_SetSpeed(int);
struct JLINKARM_HW_STATUS { uint16_t VTarget; };
struct JLINKARM_DEVICE_INFO { unsigned SizeOfStruct; const char* sName; };
int JLINKARM_GetHWStatus(JLINKARM_HW_STATUS*);
char JLINKARM_IsConnected();
int JLINKARM_GetSN();
int JLINKARM_EMU_SelectByUSBSN(uint32_t);
int JLINKARM_DEVICE_GetInfo(int, JLINKARM_DEVICE_INFO*);
int JLINKARM_ReadMemEx(uint32_t, uint32_t, void*, uint32_t);
int JLINKARM_WriteMemEx(uint32_t, uint32_t, const void*, uint32_t);
extern "C" void StartCommunication(MainContext*);
extern "C" void StopCommunication(MainContext*);
