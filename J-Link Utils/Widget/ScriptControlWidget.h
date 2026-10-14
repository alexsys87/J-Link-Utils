//// ScriptControlWidget.h
//#pragma once
//#include "widget_common.h"
//#include <string>
//#include <sstream>
//#include <vector>
//
//// Подключаем заголовочный файл Lua.
//// Вам нужно добавить библиотеку lua (например, lua5.4) к линковке вашего проекта.
//#include <lua.hpp>
//#include <lauxlib.h>
//#include <lualib.h>
//
//class ScriptControlWidget : public UIWidget {
//    std::string label;
//    uint16_t baseAddress;       // Базовый адрес, с которого будут доступны переменные в Lua
//    int       numRegisters;     // Сколько 16-битных регистров доступно для скрипта
//
//    // Состояние Lua
//    lua_State* L;
//
//    // Поля для редактора и вывода
//    std::string scriptBuffer;
//    std::vector<std::string> outputLines;
//
//public:
//    ScriptControlWidget(const char* lbl, uint16_t baseAddr, int numRegs)
//        : label(lbl ? lbl : ""), baseAddress(baseAddr), numRegisters(numRegs), L(nullptr) {
//        scriptBuffer.reserve(4096);
//        scriptBuffer = "-- Пример скрипта\nprint('Hello from Lua!')\nfor i=1,5 do\n    print('i = '..i)\nend";
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
//        // Инициализируем Lua при первом вызове
//        if (!L) {
//            initLua(ctx, dynamicAddr ? *dynamicAddr : baseAddress);
//        }
//
//        ImGui::PushID(this);
//
//        // Рисуем многострочное поле для ввода скрипта
//        ImGui::InputTextMultiline("##script", &scriptBuffer[0], scriptBuffer.capacity() + 1,
//            ImVec2(-1.0f, 200.0f),
//            ImGuiInputTextFlags_AllowTabInput);
//
//        ImGui::SameLine();
//        if (ImGui::Button("Run Script")) {
//            runScript();
//        }
//
//        // Вывод (логи) скрипта
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
//        luaL_openlibs(L); // Открыть стандартные библиотеки
//
//        // --- Создаём таблицу 'memory' для доступа к регистрам ---
//        lua_newtable(L); // Создаём таблицу на вершине стека
//
//        for (int i = 0; i < numRegisters; ++i) {
//            uint16_t addr = effectiveBaseAddr + i;
//
//            // Здесь мы не можем прочитать значение прямо сейчас,
//            // потому что ctx может меняться. Поэтому мы будем передавать контекст
//            // при каждом запуске скрипта. Для упрощения в этом примере
//            // мы просто сохраним адреса. В более продвинутой версии
//            // можно создать пользовательские данные (userdata) с метатаблицами,
//            // которые при обращении будут вызывать функции чтения/записи.
//
//            // Положим в таблицу пару (индекс, адрес) в виде пользовательских данных.
//            // Но для простоты: создадим функцию, которая будет вызываться из Lua.
//            // Это проще сделать через C-функции, зарегистрированные в Lua.
//        }
//
//        // Регистрируем C-функции для доступа к памяти
//        lua_register(L, "read16", lua_read16);
//        lua_register(L, "write16", lua_write16);
//
//        // Сохраняем указатель на контекст в глобальной переменной Lua (для C-функций)
//        lua_pushlightuserdata(L, ctx);
//        lua_setglobal(L, "imgui_ctx");
//        lua_pushinteger(L, effectiveBaseAddr);
//        lua_setglobal(L, "imgui_baseAddr");
//
//        // Добавляем функцию print, которая будет добавлять строки в outputLines
//        lua_register(L, "print", lua_print);
//    }
//
//    void runScript() {
//        outputLines.clear();
//        if (!L) return;
//
//        // Устанавливаем указатель на наш виджет как световые пользовательские данные
//        // для доступа из C-функций (например, для lua_print).
//        lua_pushlightuserdata(L, this);
//        lua_setglobal(L, "script_widget");
//
//        // Загружаем и выполняем скрипт
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
//    // --- C-функции для Lua ---
//
//    // lua_read16(addr) -> value
//    static int lua_read16(lua_State* L) {
//        // Получаем контекст и базовый адрес из глобальных переменных
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
//    // Кастомная функция print для захвата вывода
//    static int lua_print(lua_State* L) {
//        int nargs = lua_gettop(L);
//        std::stringstream ss;
//
//        // Получаем указатель на виджет
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