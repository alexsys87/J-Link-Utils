// Settings are loaded once, then staged in the shared UI preference store.
#include "Preferences.h"
#include <cassert>
#include <cstring>
#include <Main.h>

std::string getDirectoryWithCurrentExecutable()
{
#ifdef _WIN32
    std::wstring buffer(512, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, &buffer[0], static_cast<DWORD>(buffer.size()));
        if (!length) {
            prefs::detail::GetStore().Fail("Cannot locate executable directory");
            return "";
        }
        if (length < buffer.size()) {
            buffer.resize(length);
            return std::filesystem::path(buffer).parent_path().u8string();
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    std::string buffer(512, '\0');
    for (;;) {
        const auto length = ::readlink("/proc/self/exe", &buffer[0], buffer.size());
        if (length < 0) { prefs::detail::GetStore().Fail("Cannot locate executable directory"); return ""; }
        if (static_cast<size_t>(length) < buffer.size()) {
            buffer.resize(static_cast<size_t>(length));
            return std::filesystem::u8path(buffer).parent_path().u8string();
        }
        buffer.resize(buffer.size() * 2);
    }
#endif
}

namespace {
template<size_t N> void CopyString(char (&destination)[N], const std::string& source) {
    const size_t count = (std::min)(N - 1, source.size());
    std::memcpy(destination, source.data(), count);
    destination[count] = '\0';
}
}

void LoadIniSettings(void* arg)
{
    assert(arg != nullptr);
    auto* ctx = static_cast<MainContext*>(arg);
    const auto directory = getDirectoryWithCurrentExecutable();
    // Never fall back to the working directory or auto-connect with an unreadable config.
    if (directory.empty() || !prefs::Load((std::filesystem::u8path(directory) / "settings.ini").u8string())) {
        ctx->auto_connect = false;
        return; // Preserve the load diagnostic and leave the other context fields untouched.
    }
    ctx->Style = prefs::ReadInt("WINDOW.Style", ctx->Style, 0, 23);
    ctx->TopMost = prefs::ReadBool("WINDOW.TopMost", ctx->TopMost);
    ctx->FontSize = prefs::ReadInt("WINDOW.FontSize", ctx->FontSize, 0, 4095);
    ctx->Win_width = prefs::ReadInt("WINDOW.Width", ctx->Win_width, 240, 32768);
    ctx->Win_height = prefs::ReadInt("WINDOW.Height", ctx->Win_height, 120, 32768);
    ctx->Win_x = prefs::ReadInt("WINDOW.X", ctx->Win_x, -32768, 32768);
    ctx->Win_y = prefs::ReadInt("WINDOW.Y", ctx->Win_y, -32768, 32768);
    ctx->Win_position_saved = prefs::ReadBool("WINDOW.HasPosition", ctx->Win_position_saved);
    ctx->Win_maximized = prefs::ReadBool("WINDOW.Maximized", ctx->Win_maximized);
    CopyString(ctx->File_path, prefs::ReadString("File.LastFile", ctx->File_path));
    uint32_t base;
    if (prefs::detail::ParseUInt32(prefs::ReadString("MCU.Base", std::to_string(ctx->mcu_memory_base)), base))
        ctx->mcu_memory_base = base;
    else prefs::detail::Invalid("MCU.Base");
    const auto device = prefs::ReadString("JLink.Device", ctx->jlink_device);
    if (!device.empty()) CopyString(ctx->jlink_device, device);
    ctx->auto_connect = prefs::ReadBool("SETTINGS.AutoConnect", ctx->auto_connect);
    ctx->show_xml_editor = prefs::ReadBool("UI.XML", ctx->show_xml_editor);
    ctx->show_style_editor = prefs::ReadBool("UI.StyleEditor", ctx->show_style_editor);
    ctx->design_mode = prefs::ReadBool("UI.DesignMode", ctx->design_mode);
}

void SaveIniSettings(void* arg)
{
    assert(arg != nullptr);
    const auto* ctx = static_cast<const MainContext*>(arg);
    prefs::WriteInt("WINDOW.Style", ctx->Style);
    prefs::WriteBool("WINDOW.TopMost", ctx->TopMost);
    prefs::WriteInt("WINDOW.FontSize", ctx->FontSize);
    prefs::WriteInt("WINDOW.Width", ctx->Win_width);
    prefs::WriteInt("WINDOW.Height", ctx->Win_height);
    prefs::WriteInt("WINDOW.X", ctx->Win_x);
    prefs::WriteInt("WINDOW.Y", ctx->Win_y);
    prefs::WriteBool("WINDOW.HasPosition", ctx->Win_position_saved);
    prefs::WriteBool("WINDOW.Maximized", ctx->Win_maximized);
    prefs::WriteString("File.LastFile", ctx->File_path);
    std::ostringstream base;
    base << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << ctx->mcu_memory_base;
    prefs::WriteString("MCU.Base", base.str());
    prefs::WriteString("JLink.Device", ctx->jlink_device);
    prefs::WriteBool("SETTINGS.AutoConnect", ctx->auto_connect);
    prefs::WriteBool("UI.XML", ctx->show_xml_editor);
    prefs::WriteBool("UI.StyleEditor", ctx->show_style_editor);
    prefs::WriteBool("UI.DesignMode", ctx->design_mode);
    // The owner flushes periodically and at exit; do not re-read or write on every UI change.
}
