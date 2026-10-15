// WatchWidget.h
#pragma once
#include "widget_common.h"
#include <cstdint>

// Additional data types
#define S64_DEC  9
#define U64_DEC 10

class WatchWidget : public UIWidget {
private:
    static const int MAX_ROWS = 128;
    static const int MAX_STR_LEN = 128;

    struct WatchRow {
        bool enabled;
        int dataType; // BOOLEAN, S08_DEC, U08_DEC, S16_DEC, U16_DEC, S32_DEC, U32_DEC, S64_DEC, U64_DEC, FLOAT, FLOAT_INV
        char name[MAX_STR_LEN];
        char desc[MAX_STR_LEN];
        int address;
        uint64_t value; // 64-bit value (for all types)
        bool bitMode;
        bool littleEndian;
        bool hexMode; // show integers in hex
    };

    static std::vector<WatchRow> s_rows;
    static bool s_modified;
    static float s_updateInterval;
    static float s_opacity;
    static double s_refreshTime;
    static int s_contextMenuRow;
    static int s_selectedRow;
    static int s_highlightedRow;
    static char s_xmlPath[MAX_PATH];

    static void LoadFromXML(const char* path);
    static void SaveToXML(const char* path);
    static void DrawRowContextMenu();
    static bool IsIntegerType(int dt);
    static int GetDataTypeBits(int dt);

public:
    WatchWidget();
    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override;
};