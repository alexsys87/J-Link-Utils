#ifndef __MAIN_H__
#define __MAIN_H__

#define main SDL_main
#include <atomic>
#include "WriteProtectionRules.h"
#include "JLinkConnection.h"
#include <SDL.h>
#include <SDL_syswm.h>

#include <imgui.h>
#include <implot.h>
#include <imgui_memory_editor.h>

#include <backends/imgui_impl_sdlrenderer2.h>
#include <backends/imgui_impl_sdl2.h>

#include <lfqueue.h>

#include <tinyxml2/tinyxml2.h>

#include <JLinkARMDLL.h>

#define IOMAP_SIZE (1024 * 1024) // 1 MiB

#define MAX_PATH 260

#define WINDOW_WIDTH     1360
#define WINDOW_HEIGHT    780

#define MAXIMUM_WINDOWS  16

//#define EXTRA_FONT

#define MAXIMUM_FONT 10

#ifdef _DEBUG
#define DEBUG_PRINTF(x) SDL_Log(x)
#else
#define DEBUG_PRINTF(x) do{}while(0);
#endif

// Play it nice with Windows users.
#ifdef _WIN32
#define IM_NEWLINE  "\r\n"
// Modern MSVC provides standard snprintf/vsnprintf, including safe truncation.
#define sscanf      sscanf_s
#define sprintf     sprintf_s
#define strcpy	    strcpy_s
#define strncpy	    strncpy_s
#else
#define IM_NEWLINE  "\n"
#endif

#ifdef _WIN32
/** Use to init the clock */
#define TIMER_INIT \
    LARGE_INTEGER frequency; \
    LARGE_INTEGER t1,t2; \
    double elapsedTime; \
    QueryPerformanceFrequency(&frequency);

/** Use to start the performance timer */
#define TIMER_START QueryPerformanceCounter(&t1);

/** Use to stop the performance timer and output the result to the standard stream. */
#define TIMER_STOP \
    QueryPerformanceCounter(&t2); \
    elapsedTime=(float)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart; \
    printf("Elapsed %f sec\n", elapsedTime);

#endif

#define SDL_TIMER_INIT \
    uint64_t t1,t2; \
    double elapsedTime; 

#define SDL_TIMER_START \
    t1 = SDL_GetPerformanceCounter();

#define SDL_TIMER_STOP \
    t2 = SDL_GetPerformanceCounter(); \
    elapsedTime=(float)(t2 - t1) / (float)SDL_GetPerformanceFrequency(); \
    printf("Elapsed %f sec\n", elapsedTime);


#define CUSTOM_STYLE_FILE "custom.style.xml"

typedef struct {
    union {
        uint32_t hex;
        float    real;
    };
} FT32;

typedef struct {
    uint32_t  addr;   // offset in IOMAP_Data
    uint16_t  size;   // data size in bytes
} q_t;

typedef struct {
    union {
        q_t        q;
        uint64_t all;
    };
    uint8_t* value;  // data to write, or NULL for a read
    uint64_t write_epoch; // permission/session generation captured on enqueue
} iodata_t;

// One value captured by the communication thread for the plots.
typedef struct {
    double  time;      // seconds, same clock as DataTimeNow()
    uint8_t bytes[8];  // raw target bytes in memory order
} DataSample;

typedef struct MainContext {
    SDL_Window* Wimdow;
    SDL_Renderer* Renderer;
    SDL_mutex* Datalock;
    SDL_cond* Datacond;
    lfqueue_t tx_Queue;                   // writes for the communication thread (reads are polled)
    // Window
    int Win_width, Win_height;
    int FontSize;
    int Style;
    char File_path[MAX_PATH];
    char File_label[64];
    bool file_loaded;
    bool TopMost;
    bool ErrRengerer;
    bool IsClosing;
    bool StateMinimized;
    uint8_t* IOMAP_Data;
    bool Sync;
    std::atomic<bool> StartComunication{false};

    tinyxml2::XMLDocument* CurrentXMLDoc; // currently loaded XML
	bool show_style_editor;               // show the style editor
    bool show_xml_editor;                 // show the XML editor
    bool xml_modified;                    // XML has unsaved changes
    uint32_t xml_revision;                // incremented by every change of CurrentXMLDoc
    bool design_mode;                     // widgets are edited with the mouse (Editor/DesignMode.cpp)
    bool openfile, closefile;

    //J-link
    std::atomic<bool> jlink_opened{false}; // observed by UI; SDK session owned by worker
    bool auto_connect;                    // connect automatically on start
    char jlink_device[64];                // J-Link device name
    uint32_t   mcu_memory_base;           // MCU memory base address for Cortex-M (usually 0x20000000)
} MainContext;


// Every change of ctx->CurrentXMLDoc goes through here: the design mode rebuilds the
// windows from the document when the revision changes.
inline void MarkXMLModified(MainContext* ctx) {
    ctx->xml_modified = true;
    ++ctx->xml_revision;
}

int Draw_Window(MainContext* ctx);

void LoadIniSettings(void* arg);
void SaveIniSettings(void* arg);

extern "C" {
    void ClearAllWindows(void);
    void StartCommunication(MainContext* ctx);
    void StopCommunication(MainContext* ctx);
    void GetSetMaxSpeed(void);
}

void LoadWindowsFromXML(MainContext* ctx, const char* filename);

#endif // __MAIN_H__