//// ScriptControlWidget.h
//#pragma once
//#include "widget_common.h"
//#include <string>
//#include <sstream>
//#include <vector>
//
//// Lua header.
//// The lua library (e.g. lua5.4) must be added to the linker inputs.
//#include <lua.hpp>
//#include <lauxlib.h>
//#include <lualib.h>
//
//class ScriptControlWidget : public UIWidget {
//    std::string label;
//    uint16_t baseAddress;       // Base address of the variables exposed to Lua
//    int       numRegisters;     // Number of 16-bit registers available to the script
//
//    // Lua state
//    lua_State* L;
//
//    // Editor and output fields
//    std::string scriptBuffer;
//    std::vector<std::string> outputLines;
//
//public:
//    ScriptControlWidget(const char* lbl, uint16_t baseAddr, int numRegs)
//        : label(lbl ? lbl : ""), baseAddress(baseAddr), numRegisters(numRegs), L(nullptr) {
//        scriptBuffer.reserve(4096);
//        scriptBuffer = "-- Example script\nprint('Hello from Lua!')\nfor i=1,5 do\n    print('i = '..i)\nend";
//    }
//
//    ~ScriptControlWidget() {
//        if (L) {
//            lua_close(L);
//            L = nullptr;
//        }
//    }
//
//    void Draw(MainContext* ctx, uint16_t* dynamicAddr) override {
//        // Initialize Lua on the first call
//        if (!L) {
//            initLua(ctx, dynamicAddr ? *dynamicAddr : baseAddress);
//        }
//
//        ImGui::PushID(this);
//
//        // Multi-line script editor
//        ImGui::InputTextMultiline("##script", &scriptBuffer[0], scriptBuffer.capacity() + 1,
//            ImVec2(-1.0f, 200.0f),
//            ImGuiInputTextFlags_AllowTabInput);
//
//        ImGui::SameLine();
//        if (ImGui::Button("Run Script")) {
//            runScript();
//        }
//
//        // Script output (log)
//        if (!outputLines.empty()) {
//            ImGui::Separator();
//            ImGui::TextUnformatted("Output:");
//            ImGui::BeginChild("##output", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
//            for (const auto& line : outputLines) {
//                ImGui::TextUnformatted(line.c_str());
//            }
//            ImGui::EndChild();
//        }
//
//        ImGui::PopID();
//    }
//
//private:
//    void initLua(MainContext* ctx, uint16_t effectiveBaseAddr) {
//        L = luaL_newstate();
//        luaL_openlibs(L); // Open the standard libraries
//
//        // --- 'memory' table for register access ---
//        lua_newtable(L); // New table on top of the stack
//
//        for (int i = 0; i < numRegisters; ++i) {
//            uint16_t addr = effectiveBaseAddr + i;
//
//            // The value cannot be read right here,
//            // because ctx may change, so the context is passed
//            // on every script run. To keep this example simple
//            // only the addresses are stored. A more advanced version
//            // could use userdata with metatables
//            // that call the read/write functions on access.
//
//            // Put (index, address) pairs into the table as userdata.
//            // Simpler: a function called from Lua.
//            // That is easiest with C functions registered in Lua.
//        }
//
//        // Register C functions for memory access
//        lua_register(L, "read16", lua_read16);
//        lua_register(L, "write16", lua_write16);
//
//        // Store the context pointer in a Lua global (for the C functions)
//        lua_pushlightuserdata(L, ctx);
//        lua_setglobal(L, "imgui_ctx");
//        lua_pushinteger(L, effectiveBaseAddr);
//        lua_setglobal(L, "imgui_baseAddr");
//
//        // print replacement that appends lines to outputLines
//        lua_register(L, "print", lua_print);
//    }
//
//    void runScript() {
//        outputLines.clear();
//        if (!L) return;
//
//        // Store a pointer to this widget as light userdata
//        // for the C functions (e.g. lua_print).
//        lua_pushlightuserdata(L, this);
//        lua_setglobal(L, "script_widget");
//
//        // Load and run the script
//        int status = luaL_loadstring(L, scriptBuffer.c_str());
//        if (status != LUA_OK) {
//            outputLines.push_back(std::string("Error: ") + lua_tostring(L, -1));
//            lua_pop(L, 1);
//            return;
//        }
//
//        status = lua_pcall(L, 0, 0, 0);
//        if (status != LUA_OK) {
//            outputLines.push_back(std::string("Runtime Error: ") + lua_tostring(L, -1));
//            lua_pop(L, 1);
//            return;
//        }
//    }
//
//    // --- C functions for Lua ---
//
//    // lua_read16(addr) -> value
//    static int lua_read16(lua_State* L) {
//        // Get the context and the base address from the globals
//        lua_getglobal(L, "imgui_ctx");
//        MainContext* ctx = (MainContext*)lua_touserdata(L, -1);
//        lua_pop(L, 1);
//
//        lua_getglobal(L, "imgui_baseAddr");
//        uint16_t baseAddr = (uint16_t)lua_tointeger(L, -1);
//        lua_pop(L, 1);
//
//        int addr_offset = (int)luaL_checkinteger(L, 1);
//        uint16_t effAddr = baseAddr + addr_offset;
//
//        uint16_t val = DataReadWriteU16(ctx, nullptr, nullptr, &effAddr);
//        lua_pushinteger(L, val);
//        return 1;
//    }
//
//    // lua_write16(addr, value)
//    static int lua_write16(lua_State* L) {
//        lua_getglobal(L, "imgui_ctx");
//        MainContext* ctx = (MainContext*)lua_touserdata(L, -1);
//        lua_pop(L, 1);
//
//        lua_getglobal(L, "imgui_baseAddr");
//        uint16_t baseAddr = (uint16_t)lua_tointeger(L, -1);
//        lua_pop(L, 1);
//
//        int addr_offset = (int)luaL_checkinteger(L, 1);
//        uint16_t val = (uint16_t)luaL_checkinteger(L, 2);
//        uint16_t effAddr = baseAddr + addr_offset;
//
//        DataReadWriteU16(ctx, nullptr, &val, &effAddr);
//        return 0;
//    }
//
//    // Custom print that captures the output
//    static int lua_print(lua_State* L) {
//        int nargs = lua_gettop(L);
//        std::stringstream ss;
//
//        // Get the widget pointer
//        lua_getglobal(L, "script_widget");
//        ScriptControlWidget* widget = (ScriptControlWidget*)lua_touserdata(L, -1);
//        lua_pop(L, 1);
//
//        for (int i = 1; i <= nargs; i++) {
//            if (i > 1) ss << "\t";
//            if (lua_isstring(L, i)) {
//                ss << lua_tostring(L, i);
//            }
//            else {
//                ss << lua_typename(L, lua_type(L, i));
//            }
//        }
//
//        if (widget) {
//            widget->outputLines.push_back(ss.str());
//        }
//        return 0;
//    }
//};