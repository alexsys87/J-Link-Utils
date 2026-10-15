// iniConfig.cpp
#include <iostream>
#include <fstream>
#include <cassert>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>

#include <Main.h>

std::string getDirectoryWithCurrentExecutable()
{
    static char szPath[MAX_PATH] = {};
    GetModuleFileNameA(NULL, szPath, MAX_PATH);

    // Cut off the file name: keep the folder of the executable
    char* lastSlash = strrchr(szPath, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
    }

    return szPath;
}

// ---------- INI file access ----------
class IniFileConfiguration {
private:
    std::map<std::string, std::string> values;
    std::string filename;

    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::pair<std::string, std::string> splitKey(const std::string& key) {
        size_t dot = key.find_last_of('.');
        if (dot == std::string::npos)
            return { "", key };
        return { key.substr(0, dot), key.substr(dot + 1) };
    }

public:
    IniFileConfiguration(const std::string& path) : filename(path) {
        load();
    }

    void load() {
        values.clear();
        std::ifstream file(filename);
        if (!file.is_open()) return;

        std::string line, currentSection;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            if (line[0] == '[') {
                size_t end = line.find(']');
                if (end != std::string::npos) {
                    currentSection = trim(line.substr(1, end - 1));
                }
            }
            else {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = trim(line.substr(0, eq));
                    std::string value = trim(line.substr(eq + 1));
                    if (!currentSection.empty())
                        key = currentSection + "." + key;
                    values[key] = value;
                }
            }
        }
    }

    void save() {
        save(filename);
    }

    void save(const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) return;

        std::map<std::string, std::map<std::string, std::string>> sections;
        for (const auto& [fullKey, val] : values) {
            auto [section, key] = splitKey(fullKey);
            sections[section][key] = val;
        }

        for (const auto& [section, keys] : sections) {
            if (!section.empty())
                file << "[" << section << "]\n";
            for (const auto& [key, val] : keys) {
                file << key << "=" << val << "\n";
            }
            file << "\n";
        }
    }

    bool has(const std::string& key) const {
        return values.find(key) != values.end();
    }

    std::string getString(const std::string& key) const {
        auto it = values.find(key);
        if (it != values.end())
            return it->second;
        return "";
    }

    void setString(const std::string& key, const std::string& value) {
        values[key] = value;
    }
};

