// widget_common.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "ScalarFormat.h"
#include "MemoryBounds.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "implot.h"
#include <implot_internal.h>

#include "Main.h"       

// Constants
constexpr auto MAX_BUFF = 64;
constexpr auto BUFF_SIZE = 4096;
constexpr auto MAX_ITEMS = 64;
constexpr auto MAX_ITEM_LEN = 64;

#define BOOLEAN      0
#define S08_DEC      1
#define U08_DEC      2
#define S16_DEC      3
#define U16_DEC      4
#define S32_DEC      5
#define U32_DEC      6
#define FLOAT        7
#define FLOAT_INV    8
#define S64_DEC      9
#define U64_DEC     10

using namespace tinyxml2;

// Swaps the bytes of a 16-bit value
inline uint16_t Swap16(uint16_t v) {
    return (v >> 8) | (v << 8);
}

inline uint32_t SwapLeToBe32(uint32_t u32Value) {
    return ((u32Value >> 24) & 0x000000FF) |
        ((u32Value >> 8) & 0x0000FF00) |
        ((u32Value << 8) & 0x00FF0000) |
        ((u32Value << 24) & 0xFF000000);
}

inline uint32_t SwapBeToLe32(uint32_t u32Value) {
    return SwapLeToBe32(u32Value);
}

// 64-bit helpers
inline uint64_t SwapLeToBe64(uint64_t v) {
    return ((v >> 56) & 0x00000000000000FFULL) |
        ((v >> 40) & 0x000000000000FF00ULL) |
        ((v >> 24) & 0x0000000000FF0000ULL) |
        ((v >> 8) & 0x00000000FF000000ULL) |
        ((v << 8) & 0x000000FF00000000ULL) |
        ((v << 24) & 0x0000FF0000000000ULL) |
        ((v << 40) & 0x00FF000000000000ULL) |
        ((v << 56) & 0xFF00000000000000ULL);
}

inline uint64_t SwapBeToLe64(uint64_t v) {
    return SwapLeToBe64(v);
}


#ifdef __cplusplus
extern "C" {
#endif
    extern bool DataReadWriteBoolean(MainContext* ctx, XMLElement* e, uint8_t* val, uint32_t* addr);
    extern uint8_t DataReadWriteU08(MainContext* ctx, XMLElement* e, uint8_t* val, uint32_t* addr);
    extern uint16_t DataReadWriteU16(MainContext* ctx, XMLElement* e, uint16_t* val, uint32_t* addr);
    extern float DataReadWriteFT32(MainContext* ctx, XMLElement* e, float* val, uint32_t* addr);
    extern uint32_t DataReadWriteU32(MainContext* ctx, XMLElement* e, uint32_t* val, uint32_t* addr);
    extern uint64_t DataReadWriteU64(MainContext* ctx, XMLElement* e, uint64_t* val, uint32_t* addr);
    extern uint16_t DataReadWriteArray(MainContext* ctx, XMLElement* e, uint8_t* data, uint32_t* phys_addr, uint16_t size);
#ifdef __cplusplus
}
#endif

// Other external functions (C++ linkage)
extern const char* file_open_dialog(const char* filters);
extern const char* file_save_dialog(const char* filters);
extern XMLElement* TryFindElement(XMLElement* e, const char* name);
extern uint32_t AddressFromHex(XMLElement* e);
extern float average(const std::vector<float>& v);
extern std::vector<float> smooth(const std::vector<float>& input, int window);
// Plot sampling (DataHelper.cpp): values of [phys_addr, phys_addr + size) captured by the
// communication thread since this owner's previous call, oldest first; keeps the range polled.
extern size_t DataReadSamples(MainContext* ctx, const void* owner, uint32_t phys_addr, uint16_t size, std::vector<DataSample>& out);
// Polling cycles in which every subscribed block was read and published to the cache.
extern uint32_t DataCompletedCycles(void);
// Seconds on the clock used for DataSample::time.
extern double DataTimeNow(void);

// Typed reads/writes with byte order (ReadTypedValue, WriteTypedValue)
#include "TypedValue.h"

