#pragma once
#include <cstdint>
// Compile the actual iniConfig.cpp without SDL/the Windows application.
struct MainContext {
    int Win_width = 1360, Win_height = 780, FontSize = 2, Style = 3;
    int Win_x = 0, Win_y = 0;
    bool Win_position_saved = false, Win_maximized = false;
    char File_path[260] = "";
    bool TopMost = false;
    bool show_xml_editor = false, show_style_editor = false, design_mode = false;
    bool auto_connect = true;
    char jlink_device[64] = "Cortex-M4";
    uint32_t mcu_memory_base = 0x20000000;
    // None of these session-only fields may be touched or serialized.
    bool write_permission = false, armed = false, connected = false;
};
void LoadIniSettings(void*);
void SaveIniSettings(void*);
std::string getDirectoryWithCurrentExecutable();