// ---------- Loading the settings ----------
void LoadIniSettings(void* arg)
{
    assert(arg != NULL);
    MainContext* main_context = (MainContext*)arg;

    try {
        IniFileConfiguration pConf(getDirectoryWithCurrentExecutable().append("\\settings.ini"));

        // WINDOW.Style
        if (pConf.has("WINDOW.Style")) {
            try {
                main_context->Style = std::stoi(pConf.getString("WINDOW.Style"));
            }
            catch (...) {
                std::cerr << "Error parsing WINDOW.Style" << std::endl;
            }
        }
        else {
            pConf.setString("WINDOW.Style", std::to_string(main_context->Style));
            pConf.save();
        }

        // WINDOW.TopMost
        if (pConf.has("WINDOW.TopMost")) {
            try {
                main_context->TopMost = std::stoi(pConf.getString("WINDOW.TopMost")) != 0;
            }
            catch (...) {
                std::cerr << "Error parsing WINDOW.TopMost" << std::endl;
            }
        }
        else {
            pConf.setString("WINDOW.TopMost", main_context->TopMost ? "1" : "0");
            pConf.save();
        }

        // WINDOW.FontSize
        if (pConf.has("WINDOW.FontSize")) {
            try {
                main_context->FontSize = std::stoi(pConf.getString("WINDOW.FontSize"));
            }
            catch (...) {
                std::cerr << "Error parsing WINDOW.FontSize" << std::endl;
            }
        }
        else {
            pConf.setString("WINDOW.FontSize", std::to_string(main_context->FontSize));
            pConf.save();
        }

        // WINDOW.Width
        if (pConf.has("WINDOW.Width")) {
            try {
                main_context->Win_width = std::stoi(pConf.getString("WINDOW.Width"));
            }
            catch (...) {
                std::cerr << "Error parsing WINDOW.Width" << std::endl;
            }
        }
        else {
            pConf.setString("WINDOW.Width", std::to_string(main_context->Win_width));
            pConf.save();
        }

        // WINDOW.Height
        if (pConf.has("WINDOW.Height")) {
            try {
                main_context->Win_height = std::stoi(pConf.getString("WINDOW.Height"));
            }
            catch (...) {
                std::cerr << "Error parsing WINDOW.Height" << std::endl;
            }
        }
        else {
            pConf.setString("WINDOW.Height", std::to_string(main_context->Win_height));
            pConf.save();
        }

        // File.LastFile
        if (pConf.has("File.LastFile")) {
            try {
                std::string path = pConf.getString("File.LastFile");
                if (!path.empty()) {
                    SDL_strlcpy(main_context->File_path, path.c_str(), MAX_PATH);
                }
            }
            catch (...) {
                std::cerr << "Error parsing File.LastFile" << std::endl;
            }
        }
        else {
            pConf.setString("File.LastFile", "");
            pConf.save();
        }

        // MCU.Base
        if (pConf.has("MCU.Base")) {
            try {
                main_context->mcu_memory_base = std::stoul(pConf.getString("MCU.Base"), nullptr, 0);
            }
            catch (...) {
                std::cerr << "Error parsing MCU.Base" << std::endl;
            }
        }
        else {
            pConf.setString("MCU.Base", "0x20000000");
            pConf.save();
        }

        // JLink.Device
        if (pConf.has("JLink.Device")) {
            std::string dev = pConf.getString("JLink.Device");
            if (!dev.empty()) {
                strncpy(main_context->jlink_device, dev.c_str(), sizeof(main_context->jlink_device) - 1);
                main_context->jlink_device[sizeof(main_context->jlink_device) - 1] = '\0';
            }
        }
        else {
            pConf.setString("JLink.Device", main_context->jlink_device);
            pConf.save();
        }
        if (pConf.has("SETTINGS.AutoConnect")) {
            try {
                main_context->auto_connect = std::stoi(pConf.getString("SETTINGS.AutoConnect")) != 0;
            }
            catch (...) {
                main_context->auto_connect = true;
            }
        }
        else {
            pConf.setString("SETTINGS.AutoConnect", main_context->auto_connect ? "1" : "0");
            pConf.save();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in LoadIniSettings: " << e.what() << std::endl;
        std::ofstream fs("settings.ini");
        fs.close();
    }
}

// ---------- Saving the settings ----------
void SaveIniSettings(void* arg)
{
    assert(arg != NULL);
    MainContext* main_context = (MainContext*)arg;

    try {
        IniFileConfiguration pConf(getDirectoryWithCurrentExecutable().append("\\settings.ini"));

        pConf.setString("WINDOW.Style", std::to_string(main_context->Style));
        pConf.setString("WINDOW.TopMost", main_context->TopMost ? "1" : "0");
        pConf.setString("WINDOW.FontSize", std::to_string(main_context->FontSize));
        pConf.setString("WINDOW.Width", std::to_string(main_context->Win_width));
        pConf.setString("WINDOW.Height", std::to_string(main_context->Win_height));
        pConf.setString("File.LastFile", main_context->File_path);
        char mcuBaseHex[16];
        snprintf(mcuBaseHex, sizeof(mcuBaseHex), "0x%08X", main_context->mcu_memory_base);
        pConf.setString("MCU.Base", mcuBaseHex);
        pConf.setString("JLink.Device", main_context->jlink_device);
        pConf.setString("SETTINGS.AutoConnect", main_context->auto_connect ? "1" : "0");

        pConf.save();
    }
    catch (std::exception& e) {
        std::cerr << "Exception in SaveIniSettings: " << e.what() << std::endl;
        std::ofstream fs("settings.ini");
        fs.close();
    }
}