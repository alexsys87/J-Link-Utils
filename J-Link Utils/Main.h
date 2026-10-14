#ifndef __MAIN_H__
#define __MAIN_H__

#define main SDL_main
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

#define IOMAP_SIZE (1024 * 1024) // 1 МБ

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
#define snprintf    _snprintf_s
#define vsnprintf   _vsnprintf
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
    uint32_t  addr;   // смещение в IOMAP_Data
    uint16_t  size;   // размер данных в байтах
} q_t;

typedef struct {
    union {
        q_t        q;
        uint64_t all;
    };
    uint8_t* value;  // указатель на данные (для записи) или NULL для чтения
} iodata_t;

typedef struct MainContext {
    SDL_Window* Wimdow;
    SDL_Renderer* Renderer;
    SDL_mutex* Datalock;
    SDL_cond* Datacond;
    lfqueue_t rx_Queue;
    lfqueue_t tx_Queue;
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
    bool StartComunication;

    tinyxml2::XMLDocument* CurrentXMLDoc; // текущий загруженный XML
	bool show_style_editor;               // флаг отображения редактора стилей
    bool show_xml_editor;                 // флаг отображения редактора
    bool xml_modified;                    // флаг, был ли изменён XML
    bool openfile, closefile;

    //J-link
    bool jlink_opened;                    // флаг успешного открытия J‑Link
    bool auto_connect;                    // автоматически подключаться при старте
    char jlink_device[64];                // строка с именем устройства
    uint32_t   mcu_memory_base;           // базовый адрес памяти MCU для Cortex M (обычно 0x20000000)
} MainContext;


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