#include "Preferences.h"
#include <Main.h>
#include <cassert>
#include <iostream>
#include <iterator>

namespace fs = std::filesystem;
void WriteFromOtherTranslationUnit();
static void Put(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary); file << text; file.close(); assert(file);
}
static std::string Get(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
}
int main() {
    const auto root = fs::u8path(getDirectoryWithCurrentExecutable());
    const auto path = root / "settings.ini";
    fs::remove(path);
    assert(prefs::Load(path.u8string()));
    assert(!prefs::Dirty() && prefs::Error().empty());
    assert(prefs::ReadInt("absent", 12, 0, 100) == 12);
    assert(!fs::exists(path));
    prefs::WriteString("unknown.a.b", "preserved");
    prefs::WriteString("paths.unicode", u8"C:\\данные\\dashboard.xml");
    const std::string escaped = "  quoted\"\n\\text\r  ";
    prefs::WriteString("strings.escaped", escaped);
    prefs::WriteString("strings.equal", "a=b;#c");
    prefs::WriteString("strings.quote", "\"leading quote");
    prefs::WriteInt("test.int", -12);
    prefs::WriteDouble("test.double", 0.12345678901234567);
    prefs::WriteBool("test.bool", true);
    WriteFromOtherTranslationUnit();
    assert(prefs::ReadString("shared.key") == "one process-wide store");
    assert(prefs::Dirty() && prefs::Flush() && !prefs::Dirty());
    prefs::WriteBool("test.bool", true);
    assert(!prefs::Dirty());
    assert(prefs::Load(path.u8string()));
    assert(prefs::ReadString("strings.escaped") == escaped);
    assert(prefs::ReadString("strings.equal") == "a=b;#c");
    assert(prefs::ReadString("strings.quote") == "\"leading quote");
    assert(prefs::ReadString("paths.unicode") == u8"C:\\данные\\dashboard.xml");
    assert(prefs::ReadString("unknown.a.b") == "preserved");
    assert(prefs::ReadDouble("test.double", 0, -1, 1) == 0.12345678901234567);
    assert(prefs::ReadInt("test.int", 0, -100, 100) == -12);
    assert(prefs::ReadBool("test.bool", false));

    for (const auto& invalid : {"1junk", "99999999999999999999999", "-", "+", "1.5", "", " 1", "1 "}) {
        prefs::WriteString("bad.int", invalid);
        assert(prefs::ReadInt("bad.int", 7, 0, 10) == 7);
        assert(!prefs::Error().empty());
    }
    for (const auto& invalid : {"NaN", "inf", "-inf", "1e309", "1.5junk", "1 ", " 1", "", "11", "-11"}) {
        prefs::WriteString("bad.double", invalid);
        assert(prefs::ReadDouble("bad.double", 0.5, -10, 10) == 0.5);
    }
    prefs::WriteDouble("bad.nonfinite", std::numeric_limits<double>::infinity());
    assert(prefs::ReadString("bad.nonfinite", "missing") == "missing");
    prefs::WriteString("bad.bool", "2");
    assert(prefs::ReadBool("bad.bool", true));
    prefs::WriteString("bad.bool", "false");
    assert(!prefs::ReadBool("bad.bool", true));
    assert(prefs::Flush() && prefs::Error().empty());

    const auto key = prefs::ScopedKey((root / "folder/../dash.xml").u8string(), "watch", "a.b=\n[]");
    assert(key == prefs::ScopedKey((root / "dash.xml").u8string(), "watch", "a.b=\n[]"));
    assert(key != prefs::ScopedKey((root / "other.xml").u8string(), "watch", "a.b=\n[]"));
    assert(key != prefs::ScopedKey((root / "dash.xml").u8string(), "watcha", ".b=\n[]"));
    assert(prefs::ScopedKey("ab", "c", "d") != prefs::ScopedKey("a", "bc", "d"));
    prefs::WriteBool(key, false);
    assert(prefs::Flush() && prefs::Load(path.u8string()));
    assert(!prefs::ReadBool(key, true));
    assert(prefs::ReadBool(prefs::ScopedKey((root / "other.xml").u8string(), "watch", "a.b=\n[]"), true));

    const auto unicodePath = root / fs::u8path(u8"настройки.ini");
    assert(prefs::Load(unicodePath.u8string()));
    prefs::WriteString("Unicode.value", u8"данные");
    assert(prefs::Flush() && prefs::Load(unicodePath.u8string()));
    assert(prefs::ReadString("Unicode.value") == u8"данные");
    fs::remove(unicodePath);

    // Real replacement failure must preserve the old bytes, retain dirty/error and permit retry.
    const auto blocked = root / "blocked";
    fs::create_directory(blocked);
    const auto original = blocked / "settings.ini";
    Put(original, "[Keep]\nvalue=original\n");
    assert(prefs::Load(original.u8string()));
    prefs::WriteString("Keep.value", "changed");
#ifndef _WIN32
    fs::permissions(blocked, fs::perms::owner_read | fs::perms::owner_exec, fs::perm_options::replace);
    assert(!prefs::Flush());
    assert(Get(original) == "[Keep]\nvalue=original\n");
    assert(prefs::Dirty() && !prefs::Error().empty());
    fs::permissions(blocked, fs::perms::owner_all, fs::perm_options::replace);
    assert(prefs::Flush() && !prefs::Dirty() && prefs::Error().empty());
    assert(prefs::Load(original.u8string()));
    assert(prefs::ReadString("Keep.value") == "changed");
    // An unreadable file is not treated as empty and may not be overwritten, even after chmod.
    fs::permissions(original, fs::perms::none, fs::perm_options::replace);
    assert(!prefs::Load(original.u8string()));
    prefs::WriteString("Keep.value", "danger");
    fs::permissions(original, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
    assert(!prefs::Flush());
    assert(Get(original).find("changed") != std::string::npos);
    assert(prefs::Load(original.u8string()));
#endif
    // A destination replaced with a directory exercises failure AFTER temporary file write.
    const auto backup = blocked / "backup.ini";
    fs::rename(original, backup);
    fs::create_directory(original);
    prefs::WriteString("Keep.value", "retry");
    assert(!prefs::Flush() && prefs::Dirty() && !prefs::Error().empty());
    assert(Get(backup).find("changed") != std::string::npos);
    for (const auto& entry : fs::directory_iterator(blocked)) assert(entry.path().filename().u8string().find(".tmp.") == std::string::npos);
    fs::remove(original);
    fs::rename(backup, original);
    assert(prefs::Flush() && prefs::Error().empty());
    assert(prefs::Load(blocked.u8string()) == false);
    prefs::WriteInt("unsafe", 1);
    assert(!prefs::Flush());

    // Legacy nine keys plus newly persisted window geometry/editor toggles.
    Put(path, "[WINDOW]\nStyle=23\nTopMost=1\nFontSize=9\nWidth=1600\nHeight=900\n"
        "X=-1920\nY=123\nHasPosition=1\nMaximized=1\n[File]\nLastFile=C:\\dash.xml\n"
        "[MCU]\nBase=0xFFFFFFFF\n[JLink]\nDevice=STM32F4\n[SETTINGS]\nAutoConnect=0\n"
        "[UI]\nXML=1\nStyleEditor=1\nDesignMode=1\n[Future]\nkey=do not discard\nquoted=\"keep quotes\"\n");
    MainContext ctx;
    ctx.write_permission = true; ctx.armed = true; ctx.connected = true;
    LoadIniSettings(&ctx);
    assert(ctx.Style == 23 && ctx.TopMost && ctx.FontSize == 9);
    assert(ctx.Win_width == 1600 && ctx.Win_height == 900);
    assert(ctx.Win_x == -1920 && ctx.Win_y == 123 && ctx.Win_position_saved && ctx.Win_maximized);
    assert(std::string(ctx.File_path) == "C:\\dash.xml" && std::string(ctx.jlink_device) == "STM32F4");
    assert(ctx.mcu_memory_base == UINT32_MAX && !ctx.auto_connect);
    assert(ctx.show_xml_editor && ctx.show_style_editor && ctx.design_mode);
    SaveIniSettings(&ctx);
    assert(prefs::Flush());
    MainContext restored;
    LoadIniSettings(&restored);
    assert(restored.Style == 23 && restored.TopMost && restored.FontSize == 9);
    assert(restored.Win_width == 1600 && restored.Win_height == 900);
    assert(restored.Win_x == -1920 && restored.Win_y == 123 && restored.Win_position_saved && restored.Win_maximized);
    assert(restored.show_xml_editor && restored.show_style_editor && restored.design_mode);
    assert(restored.mcu_memory_base == UINT32_MAX && !restored.auto_connect);
    assert(std::string(restored.File_path) == "C:\\dash.xml" && std::string(restored.jlink_device) == "STM32F4");
    assert(prefs::ReadString("Future.key") == "do not discard");
    assert(Get(path).find("quoted=\"keep quotes\"") != std::string::npos);
    assert(ctx.write_permission && ctx.armed && ctx.connected);
    assert(Get(path).find("permission") == std::string::npos && Get(path).find("armed") == std::string::npos);
    SaveIniSettings(&restored);
    assert(!prefs::Dirty());

    for (const auto& invalidBase : {"-1", "4294967296", "0x100000000", "0x", "0x20junk", "123x", "+32"}) {
        Put(path, std::string("[MCU]\nBase=") + invalidBase + "\n");
        MainContext defaults; LoadIniSettings(&defaults);
        assert(defaults.mcu_memory_base == 0x20000000 && !prefs::Error().empty());
    }
    Put(path, "[MCU]\nBase=010\n[WINDOW]\nStyle=24\nFontSize=-1\nWidth=239\nHeight=119\nTopMost=garbage\n");
    MainContext defaults; LoadIniSettings(&defaults);
    assert(defaults.mcu_memory_base == 10); // Decimal, never accidental octal.
    assert(defaults.Style == 3 && defaults.FontSize == 2 && defaults.Win_width == 1360 && defaults.Win_height == 780 && !defaults.TopMost);
    Put(path, "[WINDOW]\nStyle=2junk\nFontSize=4096\nWidth=32769\nHeight=2147483648\n");
    LoadIniSettings(&defaults);
    assert(defaults.Style == 3 && defaults.FontSize == 2 && defaults.Win_width == 1360 && defaults.Win_height == 780);
    Put(path, "\xEF\xBB\xBF[WINDOW]\r\nWidth=240\r\nHeight=120\r\nStyle=0\r\nFontSize=4095\r\n");
    LoadIniSettings(&defaults);
    assert(defaults.Win_width == 240 && defaults.Win_height == 120 && defaults.Style == 0 && defaults.FontSize == 4095);
    Put(path, "not an ini entry\n[Good]\nkey=value\n");
    assert(prefs::Load(path.u8string()) && !prefs::Error().empty());
    assert(prefs::ReadString("Good.key") == "value");
    fs::remove_all(blocked);
    fs::remove(path);
    std::cout << "Settings persistence tests passed\n";
}