// Inline helpers

struct ColorRGBA {
    float R, G, B, A;
};

// IM_COL32 color with another alpha (IM_COL32 keeps red in the low byte).
inline ImVec4 WithAlpha(ImU32 color, float alpha) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
    c.w = alpha;
    return c;
}

inline ColorRGBA ColorConverter(int hexValue) {
    ColorRGBA C;
    C.R = ((hexValue >> 24) & 0xFF) / 255.0f;
    C.G = ((hexValue >> 16) & 0xFF) / 255.0f;
    C.B = ((hexValue >> 8) & 0xFF) / 255.0f;
    C.A = ((hexValue >> 0) & 0xFF) / 255.0f;
    return C;
}

inline void EnsureXmlExtension(char* path, size_t maxLen) {
    size_t len = strlen(path);
    if (len < 4 || (SDL_strcasecmp(path + len - 4, ".xml") != 0)) {
        // No .xml at the end (or shorter than 4 characters): append it
        if (len + 5 <= maxLen) { // 5 = strlen(".xml") + 1
            SDL_strlcat(path, ".xml", maxLen);
        }
    }
}

inline char* GetFilePath(const char* Config) {
    char* data_path = NULL;
    char* base_path = SDL_GetBasePath();

    if (base_path) {
        data_path = (char*)SDL_malloc(MAX_PATH);
        if (!data_path)
        {
            fprintf(stderr, "Unable to allocate memory\r\n");
            SDL_free(base_path);
            return NULL;
        }

        SDL_memset(data_path, 0, MAX_PATH);
        SDL_strlcpy(&data_path[0], base_path, MAX_PATH);
        SDL_strlcpy(&data_path[SDL_strlen(data_path)], Config, MAX_PATH - SDL_strlen(data_path));
        SDL_free(base_path);   // SDL_GetBasePath() allocates the string
    }
    else {
        data_path = SDL_strdup(Config);
        if (!data_path)
            fprintf(stderr, "Unable to allocate memory\r\n");
    }

    return data_path;
}

// Global plot buffer (defined in xml_window.cpp)
extern float VirtualBuffer[MAX_BUFF][BUFF_SIZE];

// Widget base classes
struct MainContext; // defined in Main.h

class UIWidget {
public:
    virtual ~UIWidget() = default;
    virtual void Draw(MainContext* ctx, uint32_t* dynamicAddr = nullptr) = 0;
};

// Window data (defined here so that other files can use it)
struct WindowData {
    std::string name;
    std::unique_ptr<UIWidget> root;
    bool show = false;
    // <Window> element in ctx->CurrentXMLDoc when the windows were built from the open
    // document (design mode); only compared, never dereferenced.
    const tinyxml2::XMLElement* source = nullptr;
    // Geometry from <PosX>/<PosY>/<Width>/<Height>, applied once after loading
    bool hasPos = false, hasSize = false, applyGeometry = false;
    ImVec2 pos, size;
    // Design mode: geometry of the previous frame, to notice when the user moves the window
    bool seen = false, geometryChanged = false;
    ImVec2 seenPos, seenSize;
};

// Global window list (defined in xml_window.cpp)
extern std::vector<WindowData> g_Windows;

class LeafWidget : public UIWidget {
protected:
    std::string text;
public:
    LeafWidget(const char* txt = "") : text(txt ? txt : "") {}
};

class ContainerWidget : public UIWidget {
protected:
    std::vector<std::unique_ptr<UIWidget>> children;

    // Each child gets its own ID scope, so children with the same or an empty label
    // neither collide nor end up with the window's own ID (an ImGui assertion).
    void DrawChildren(MainContext* ctx, uint32_t* dynamicAddr) {
        for (size_t i = 0; i < children.size(); ++i) {
            ImGui::PushID((int)i);
            children[i]->Draw(ctx, dynamicAddr);
            ImGui::PopID();
        }
    }
public:
    void AddChild(std::unique_ptr<UIWidget> child) {
        if (child) children.push_back(std::move(child));
    }
};