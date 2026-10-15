#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <new>
#include <chrono>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#include <io.h>
#define ACCESS _access
const char* defaultFontDir = "C:\\Windows\\Fonts\\";
#else
#include <unistd.h>
#define ACCESS access
const char* defaultFontDir = "/usr/share/fonts/"; // Base font path on Linux
#endif

#include <Main.h>
#include <misc/freetype/imgui_freetype.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#define NOC_FILE_DIALOG_WIN32
#include "noc_file_dialog.h"

#include <imgui_color.h>

#include "XMLEditor.h"
#include "DesignMode.h"
#include "widget_common.h"
#include "res/app_icon.h"
#include "MainWindowChrome.h"
#include "UserGuide.h"
#include "Preferences.h"
#include "Localization.h"
#include "LocalizedDiagnostics.h"

static MainWindowChrome g_MainWindowChrome;

static std::string ApplicationFile(const char* name)
{
    return (std::filesystem::u8path(getDirectoryWithCurrentExecutable()) / name).u8string();
}

// Only capture restored geometry. Minimize/maximize must not replace the normal
// rectangle that will be used on the next startup or when restoring the window.
static void CaptureMainWindowSettings(MainContext* ctx)
{
    if (!ctx->Wimdow) return;
    const Uint32 flags = SDL_GetWindowFlags(ctx->Wimdow);
    if (flags & SDL_WINDOW_MINIMIZED) return;
    ctx->Win_maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
    if (!ctx->Win_maximized) {
        SDL_GetWindowPosition(ctx->Wimdow, &ctx->Win_x, &ctx->Win_y);
        SDL_GetWindowSize(ctx->Wimdow, &ctx->Win_width, &ctx->Win_height);
        ctx->Win_position_saved = true;
    }
}

static void FitMainWindowToDisplay(MainContext* ctx)
{
    SDL_Rect bounds{};
    int displayIndex = 0;
    if (ctx->Win_position_saved) {
        SDL_Rect saved{ctx->Win_x, ctx->Win_y, ctx->Win_width, ctx->Win_height};
        displayIndex = SDL_GetRectDisplayIndex(&saved);
        if (displayIndex < 0) displayIndex = 0;
    }
    if (SDL_GetDisplayUsableBounds(displayIndex, &bounds) != 0) return;
    ctx->Win_width = std::min(ctx->Win_width, std::max(bounds.w, 1));
    ctx->Win_height = std::min(ctx->Win_height, std::max(bounds.h, 1));
    if (ctx->Win_position_saved) {
        // A monitor may have been disconnected since the last run.
        ctx->Win_x = std::clamp(ctx->Win_x, bounds.x, bounds.x + std::max(0, bounds.w - ctx->Win_width));
        ctx->Win_y = std::clamp(ctx->Win_y, bounds.y, bounds.y + std::max(0, bounds.h - ctx->Win_height));
    }
}

#ifdef _WIN32
// The supplied JLink.lib/JLink_x64.lib expose a cdecl, explicit DLL loader.
// Its return value is 0 on success, -1 on failure. The SDK API declarations
// intentionally have no dllimport attribute; calls go through these wrappers.
extern "C" int __cdecl JLINKARMDLL_LIB_LoadLibrary(void);
#endif

// ------------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------------

size_t getFileNameFromPath(const char* path)
{
    if (!path || !*path)
        return 0;
    size_t len = SDL_strlen(path);
    for (size_t i = len; i-- > 0; )
    {
        if (path[i] == '\\' || path[i] == '/')
            return i + 1;
    }
    return 0;
}

// ------------------------------------------------------------------
// Context initialization and management
// ------------------------------------------------------------------

MainContext* allocate_context() {
    MainContext* ctx = new (std::nothrow) MainContext{};
    if (ctx == NULL) {
        fprintf(stderr, "Couldn't allocate memory");
        return NULL;
    }
    ctx->Win_width = WINDOW_WIDTH;
    ctx->Win_height = WINDOW_HEIGHT;
    ctx->FontSize = 2;
    ctx->TopMost = false;
    ctx->Style = 1;
    ctx->Datalock = SDL_CreateMutex();
    ctx->Datacond = SDL_CreateCond();
    if (!ctx->Datalock || !ctx->Datacond) {
        if (ctx->Datalock) SDL_DestroyMutex(ctx->Datalock);
        if (ctx->Datacond) SDL_DestroyCond(ctx->Datacond);
        delete ctx;
        return nullptr;
    }
    ctx->IsClosing = false;
    ctx->StartComunication = false;
    ctx->Sync = 0;
    ctx->file_loaded = false;
    ctx->ErrRengerer = false;
    ctx->xml_modified = false;
	ctx->closefile = false;
	ctx->openfile = false;
	ctx->show_style_editor = false;
    ctx->show_xml_editor = false;
    ctx->xml_revision = 0;
    ctx->design_mode = false;
    ctx->CurrentXMLDoc = nullptr;
    ctx->jlink_opened = false;
    ctx->StateMinimized = false;
    ctx->mcu_memory_base = 0x20000000;
    ctx->auto_connect = true;   // auto-connect is enabled by default

    // Allocate 1 MiB (1048576 bytes) and check the result
    ctx->IOMAP_Data = (uint8_t*)SDL_malloc(IOMAP_SIZE);
    if (!ctx->IOMAP_Data) {
        fprintf(stderr, "Failed to allocate 1MB IOMAP memory\n");
        if (ctx->Datacond) SDL_DestroyCond(ctx->Datacond);
        if (ctx->Datalock) SDL_DestroyMutex(ctx->Datalock);
        delete ctx;
        return NULL;
    }
    SDL_memset(ctx->IOMAP_Data, 0, 1024 * 1024);

    ClearAllWindows();

    SDL_memset(ctx->File_path, 0, sizeof(ctx->File_path));
    SDL_memset(ctx->File_label, 0, sizeof(ctx->File_label));
    // The main window title must never be empty: ImGui asserts on an empty window name.
    SDL_strlcpy(ctx->File_label, "J-Link Utils", sizeof(ctx->File_label));
    SDL_memset(ctx->jlink_device, 0, sizeof(ctx->jlink_device));
    strcpy(ctx->jlink_device, "Cortex-M0");

    if (lfqueue_init(&ctx->tx_Queue) != 0) {
        SDL_free(ctx->IOMAP_Data);
        SDL_DestroyCond(ctx->Datacond);
        SDL_DestroyMutex(ctx->Datalock);
        delete ctx;
        return nullptr;
    }

    return ctx;
}

void free_context(MainContext* ctx)
{
    if (!ctx) return;
    StopCommunication(ctx);
    ResetXMLEditorState();
    DesignMode::Reset();
    ClearAllWindows();
    if (ctx->CurrentXMLDoc) delete ctx->CurrentXMLDoc;

    lfqueue_destroy(&ctx->tx_Queue);

    SDL_DestroyCond(ctx->Datacond);
    SDL_DestroyMutex(ctx->Datalock);

    SDL_free(ctx->IOMAP_Data);
    delete ctx;
}

// ------------------------------------------------------------------
// Application icon (res/app_icon.h, made by res/make_icon.py)
// ------------------------------------------------------------------

// The executable carries the same icon as a resource (app.rc) for Explorer and the taskbar;
// this sets it on the window itself.
static void SetAppIcon(SDL_Window* window)
{
    SDL_Surface* icon = SDL_CreateRGBSurfaceWithFormatFrom((void*)kAppIconPixels, kAppIconSize, kAppIconSize,
        32, kAppIconSize * 4, SDL_PIXELFORMAT_RGBA32);
    if (!icon) return;
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
}

// Texture of the icon for the About window
static SDL_Texture* g_AppIconTexture = nullptr;

static SDL_Texture* CreateAppIconTexture(SDL_Renderer* renderer)
{
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, kAppIconSize, kAppIconSize);
    if (!texture) return nullptr;
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, nullptr, kAppIconPixels, kAppIconSize * 4);
    return texture;
}

// ------------------------------------------------------------------
// Open/save file dialogs
// ------------------------------------------------------------------

// Not inline: WatchWidget.cpp calls these through the declarations in widget_common.h.
const char* file_open_dialog(const char* filters)
{
    return (noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, filters, NULL, NULL));
}

const char* file_save_dialog(const char* filters)
{
    return (noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, filters, NULL, NULL));
}

bool NewFile(MainContext* ctx) {
    const char* s = file_save_dialog("XML(*.xml)\0*.XML\0");
    if (!s) return false; // the user cancelled the dialog

    // Close the current document, if any
    ResetXMLEditorState();
    DesignMode::Reset();
    if (ctx->CurrentXMLDoc) {
        delete ctx->CurrentXMLDoc;
        ctx->CurrentXMLDoc = nullptr;
    }

    // Clear the windows and reset the state
    ClearAllWindows();;
    ctx->file_loaded = false;
    ctx->xml_modified = false;
    ctx->show_xml_editor = true;

    // Copy the new path
    SDL_strlcpy(ctx->File_path, s, MAX_PATH);

    // Append .xml if needed
    EnsureXmlExtension(ctx->File_path, sizeof(ctx->File_path));

    // Update the main window title
    SDL_snprintf(ctx->File_label, sizeof(ctx->File_label), "J-Link Utils - %s", &ctx->File_path[getFileNameFromPath(ctx->File_path)]);

    // Save the path to the INI file
    SaveIniSettings(ctx);

    // Create a new empty XML document
    ctx->CurrentXMLDoc = new (std::nothrow) tinyxml2::XMLDocument();
    if (!ctx->CurrentXMLDoc) {
        // Out of memory; a message could be shown here
        return false;
    }

    return true;
}

bool OpenFile(MainContext* ctx)
{
    const char* s = file_open_dialog("XML(*.xml)\0*.XML\0");
    if (!s) return false; // the user cancelled the dialog

    ctx->File_path[0] = '\0';

    ctx->show_xml_editor = false;
    ctx->show_style_editor = false;

    ResetXMLEditorState();
    DesignMode::Reset();
    if (ctx->CurrentXMLDoc) {
        delete ctx->CurrentXMLDoc;
        ctx->CurrentXMLDoc = nullptr;
    }

    ctx->file_loaded = false;
    ctx->xml_modified = false;

    ClearAllWindows();

    // Copy the new path
    SDL_strlcpy(ctx->File_path, s, MAX_PATH);

    // Update the main window title
    SDL_snprintf(ctx->File_label, sizeof(ctx->File_label), "J-Link Utils - %s", &ctx->File_path[getFileNameFromPath(ctx->File_path)]);

    // Save the path to the INI file
    SaveIniSettings(ctx);

    // Load the XML document for the editor
    if (ctx->CurrentXMLDoc) delete ctx->CurrentXMLDoc;
    ctx->CurrentXMLDoc = new tinyxml2::XMLDocument();
    if (ctx->CurrentXMLDoc->LoadFile(s) != tinyxml2::XML_SUCCESS) {
        // Load error; a popup could be shown here
        delete ctx->CurrentXMLDoc;
        ctx->CurrentXMLDoc = nullptr;
    }

    return true;
}

bool CloseFile(MainContext* ctx)
{
    ctx->File_path[0] = '\0';

    ResetXMLEditorState();
    DesignMode::Reset();
    if (ctx->CurrentXMLDoc) {
        delete ctx->CurrentXMLDoc;
        ctx->CurrentXMLDoc = nullptr;
    }

    SaveIniSettings(ctx);
    sprintf(ctx->File_label, "J-Link Utils");
    ctx->file_loaded = false;
    ClearAllWindows();
    return true;
}

// ------------------------------------------------------------------
// DPI-independent UI coordinates
// ------------------------------------------------------------------

void ConfigureDpiAwareness() {
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
#ifdef _WIN32
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    // SDL sizes, mouse events and display bounds use 96-DPI logical points.
    // SDL also resizes the native window when its monitor DPI changes. This
    // scales ALL geometry, including explicit XML sizes and custom draw lists.
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    // Keep native taskbar/system-menu/minimize behavior without drawing an OS frame.
    SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
#endif
}

void PrepareUiFrame(SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    // The SDL backend supplies logical DisplaySize and physical/logical
    // DisplayFramebufferScale. The renderer scales geometry ONCE, and ImGui
    // 1.92 uses that density to rasterize fonts sharply at the physical size.
    // FontScaleDpi or ScaleAllSizes here would apply DPI a second time.
    ImGui::GetStyle().FontScaleDpi = 1.0f;
    ImGuiIO& io = ImGui::GetIO();
    // The SDL renderer backend projects clip rectangles, but passes vertices
    // through unchanged. SDL must scale both geometry and clipping together.
    // It detects this renderer scale and avoids multiplying clips twice.
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    if (io.FontDefault)
        ImGui::GetStyle().FontSizeBase = io.FontDefault->LegacySize;
    ImGui::NewFrame();
}

// ------------------------------------------------------------------
// Loading system fonts (C4244 warning fixed)
// ------------------------------------------------------------------

void LoadFonts(MainContext* ctx) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    // FreeType is already linked by the project, but ImGui otherwise defaults
    // to stb_truetype. Native, full hinting keeps small text crisp while retaining
    // grayscale antialiasing (no fuzzy light hinting or jagged monochrome mode).
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    io.Fonts->FontLoaderFlags = 0;

    // Font files (Windows and Linux equivalents)
    std::vector<std::string> fontFiles = {
        "consola.ttf", "arial.ttf", "verdana.ttf", // Windows
        "truetype/dejavu/DejaVuSansMono.ttf",      // Ubuntu/Debian
        "TTF/DejaVuSansMono.ttf",                  // Arch/Fedora
        "liberation/LiberationSans-Regular.ttf"    // Common Linux
    };

    const int minSize = 11;
    const int maxSize = 20;

    for (const auto& fileName : fontFiles) {
        std::string fullPath = std::string(defaultFontDir) + fileName;

        // Check that the file exists (mode 0 = existence check)
        if (ACCESS(fullPath.c_str(), 0) != 0) continue;

        for (int size = minSize; size <= maxSize; ++size) {
            // Keep logical sizes stable. ImGui 1.92 bakes glyphs at the current
            // FramebufferScale, rather than stretching old bitmaps.
            float fontSize = static_cast<float>(size);
            ImFontConfig config;

            // Name without the extension
            size_t lastDot = fileName.find_last_of(".");
            std::string displayName = (lastDot != std::string::npos) ? fileName.substr(0, lastDot) : fileName;
            snprintf(config.Name, sizeof(config.Name), "%s, %dpx", displayName.c_str(), size);

            io.Fonts->AddFontFromFileTTF(
                fullPath.c_str(),
                fontSize,
                &config,
                io.Fonts->GetGlyphRangesCyrillic()
            );
        }
    }

    if (io.Fonts->Fonts.empty()) {
        // The bitmap default cannot stay crisp at arbitrary DPI scales.
        io.Fonts->AddFontDefaultVector();
    }

    int fontIndex = ctx->FontSize;
    if (fontIndex < 0 || fontIndex >= io.Fonts->Fonts.Size) {
        // The installed font list may differ from the machine that saved INI.
        fontIndex = std::min(2, io.Fonts->Fonts.Size - 1);
        ctx->FontSize = fontIndex;
        prefs::WriteInt("WINDOW.FontSize", fontIndex);
    }
    io.FontDefault = io.Fonts->Fonts[fontIndex];
}

// ------------------------------------------------------------------
// Menu functions (File, Window, Help, Style)
// ------------------------------------------------------------------

int FontSelector(const char* label)
{
    int sel = -1;
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_current = ImGui::GetFont();
    if (ImGui::BeginCombo(label, font_current->GetDebugName()))
    {
        for (int n = 0; n < io.Fonts->Fonts.Size; n++)
        {
            ImFont* font = io.Fonts->Fonts[n];
            ImGui::PushID((void*)font);
            if (ImGui::Selectable(font->GetDebugName(), font == font_current))
            {
                io.FontDefault = font; sel = n;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    return sel;
}

void ShowMenuFile(MainContext* ctx) {

    if (ImGui::MenuItem(app_language::Label("New", "Создать"), "Ctrl+N")) {
        NewFile(ctx);
    }
    if (ImGui::MenuItem(app_language::Label("Open", "Открыть"), "Ctrl+O")) {
        OpenFile(ctx);
    }
    if (ImGui::MenuItem(app_language::Label("Close", "Закрыть"), "Ctrl+Q")) {
        CloseFile(ctx);
    }
    ImGui::Separator();
    if (ImGui::MenuItem(app_language::Label("Quit", "Выход"), "Alt+F4")) {
        ctx->IsClosing = true;
    }
}

// ------------------------------------------------------------------
// Helpers to save/load the style as XML
// ------------------------------------------------------------------
// tinyxml2's filename overload uses the Windows ANSI code page, whereas
// ApplicationFile returns UTF-8. Open explicitly so non-ASCII EXE folders work.
static FILE* OpenStyleFile(const char* filename, const char* mode)
{
    try {
#ifdef _WIN32
        const auto path = std::filesystem::u8path(filename).native();
        const std::wstring wideMode(mode, mode + strlen(mode));
        FILE* file = nullptr;
        return _wfopen_s(&file, path.c_str(), wideMode.c_str()) == 0 ? file : nullptr;
#else
        return fopen(filename, mode);
#endif
    } catch (const std::exception&) {
        return nullptr;
    }
}

bool SaveStyleToXML(const char* filename, const ImGuiStyle& style)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* root = doc.NewElement("Style");
    doc.InsertFirstChild(root);

    // Save scalar parameters (float, int, ImVec2)
    auto addFloat = [&](const char* name, float value) {
        tinyxml2::XMLElement* elem = doc.NewElement(name);
        elem->SetAttribute("value", value);
        root->InsertEndChild(elem);
        };
    auto addInt = [&](const char* name, int value) {
        tinyxml2::XMLElement* elem = doc.NewElement(name);
        elem->SetAttribute("value", value);
        root->InsertEndChild(elem);
        };
    auto addVec2 = [&](const char* name, const ImVec2& v) {
        tinyxml2::XMLElement* elem = doc.NewElement(name);
        elem->SetAttribute("x", v.x);
        elem->SetAttribute("y", v.y);
        root->InsertEndChild(elem);
        };

    addVec2("WindowPadding", style.WindowPadding);
    addVec2("FramePadding", style.FramePadding);
    addVec2("CellPadding", style.CellPadding);
    addVec2("ItemSpacing", style.ItemSpacing);
    addVec2("ItemInnerSpacing", style.ItemInnerSpacing);
    addVec2("TouchExtraPadding", style.TouchExtraPadding);
    addFloat("IndentSpacing", style.IndentSpacing);
    addFloat("ScrollbarSize", style.ScrollbarSize);
    addFloat("GrabMinSize", style.GrabMinSize);
    addFloat("WindowBorderSize", style.WindowBorderSize);
    addFloat("ChildBorderSize", style.ChildBorderSize);
    addFloat("PopupBorderSize", style.PopupBorderSize);
    addFloat("FrameBorderSize", style.FrameBorderSize);
    addFloat("TabBorderSize", style.TabBorderSize);
    addFloat("WindowRounding", style.WindowRounding);
    addFloat("ChildRounding", style.ChildRounding);
    addFloat("FrameRounding", style.FrameRounding);
    addFloat("PopupRounding", style.PopupRounding);
    addFloat("ScrollbarRounding", style.ScrollbarRounding);
    addFloat("GrabRounding", style.GrabRounding);
    addFloat("TabRounding", style.TabRounding);
    addVec2("WindowTitleAlign", style.WindowTitleAlign);
    addInt("WindowMenuButtonPosition", style.WindowMenuButtonPosition); // ImGuiDir converts to int implicitly
    addVec2("ButtonTextAlign", style.ButtonTextAlign);
    addVec2("SelectableTextAlign", style.SelectableTextAlign);
    addVec2("DisplaySafeAreaPadding", style.DisplaySafeAreaPadding);

    // Save colors
    tinyxml2::XMLElement* colorsElem = doc.NewElement("Colors");
    root->InsertEndChild(colorsElem);
    for (int i = 0; i < ImGuiCol_COUNT; i++)
    {
        tinyxml2::XMLElement* colorElem = doc.NewElement("Color");
        colorElem->SetAttribute("name", ImGui::GetStyleColorName(i));
        colorElem->SetAttribute("r", style.Colors[i].x);
        colorElem->SetAttribute("g", style.Colors[i].y);
        colorElem->SetAttribute("b", style.Colors[i].z);
        colorElem->SetAttribute("a", style.Colors[i].w);
        colorsElem->InsertEndChild(colorElem);
    }

    FILE* file = OpenStyleFile(filename, "wb");
    if (!file) return false;
    const bool saved = doc.SaveFile(file) == tinyxml2::XML_SUCCESS;
    const bool closed = fclose(file) == 0;
    return saved && closed;
}

bool LoadStyleFromXML(const char* filename, ImGuiStyle& style)
{
    tinyxml2::XMLDocument doc;
    FILE* file = OpenStyleFile(filename, "rb");
    if (!file) return false;
    const auto result = doc.LoadFile(file);
    const bool closed = fclose(file) == 0;
    if (result != tinyxml2::XML_SUCCESS || !closed)
        return false;

    tinyxml2::XMLElement* root = doc.FirstChildElement("Style");
    if (!root) return false;

    // Load scalar parameters
    auto getFloat = [&](const char* name, float* out) -> bool {
        tinyxml2::XMLElement* elem = root->FirstChildElement(name);
        if (elem && elem->QueryAttribute("value", out) == tinyxml2::XML_SUCCESS)
            return true;
        return false;
        };
    auto getInt = [&](const char* name, int* out) -> bool {
        tinyxml2::XMLElement* elem = root->FirstChildElement(name);
        if (elem && elem->QueryAttribute("value", out) == tinyxml2::XML_SUCCESS)
            return true;
        return false;
        };
    auto getVec2 = [&](const char* name, ImVec2* out) -> bool {
        tinyxml2::XMLElement* elem = root->FirstChildElement(name);
        if (elem && elem->QueryAttribute("x", &out->x) == tinyxml2::XML_SUCCESS &&
            elem->QueryAttribute("y", &out->y) == tinyxml2::XML_SUCCESS)
            return true;
        return false;
        };

    getVec2("WindowPadding", &style.WindowPadding);
    getVec2("FramePadding", &style.FramePadding);
    getVec2("CellPadding", &style.CellPadding);
    getVec2("ItemSpacing", &style.ItemSpacing);
    getVec2("ItemInnerSpacing", &style.ItemInnerSpacing);
    getVec2("TouchExtraPadding", &style.TouchExtraPadding);
    getFloat("IndentSpacing", &style.IndentSpacing);
    getFloat("ScrollbarSize", &style.ScrollbarSize);
    getFloat("GrabMinSize", &style.GrabMinSize);
    getFloat("WindowBorderSize", &style.WindowBorderSize);
    getFloat("ChildBorderSize", &style.ChildBorderSize);
    getFloat("PopupBorderSize", &style.PopupBorderSize);
    getFloat("FrameBorderSize", &style.FrameBorderSize);
    getFloat("TabBorderSize", &style.TabBorderSize);
    getFloat("WindowRounding", &style.WindowRounding);
    getFloat("ChildRounding", &style.ChildRounding);
    getFloat("FrameRounding", &style.FrameRounding);
    getFloat("PopupRounding", &style.PopupRounding);
    getFloat("ScrollbarRounding", &style.ScrollbarRounding);
    getFloat("GrabRounding", &style.GrabRounding);
    getFloat("TabRounding", &style.TabRounding);
    getVec2("WindowTitleAlign", &style.WindowTitleAlign);
    getInt("WindowMenuButtonPosition", (int*)&style.WindowMenuButtonPosition);
    getVec2("ButtonTextAlign", &style.ButtonTextAlign);
    getVec2("SelectableTextAlign", &style.SelectableTextAlign);
    getVec2("DisplaySafeAreaPadding", &style.DisplaySafeAreaPadding);

    // Load colors
    tinyxml2::XMLElement* colorsElem = root->FirstChildElement("Colors");
    if (colorsElem)
    {
        for (tinyxml2::XMLElement* colorElem = colorsElem->FirstChildElement("Color");
            colorElem; colorElem = colorElem->NextSiblingElement("Color"))
        {
            const char* name = colorElem->Attribute("name");
            if (!name) continue;
            // Find the color index by name
            int idx = -1;
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                if (strcmp(name, ImGui::GetStyleColorName(i)) == 0)
                {
                    idx = i;
                    break;
                }
            }
            if (idx == -1) continue;

            float r, g, b, a;
            if (colorElem->QueryAttribute("r", &r) == tinyxml2::XML_SUCCESS &&
                colorElem->QueryAttribute("g", &g) == tinyxml2::XML_SUCCESS &&
                colorElem->QueryAttribute("b", &b) == tinyxml2::XML_SUCCESS &&
                colorElem->QueryAttribute("a", &a) == tinyxml2::XML_SUCCESS)
            {
                style.Colors[idx] = ImVec4(r, g, b, a);
            }
        }
    }

    return true;
}

// ------------------------------------------------------------------
// Style editor
// ------------------------------------------------------------------
// Style paths are UTF-8 (OpenStyleFile converts them back to UTF-16 on Windows).
// The legacy noc dialog uses GetOpenFileNameA: passing translated UTF-8 filters
// to it corrupts Russian captions on ANSI code pages. Keep document/widget
// dialogs and their existing path encoding untouched; use Unicode for styles.
static const char* OpenStyleFileDialog(int flags, const char* filters, const char* defaultName)
{
#ifdef _WIN32
    static std::string result;
    result.clear();
    const char* end = filters;
    while (*end) end += strlen(end) + 1;
    const int length = static_cast<int>(end - filters + 1); // Includes the double NUL.
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filters, length, nullptr, 0);
    if (!count) return nullptr;
    std::wstring wideFilters(count, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filters, length, &wideFilters[0], count))
        return nullptr;
    wchar_t filename[32768] = {};
    if (defaultName && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, defaultName, -1,
        filename, static_cast<int>(sizeof(filename) / sizeof(filename[0]))))
        return nullptr;
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFile = filename;
    dialog.nMaxFile = static_cast<DWORD>(sizeof(filename) / sizeof(filename[0]));
    dialog.lpstrFilter = wideFilters.c_str();
    dialog.nFilterIndex = 1;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
        ((flags & NOC_FILE_DIALOG_OPEN) ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT);
    const BOOL accepted = (flags & NOC_FILE_DIALOG_OPEN)
        ? GetOpenFileNameW(&dialog) : GetSaveFileNameW(&dialog);
    if (!accepted) return nullptr;
    result = std::filesystem::path(filename).u8string();
    return result.c_str();
#else
    return noc_file_dialog_open(flags, filters, nullptr, defaultName);
#endif
}

// Localized display names only: XML serialization continues to use ImGui's keys.
static const char* StyleColorDisplayName(int color)
{
    switch (color) {
    case ImGuiCol_Text: return app_language::Tr("Text", "Текст");
    case ImGuiCol_TextDisabled: return app_language::Tr("TextDisabled", "Неактивный текст");
    case ImGuiCol_WindowBg: return app_language::Tr("WindowBg", "Фон окна");
    case ImGuiCol_ChildBg: return app_language::Tr("ChildBg", "Фон дочернего окна");
    case ImGuiCol_PopupBg: return app_language::Tr("PopupBg", "Фон всплывающего окна");
    case ImGuiCol_Border: return app_language::Tr("Border", "Граница");
    case ImGuiCol_BorderShadow: return app_language::Tr("BorderShadow", "Тень границы");
    case ImGuiCol_FrameBg: return app_language::Tr("FrameBg", "Фон рамки");
    case ImGuiCol_FrameBgHovered: return app_language::Tr("FrameBgHovered", "Фон рамки при наведении");
    case ImGuiCol_FrameBgActive: return app_language::Tr("FrameBgActive", "Фон активной рамки");
    case ImGuiCol_TitleBg: return app_language::Tr("TitleBg", "Фон заголовка");
    case ImGuiCol_TitleBgActive: return app_language::Tr("TitleBgActive", "Фон активного заголовка");
    case ImGuiCol_TitleBgCollapsed: return app_language::Tr("TitleBgCollapsed", "Фон свёрнутого заголовка");
    case ImGuiCol_MenuBarBg: return app_language::Tr("MenuBarBg", "Фон строки меню");
    case ImGuiCol_ScrollbarBg: return app_language::Tr("ScrollbarBg", "Фон полосы прокрутки");
    case ImGuiCol_ScrollbarGrab: return app_language::Tr("ScrollbarGrab", "Ползунок прокрутки");
    case ImGuiCol_ScrollbarGrabHovered: return app_language::Tr("ScrollbarGrabHovered", "Ползунок прокрутки при наведении");
    case ImGuiCol_ScrollbarGrabActive: return app_language::Tr("ScrollbarGrabActive", "Активный ползунок прокрутки");
    case ImGuiCol_CheckMark: return app_language::Tr("CheckMark", "Отметка флажка");
    case ImGuiCol_SliderGrab: return app_language::Tr("SliderGrab", "Ползунок регулятора");
    case ImGuiCol_SliderGrabActive: return app_language::Tr("SliderGrabActive", "Активный ползунок регулятора");
    case ImGuiCol_Button: return app_language::Tr("Button", "Кнопка");
    case ImGuiCol_ButtonHovered: return app_language::Tr("ButtonHovered", "Кнопка при наведении");
    case ImGuiCol_ButtonActive: return app_language::Tr("ButtonActive", "Активная кнопка");
    case ImGuiCol_Header: return app_language::Tr("Header", "Заголовок элемента");
    case ImGuiCol_HeaderHovered: return app_language::Tr("HeaderHovered", "Заголовок элемента при наведении");
    case ImGuiCol_HeaderActive: return app_language::Tr("HeaderActive", "Активный заголовок элемента");
    case ImGuiCol_Separator: return app_language::Tr("Separator", "Разделитель");
    case ImGuiCol_SeparatorHovered: return app_language::Tr("SeparatorHovered", "Разделитель при наведении");
    case ImGuiCol_SeparatorActive: return app_language::Tr("SeparatorActive", "Активный разделитель");
    case ImGuiCol_ResizeGrip: return app_language::Tr("ResizeGrip", "Маркер изменения размера");
    case ImGuiCol_ResizeGripHovered: return app_language::Tr("ResizeGripHovered", "Маркер размера при наведении");
    case ImGuiCol_ResizeGripActive: return app_language::Tr("ResizeGripActive", "Активный маркер размера");
    case ImGuiCol_InputTextCursor: return app_language::Tr("InputTextCursor", "Курсор текстового ввода");
    case ImGuiCol_TabHovered: return app_language::Tr("TabHovered", "Вкладка при наведении");
    case ImGuiCol_Tab: return app_language::Tr("Tab", "Вкладка");
    case ImGuiCol_TabSelected: return app_language::Tr("TabSelected", "Выбранная вкладка");
    case ImGuiCol_TabSelectedOverline: return app_language::Tr("TabSelectedOverline", "Верхняя линия выбранной вкладки");
    case ImGuiCol_TabDimmed: return app_language::Tr("TabDimmed", "Вкладка без фокуса");
    case ImGuiCol_TabDimmedSelected: return app_language::Tr("TabDimmedSelected", "Выбранная вкладка без фокуса");
    case ImGuiCol_TabDimmedSelectedOverline: return app_language::Tr("TabDimmedSelectedOverline", "Верхняя линия выбранной вкладки без фокуса");
    case ImGuiCol_PlotLines: return app_language::Tr("PlotLines", "Линии графика");
    case ImGuiCol_PlotLinesHovered: return app_language::Tr("PlotLinesHovered", "Линии графика при наведении");
    case ImGuiCol_PlotHistogram: return app_language::Tr("PlotHistogram", "Гистограмма");
    case ImGuiCol_PlotHistogramHovered: return app_language::Tr("PlotHistogramHovered", "Гистограмма при наведении");
    case ImGuiCol_TableHeaderBg: return app_language::Tr("TableHeaderBg", "Фон заголовка таблицы");
    case ImGuiCol_TableBorderStrong: return app_language::Tr("TableBorderStrong", "Основные границы таблицы");
    case ImGuiCol_TableBorderLight: return app_language::Tr("TableBorderLight", "Внутренние границы таблицы");
    case ImGuiCol_TableRowBg: return app_language::Tr("TableRowBg", "Фон строки таблицы");
    case ImGuiCol_TableRowBgAlt: return app_language::Tr("TableRowBgAlt", "Фон чередующихся строк таблицы");
    case ImGuiCol_TextLink: return app_language::Tr("TextLink", "Текст ссылки");
    case ImGuiCol_TextSelectedBg: return app_language::Tr("TextSelectedBg", "Фон выделенного текста");
    case ImGuiCol_TreeLines: return app_language::Tr("TreeLines", "Линии дерева");
    case ImGuiCol_DragDropTarget: return app_language::Tr("DragDropTarget", "Граница области перетаскивания");
    case ImGuiCol_DragDropTargetBg: return app_language::Tr("DragDropTargetBg", "Фон области перетаскивания");
    case ImGuiCol_UnsavedMarker: return app_language::Tr("UnsavedMarker", "Метка несохранённых изменений");
    case ImGuiCol_NavCursor: return app_language::Tr("NavCursor", "Курсор навигации");
    case ImGuiCol_NavWindowingHighlight: return app_language::Tr("NavWindowingHighlight", "Выделение окна при переключении");
    case ImGuiCol_NavWindowingDimBg: return app_language::Tr("NavWindowingDimBg", "Затемнение фона при переключении окон");
    case ImGuiCol_ModalWindowDimBg: return app_language::Tr("ModalWindowDimBg", "Затемнение фона модального окна");
    default: return ImGui::GetStyleColorName(color);
    }
}

void ShowStyleEditor(bool* p_open)
{
    // Reference style for the per-color Save/Revert buttons; the menu actions below update it too.
    static ImGuiStyle ref_saved_style = ImGui::GetStyle();

    ImGui::Begin(app_language::Label("Style Editor", "Редактор стиля"), p_open, ImGuiWindowFlags_MenuBar);

    const std::string xmlFilters = std::string(app_language::Tr("XML files", "Файлы XML")) + '\0' + "*.xml" + '\0' +
        app_language::Tr("All files", "Все файлы") + '\0' + "*.*" + '\0';
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(app_language::Label("File", "Файл")))
        {
            if (ImGui::MenuItem(app_language::Label("Save to XML...", "Сохранить в XML...")))
            {
                const char* filename = OpenStyleFileDialog(NOC_FILE_DIALOG_SAVE,
                    xmlFilters.c_str(), "style.xml");
                if (filename)
                {
                    if (!SaveStyleToXML(filename, ImGui::GetStyle()))
                        ImGui::OpenPopup(app_language::Label("Error##SaveStyle", "Ошибка##SaveStyle"));
                }
            }
            if (ImGui::MenuItem(app_language::Label("Load from XML...", "Загрузить из XML...")))
            {
                const char* filename = OpenStyleFileDialog(NOC_FILE_DIALOG_OPEN,
                    xmlFilters.c_str(), nullptr);
                if (filename)
                {
                    if (LoadStyleFromXML(filename, ImGui::GetStyle()))
                    {
                        // Update the saved reference style
                        ref_saved_style = ImGui::GetStyle();
                    }
                    else
                        ImGui::OpenPopup(app_language::Label("Error##LoadStyle", "Ошибка##LoadStyle"));
                }
            }
            if (ImGui::MenuItem(app_language::Label("Save as Custom", "Сохранить как пользовательский")))
            {
                if (!SaveStyleToXML(ApplicationFile("custom.style.xml").c_str(), ImGui::GetStyle()))
                    ImGui::OpenPopup(app_language::Label("Error##SaveStyle", "Ошибка##SaveStyle"));
            }
            ImGui::Separator();
            if (ImGui::MenuItem(app_language::Label("Reset to Default", "Сбросить по умолчанию")))
            {
                ImGui::GetStyle() = ImGuiStyle();
                ref_saved_style = ImGui::GetStyle();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Error popups
    if (ImGui::BeginPopupModal(app_language::Label("Error##SaveStyle", "Ошибка##SaveStyle"), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(app_language::Tr("Failed to save style to XML file.", "Не удалось сохранить стиль в файл XML."));
        if (ImGui::Button(app_language::Label("OK", "ОК"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal(app_language::Label("Error##LoadStyle", "Ошибка##LoadStyle"), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(app_language::Tr("Failed to load style from XML file.", "Не удалось загрузить стиль из файла XML."));
        if (ImGui::Button(app_language::Label("OK", "ОК"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiStyle* ref = &ref_saved_style;

        ImGui::Spacing();

        if (ImGui::BeginTabItem(app_language::Label("Sizes", "Размеры")))
        {
            ImGui::Text(app_language::Tr("Main", "Основные"));
            ImGui::SliderFloat2(app_language::Label("WindowPadding", "Отступы окна"), (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(app_language::Label("FramePadding", "Отступы рамки"), (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(app_language::Label("CellPadding", "Отступы ячейки"), (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(app_language::Label("ItemSpacing", "Интервал между элементами"), (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(app_language::Label("ItemInnerSpacing", "Внутренний интервал элемента"), (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(app_language::Label("TouchExtraPadding", "Дополнительная сенсорная область"), (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("IndentSpacing", "Ширина отступа"), &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("ScrollbarSize", "Ширина полосы прокрутки"), &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("GrabMinSize", "Минимальный размер ползунка"), &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
            ImGui::Text(app_language::Tr("Borders", "Границы"));
            ImGui::SliderFloat(app_language::Label("WindowBorderSize", "Толщина границы окна"), &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("ChildBorderSize", "Толщина границы дочернего окна"), &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("PopupBorderSize", "Толщина границы всплывающего окна"), &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("FrameBorderSize", "Толщина границы рамки"), &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("TabBorderSize", "Толщина границы вкладки"), &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::Text(app_language::Tr("Rounding", "Скругление"));
            ImGui::SliderFloat(app_language::Label("WindowRounding", "Скругление окна"), &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("ChildRounding", "Скругление дочернего окна"), &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("FrameRounding", "Скругление рамки"), &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("PopupRounding", "Скругление всплывающего окна"), &style.PopupRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("ScrollbarRounding", "Скругление полосы прокрутки"), &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("GrabRounding", "Скругление ползунка"), &style.GrabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(app_language::Label("TabRounding", "Скругление вкладки"), &style.TabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::Text(app_language::Tr("Alignment", "Выравнивание"));
            ImGui::SliderFloat2(app_language::Label("WindowTitleAlign", "Выравнивание заголовка окна"), (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
            const char* menuPositions[] = { app_language::Tr("Left", "Слева"), app_language::Tr("Right", "Справа") };
            ImGui::Combo(app_language::Label("WindowMenuButtonPosition", "Положение кнопки меню окна"), (int*)&style.WindowMenuButtonPosition, menuPositions, IM_ARRAYSIZE(menuPositions));
            ImGui::SliderFloat2(app_language::Label("ButtonTextAlign", "Выравнивание текста кнопки"), (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat2(app_language::Label("SelectableTextAlign", "Выравнивание текста выбора"), (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::Text(app_language::Tr("Safe Area Padding", "Отступы безопасной области"));
            ImGui::SliderFloat2(app_language::Label("DisplaySafeAreaPadding", "Отступы безопасной области экрана"), (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(app_language::Label("Colors", "Цвета")))
        {
            static ImGuiTextFilter filter;
            filter.Draw(app_language::Label("Filter colors", "Фильтр цветов"), ImGui::GetFontSize() * 16);

            static int alpha_mode = 0; // 0=Opaque, 1=Alpha, 2=Both
            ImGui::RadioButton(app_language::Label("Opaque", "Без прозрачности"), &alpha_mode, 0); ImGui::SameLine();
            ImGui::RadioButton(app_language::Label("Alpha", "Прозрачность"), &alpha_mode, 1); ImGui::SameLine();
            ImGui::RadioButton(app_language::Label("Both", "Оба режима"), &alpha_mode, 2); ImGui::SameLine();

            ImGui::BeginChild("##colors", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
            ImGui::PushItemWidth(-160);
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                const char* name = StyleColorDisplayName(i);
                if (!filter.PassFilter(name))
                    continue;
                ImGui::PushID(i);
                ImGuiColorEditFlags alpha_flags = 0;
                if (alpha_mode == 1) alpha_flags = ImGuiColorEditFlags_AlphaPreview;
                else if (alpha_mode == 2) alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf;
                ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
                if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0)
                {
                    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                    if (ImGui::Button(app_language::Label("Save", "Сохранить"))) ref->Colors[i] = style.Colors[i];
                    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                    if (ImGui::Button(app_language::Label("Revert", "Отменить"))) style.Colors[i] = ref->Colors[i];
                }
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::TextUnformatted(name);
                ImGui::PopID();
            }
            ImGui::PopItemWidth();
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void ShowMenuWindow(MainContext* ctx)
{
    // Use the global window list
    for (size_t i = 0; i < g_Windows.size(); i++)
    {
        if (ImGui::MenuItem(g_Windows[i].name.c_str(), "", &g_Windows[i].show))
            SaveDashboardWindowVisibility();
    }

    ImGui::Separator();

    if (ImGui::MenuItem(app_language::Label("Top Most", "Поверх всех окон"), "", &ctx->TopMost)) {
        SDL_SetWindowAlwaysOnTop(ctx->Wimdow, ctx->TopMost ? SDL_TRUE : SDL_FALSE);
        SaveIniSettings(ctx);
    }

    int font = FontSelector(app_language::Label("Fonts##Selector", "Шрифты##Selector"));
    if (font != -1)
    {
        ctx->FontSize = font;
        SaveIniSettings(ctx);
    }

    ImGui::Separator();

    // Preserve the serialized style indices; names are localized only for display.
    const char* styleNames[] = {
        app_language::Tr("Classic", "Классический"), app_language::Tr("Blue", "Синий"),
        app_language::Tr("Light", "Светлый"), app_language::Tr("Soft", "Мягкий"),
        app_language::Tr("Blender", "Blender"), app_language::Tr("Cyberpunk", "Киберпанк"),
        app_language::Tr("Nord", "Nord"), app_language::Tr("Black", "Чёрный"),
        app_language::Tr("Green", "Зелёный"), app_language::Tr("Red", "Красный"),
        app_language::Tr("Embrace", "Embrace"), app_language::Tr("Dracula", "Дракула"),
        app_language::Tr("Visual Studio", "Visual Studio"), app_language::Tr("One Dark", "One Dark"),
        app_language::Tr("Light Classic", "Светлый классический"), app_language::Tr("Cherry", "Вишнёвый"),
        app_language::Tr("Nord Light", "Nord светлый"), app_language::Tr("Matrix", "Матрица"),
        app_language::Tr("High Contrast", "Высокая контрастность"), app_language::Tr("Adobe Spectrum", "Adobe Spectrum"),
        app_language::Tr("Doug Binks Dark", "Doug Binks тёмный"), app_language::Tr("Enemy Mouse", "Enemy Mouse"),
        app_language::Tr("LED Synth Master", "LED Synth Master"), app_language::Tr("Custom", "Пользовательский")
    };
    if (ImGui::Combo(app_language::Label("Style", "Стиль"), &ctx->Style, styleNames, IM_ARRAYSIZE(styleNames)))
    {
        switch (ctx->Style)
        {
        case 0:  ImGui::StyleColorsClassic(); break;
        case 1:  ImGui::StyleColorsDark(); break;
        case 2:  ImGui::StyleColorsLight(); break;
        case 3:  ImGui::StyleColorsSoft(); break;
        case 4:  ImGui::StyleColorsBlender(); break;
        case 5:  ImGui::StyleColorsCyberpunk(); break;
        case 6:  ImGui::StyleColorsNord(); break;
        case 7:  ImGui::StyleColorsBlack(); break;
        case 8:  ImGui::StyleColorsGreen(); break;
        case 9:  ImGui::StyleColorsRed(); break;
        case 10: ImGui::StyleColorsEmbrace(); break;
        case 11: ImGui::StyleColorsDracula(); break;
        case 12: ImGui::StyleColorsVisualStudio(); break;
        case 13: ImGui::StyleColorsOneDark(); break;
        case 14: ImGui::StyleColorsLightClassic(); break;
        case 15: ImGui::StyleColorsCherry(); break;
        case 16: ImGui::StyleColorsNordLight(); break;
        case 17: ImGui::StyleColorsMatrix(); break;
        case 18: ImGui::StyleColorsHighContrast(); break;
        case 19: ImGui::StyleColorsAdobeSpectrum(); break;
        case 20: ImGui::StyleColorsDougBinksDark(); break;
        case 21: ImGui::StyleColorsEnemyMouse(); break;
        case 22: ImGui::StyleColorsLEDSynthMaster(); break;
        case 23: // Custom style
        {
            ImGuiStyle& style = ImGui::GetStyle();
            if (!LoadStyleFromXML(ApplicationFile("custom.style.xml").c_str(), style))
            {
                // if the file is missing, nothing changes
            }
        }
        break;
        default: ImGui::StyleColorsNord(); break;
        }
        SaveIniSettings(ctx);
    }
    ImGui::MenuItem(app_language::Label("Style Editor", "Редактор стиля"), NULL, &ctx->show_style_editor);

    ImGui::Separator();
    if (ImGui::MenuItem(app_language::Label("XML Editor", "Редактор XML"), NULL, &ctx->show_xml_editor)) {
        // nothing to do
    }
    bool design = ctx->design_mode;
    if (ImGui::MenuItem(app_language::Label("Design Mode", "Режим дизайна"), "Ctrl+E", &design, DesignMode::CanEnable(ctx))) {
        DesignMode::SetEnabled(ctx, design);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", DesignMode::CanEnable(ctx)
            ? app_language::Tr("Edit the widgets with the mouse: select, move, resize, and change their properties", "Редактируйте виджеты мышью: выбирайте, перемещайте, меняйте размер и свойства")
            : app_language::Tr("Open or create an XML file first", "Сначала откройте или создайте файл XML"));
    }
}

static const char* ConnectionStateText(JLinkConnectionState state)
{
    switch (state) {
    case JLinkConnectionState::Connecting: return app_language::Tr("Connecting", "Подключение");
    case JLinkConnectionState::Connected: return app_language::Tr("Connected", "Подключено");
    case JLinkConnectionState::Reconnecting: return app_language::Tr("Reconnecting", "Повторное подключение");
    case JLinkConnectionState::Failed: return app_language::Tr("Connection failed", "Ошибка подключения");
    case JLinkConnectionState::Stopping: return app_language::Tr("Disconnecting", "Отключение");
    default: return app_language::Tr("Disconnected", "Отключено");
    }
}

static void ShowConnectionStatus(bool details)
{
    const auto status = GetJLinkConnectionStatus();
    const ImVec4 color = status.state == JLinkConnectionState::Connected ? ImVec4(0, 1, 0, 1) :
        status.state == JLinkConnectionState::Failed ? ImVec4(1, 0.35f, 0.25f, 1) : ImVec4(1, 0.75f, 0.2f, 1);
    ImGui::TextColored(color, app_language::Tr("J-Link: %s%s", "J-Link: %s%s"), ConnectionStateText(status.state),
        status.lastError.empty() && !status.writeMayHaveCompleted ? "" : " (!)");
    if (ImGui::IsItemHovered() && (!status.lastError.empty() || status.writeMayHaveCompleted))
        ImGui::SetTooltip(app_language::Tr("Last error: %s%s", "Последняя ошибка: %s%s"), app_language::Diagnostic(status.lastError).c_str(), status.writeMayHaveCompleted ?
            app_language::Tr("\nA write may have partially completed. Check the target; pending writes were discarded.", "\nЗапись могла выполниться частично. Проверьте целевое устройство; ожидающие записи отменены.") : "");
    if (details) {
        if (status.attempt) ImGui::Text(app_language::Tr("Attempt %u of 5", "Попытка %u из 5"), status.attempt);
        if (status.retryInMs) ImGui::Text(app_language::Tr("Next attempt in %.1f s", "Следующая попытка через %.1f с"), status.retryInMs / 1000.0);
        if (!status.lastError.empty()) {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 500.0f);
            ImGui::TextWrapped(app_language::Tr("Last error: %s", "Последняя ошибка: %s"), app_language::Diagnostic(status.lastError).c_str());
            ImGui::PopTextWrapPos();
        }
        if (status.writeMayHaveCompleted)
            ImGui::TextWrapped(app_language::Tr("A write may have partially completed. Check the target before writing again; pending writes were discarded.", "Запись могла выполниться частично. Проверьте целевое устройство перед повторной записью; ожидающие записи отменены."));
        if (status.state == JLinkConnectionState::Failed)
            ImGui::TextDisabled(app_language::Tr("Automatic attempts stopped. Fix the cause and press Connect to retry.", "Автоматические попытки прекращены. Устраните причину и нажмите «Подключить» для повтора."));
    }
}

static bool g_RequestWriteUnlock = false;
static uint64_t g_WriteUnlockEpoch = 0;
static bool g_WriteUnlockAcknowledged = false;
static const char* g_WriteValueTypes[] = {"U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64", "Float32", "Float64"};

static void ShowWriteProtectionSettings(MainContext* ctx)
{
    const auto status = GetWriteProtectionStatus();
    ImGui::SeparatorText(app_language::Tr("Target write protection", "Защита целевого устройства от записи"));
    ImGui::TextColored(status.readOnly ? ImVec4(0.3f, 1, 0.4f, 1) : ImVec4(1, 0.4f, 0.2f, 1),
        "%s", status.readOnly ? app_language::Tr("READ ONLY - target writes blocked", "ТОЛЬКО ЧТЕНИЕ — запись в устройство заблокирована") : app_language::Tr("WRITES ENABLED for this connection", "ЗАПИСЬ РАЗРЕШЕНА для этого подключения"));
    if (status.readOnly) {
        ImGui::BeginDisabled(!ctx->StartComunication);
        if (ImGui::Button(app_language::Label("Enable writes...", "Разрешить запись..."))) {
            g_WriteUnlockEpoch = status.epoch;
            g_WriteUnlockAcknowledged = false;
            g_RequestWriteUnlock = true;
        }
        ImGui::EndDisabled();
    } else if (ImGui::Button(app_language::Label("Lock target writes now", "Заблокировать запись сейчас"))) {
        LockTargetWrites(ctx);
    }
    ImGui::TextDisabled(app_language::Tr("Writes lock again after disconnect, I/O failure or protection changes.", "Запись снова блокируется при отключении, ошибке ввода-вывода или изменении защиты."));
    ImGui::Text(app_language::Tr("Blocked requests: %llu", "Заблокировано запросов: %llu"), static_cast<unsigned long long>(status.blockedWrites));
    if (!status.lastError.empty()) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 500);
        ImGui::TextWrapped(app_language::Tr("Write protection: %s", "Защита от записи: %s"), app_language::Diagnostic(status.lastError).c_str());
        ImGui::PopTextWrapPos();
    }
    if (status.restrictAddresses)
        ImGui::Text(app_language::Tr("Active allowed addresses: 0x%08X - 0x%08X (inclusive)", "Разрешённые адреса: 0x%08X - 0x%08X (включительно)"), status.firstAddress, status.lastAddress);
    else ImGui::TextDisabled(app_language::Tr("No extra address limit; the MCU memory mapping still applies.", "Дополнительных ограничений адресов нет; отображение памяти МК по-прежнему действует."));

    // Policy editing is available only while locked. Edits are drafts until Apply.
    ImGui::BeginDisabled(!status.readOnly);
    static bool initialized = false, limitAddresses = false;
    static uint32_t first = 0, last = 0;
    if (!initialized) {
        first = ctx->mcu_memory_base;
        const uint64_t end = static_cast<uint64_t>(first) + IOMAP_SIZE - 1;
        last = end > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(end);
        initialized = true;
    }
    ImGui::Checkbox(app_language::Label("Limit writable addresses (draft)", "Ограничить адреса для записи (черновик)"), &limitAddresses);
    ImGui::SetNextItemWidth(160);
    ImGui::InputScalar(app_language::Label("First address##write", "Первый адрес##write"), ImGuiDataType_U32, &first, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SetNextItemWidth(160);
    ImGui::InputScalar(app_language::Label("Last address (inclusive)##write", "Последний адрес (включительно)##write"), ImGuiDataType_U32, &last, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    if (ImGui::Button(app_language::Label("Apply address limit", "Применить ограничение адресов"))) SetWriteAddressRange(ctx, limitAddresses, first, last);

    ImGui::SeparatorText(app_language::Tr("Optional value limits by address", "Дополнительные ограничения значений по адресам"));
    static uint32_t ruleAddress = 0;
    static int ruleType = 1;
    static char minimum[64] = "0", maximum[64] = "65535";
    ImGui::SetNextItemWidth(160);
    ImGui::InputScalar(app_language::Label("Address##rule", "Адрес##rule"), ImGuiDataType_U32, &ruleAddress, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SetNextItemWidth(160);
    ImGui::Combo(app_language::Label("Stored data type", "Тип хранимых данных"), &ruleType, g_WriteValueTypes, 10);
    ImGui::SetNextItemWidth(240);
    ImGui::InputText(app_language::Label("Minimum (decimal)", "Минимум (десятичный)"), minimum, sizeof(minimum));
    ImGui::SetNextItemWidth(240);
    ImGui::InputText(app_language::Label("Maximum (decimal)", "Максимум (десятичный)"), maximum, sizeof(maximum));
    if (ImGui::Button(app_language::Label("Add value limit", "Добавить ограничение значений")))
        AddWriteValueRule(ctx, {ruleAddress, static_cast<WriteValueType>(ruleType), minimum, maximum});
    for (unsigned i = 0; i < status.rules.size(); ++i) {
        const auto& rule = status.rules[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::Text("0x%08X %s: [%s, %s]", rule.address, g_WriteValueTypes[static_cast<int>(rule.type)],
            rule.minimum.c_str(), rule.maximum.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(app_language::Label("Remove", "Удалить"))) RemoveWriteValueRule(ctx, i);
        ImGui::PopID();
    }
    ImGui::TextDisabled(app_language::Tr("Partial/overlapping writes to a value rule are rejected, including byte edits.", "Частичные и перекрывающиеся записи в область правила отклоняются, включая побайтовые изменения."));
    ImGui::EndDisabled();
}

static void ShowLanguageMenu()
{
    // Keep the submenu bilingual so either locale is easy to find. Changing
    // language is UI-only and must not affect the current target/write lock.
    if (ImGui::BeginMenu(app_language::Label("Language / Язык", "Язык / Language"))) {
        if (ImGui::MenuItem("English", nullptr, !app_language::IsRussian())) {
            app_language::Set(app_language::Language::English);
            prefs::WriteString("SETTINGS.Language", app_language::Code());
        }
        if (ImGui::MenuItem("Русский", nullptr, app_language::IsRussian())) {
            app_language::Set(app_language::Language::Russian);
            prefs::WriteString("SETTINGS.Language", app_language::Code());
        }
        ImGui::EndMenu();
    }
}

static void ShowWriteUnlockDialog(MainContext* ctx)
{
    if (g_RequestWriteUnlock) {
        ImGui::OpenPopup(app_language::Label("Enable target writes?", "Разрешить запись в целевое устройство?"));
        g_RequestWriteUnlock = false;
    }
    if (!ImGui::BeginPopupModal(app_language::Label("Enable target writes?", "Разрешить запись в целевое устройство?"), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    const auto status = GetWriteProtectionStatus();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 520);
    ImGui::TextWrapped(app_language::Tr("Writing can change outputs, start machinery or damage the target. This enables ALL permitted writes for the current connection, not just one click.", "Запись может изменить выходные сигналы, запустить оборудование или повредить целевое устройство. Это разрешает ВСЕ допустимые записи для текущего подключения, а не только одно действие."));
    ImGui::Text(app_language::Tr("Target device: %s", "Целевое устройство: %s"), ctx->jlink_device);
    ImGui::Text(app_language::Tr("MCU mapping base: 0x%08X", "Базовый адрес отображения памяти МК: 0x%08X"), ctx->mcu_memory_base);
    if (status.restrictAddresses)
        ImGui::Text(app_language::Tr("Allowed addresses: 0x%08X - 0x%08X", "Разрешённые адреса: 0x%08X - 0x%08X"), status.firstAddress, status.lastAddress);
    else ImGui::TextWrapped(app_language::Tr("No additional address limit is set. All addresses in the MCU mapping may be written.", "Дополнительное ограничение адресов не задано. Запись разрешена по всем адресам отображения памяти МК."));
    ImGui::Text(app_language::Tr("Additional value limits: %u", "Дополнительных ограничений значений: %u"), static_cast<unsigned>(status.rules.size()));
    ImGui::TextWrapped(app_language::Tr("The lock covers widgets, memory editors and automatic DoneAddr acknowledgements. An already submitted SDK write cannot be recalled by locking.", "Блокировка распространяется на виджеты, редакторы памяти и автоматические подтверждения DoneAddr. Уже переданную в SDK запись нельзя отменить блокировкой."));
    const bool sameSession = ctx->StartComunication && status.epoch == g_WriteUnlockEpoch;
    if (!sameSession) ImGui::TextWrapped(app_language::Tr("Connection or protection changed. Cancel and review the new session before enabling writes.", "Подключение или защита изменились. Отмените действие и проверьте новый сеанс, прежде чем разрешать запись."));
    ImGui::Checkbox(app_language::Label("I checked the target and accept enabling writes", "Я проверил целевое устройство и согласен разрешить запись"), &g_WriteUnlockAcknowledged);
    ImGui::BeginDisabled(!sameSession || !g_WriteUnlockAcknowledged);
    if (ImGui::Button(app_language::Label("Enable for this connection", "Разрешить для этого подключения"))) {
        if (ArmTargetWrites(ctx, g_WriteUnlockEpoch)) ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(app_language::Label("Cancel", "Отмена"))) ImGui::CloseCurrentPopup();
    ImGui::PopTextWrapPos();
    ImGui::EndPopup();
}

void ShowMenuSettings(MainContext* ctx)
{
    // --- MCU base address (hex input) ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text(app_language::Tr("MCU Memory address 0x", "Адрес памяти МК 0x"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(170);
    uint32_t addr = ctx->mcu_memory_base;
    if (ImGui::InputScalar("##mcu_base_menu", ImGuiDataType_U32, &addr, NULL, NULL,
        "%08X", ImGuiInputTextFlags_CharsHexadecimal))
    {
        // Pending requests are offsets relative to the old base address.
        // Stop and discard them before changing the mapping.
        StopCommunication(ctx);
        ctx->mcu_memory_base = addr;
        SaveIniSettings(ctx);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(app_language::Tr("Base address of the MCU memory region (e.g., 0x20000000)", "Базовый адрес области памяти МК (например, 0x20000000)"));

    // --- Auto Connect checkbox ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text(app_language::Tr("Auto Connect", "Автоподключение"));
    ImGui::SameLine();
    if (ImGui::Checkbox("##autoconnect", &ctx->auto_connect)) {
        SaveIniSettings(ctx);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(app_language::Tr("Automatically connect to J-Link on program start.", "Автоматически подключаться к J-Link при запуске программы."));

    // Cancel remains available during connection and retry backoff.
    ImGui::SameLine();
    if (IsCommunicationRequested()) {
        if (ImGui::Button(ctx->StartComunication ? app_language::Label("Disconnect", "Отключить") : app_language::Label("Cancel connection", "Отменить подключение")))
            StopCommunication(ctx);
    } else if (ImGui::Button(app_language::Label("Connect", "Подключить"))) {
        StartCommunication(ctx);
    }
    ShowConnectionStatus(true);
    ShowWriteProtectionSettings(ctx);

    // --- J-Link device selection with filterable list ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text(app_language::Tr("J-Link device", "Устройство J-Link"));
    ImGui::SameLine();

    static std::vector<std::string> all_devices;
    static std::vector<std::string> all_devices_lower;
    static std::vector<int> filtered_indices;
    static char filter_buf[64] = "";
    static bool init_done = false;

    // Build the device list on the first call
    if (!init_done && TryGetJLinkDeviceNames(all_devices)) {
        for (const std::string& name : all_devices) {
            std::string lower = name;
            for (auto& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            all_devices_lower.push_back(lower);
        }
        // If the list is empty, add the current device
        if (all_devices.empty()) {
            all_devices.push_back(ctx->jlink_device);
            std::string lower = ctx->jlink_device;
            for (auto& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            all_devices_lower.push_back(lower);
        }

        // Initialize the filter: show everything
        filtered_indices.resize(all_devices.size());
        for (size_t i = 0; i < all_devices.size(); ++i) filtered_indices[i] = (int)i;

        init_done = true;
    }

    // Case-insensitive filtering
    static char prev_filter[64] = "";
    if (strcmp(filter_buf, prev_filter) != 0) {
        strcpy(prev_filter, filter_buf);
        filtered_indices.clear();

        if (strlen(filter_buf) == 0) {
            for (size_t i = 0; i < all_devices.size(); ++i)
                filtered_indices.push_back((int)i);
        }
        else {
            std::string filter_lower = filter_buf;
            for (auto& c : filter_lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            for (size_t i = 0; i < all_devices.size(); ++i) {
                if (all_devices_lower[i].find(filter_lower) != std::string::npos) {
                    filtered_indices.push_back((int)i);
                }
            }
        }
    }

    // Show the currently selected device
    ImGui::Text("%s", ctx->jlink_device);
    ImGui::SameLine();
    ImGui::TextDisabled(app_language::Tr("(current)", "(текущее)"));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(app_language::Tr("Currently selected device. Select a new one from the list below.", "Текущее выбранное устройство. Выберите другое из списка ниже."));

    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(app_language::Tr("Filter:", "Фильтр:"));
    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    ImGui::InputText("##filter", filter_buf, sizeof(filter_buf));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button(app_language::Label("Clear", "Очистить"))) {
        filter_buf[0] = '\0';
        strcpy(prev_filter, "");
        filtered_indices.clear();
        for (size_t i = 0; i < all_devices.size(); ++i)
            filtered_indices.push_back((int)i);
    }
    ImGui::SameLine();
    ImGui::TextDisabled(app_language::Tr("(%d found)", "(найдено: %d)"), (int)filtered_indices.size());

    // Device list
    if (ImGui::BeginListBox("##device_list", ImVec2(500, 250))) {
        for (int idx : filtered_indices) {
            const char* item = all_devices[idx].c_str();
            bool is_selected = (strcmp(item, ctx->jlink_device) == 0);
            if (ImGui::Selectable(item, is_selected)) {
                if (!is_selected) StopCommunication(ctx);
                // Save the selected device right away
                strncpy(ctx->jlink_device, item, sizeof(ctx->jlink_device) - 1);
                ctx->jlink_device[sizeof(ctx->jlink_device) - 1] = '\0';
                SaveIniSettings(ctx);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(app_language::Tr("Click to select target device for J-Link", "Нажмите, чтобы выбрать целевое устройство для J-Link"));
}

void ShowUserGuide(bool* p_open)
{
    user_guide::Show(p_open);
}

void ShowAbout(bool* p_open)
{
    if (!ImGui::Begin(app_language::Label("About J-Link Utils", "О программе J-Link Utils"), p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    if (g_AppIconTexture) {
        ImGui::Image((ImTextureID)(intptr_t)g_AppIconTexture, ImVec2(64.0f, 64.0f));
        ImGui::SameLine();
    }
    ImGui::BeginGroup();
    ImGui::TextUnformatted(app_language::Tr("J-Link Utils", "J-Link Utils"));
    ImGui::Text(app_language::Tr("Version 2.0", "Версия 2.0"));
    ImGui::EndGroup();
    ImGui::Separator();
    ImGui::Text(app_language::Tr("A utility application for debugging and configuring microcontrollers via J-Link.", "Приложение для отладки и настройки микроконтроллеров через J-Link."));
    ImGui::Text(app_language::Tr("Built with SDL2, ImGui, and ImPlot.", "Создано с использованием SDL2, ImGui и ImPlot."));
    ImGui::Text(" ");
    ImGui::Text(app_language::Tr("Libraries used:", "Используемые библиотеки:"));
    ImGui::BulletText(app_language::Tr("SDL %d.%d.%d", "SDL %d.%d.%d"), SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    ImGui::BulletText(app_language::Tr("Dear ImGui %s", "Dear ImGui %s"), ImGui::GetVersion());
    ImGui::BulletText(app_language::Tr("ImPlot v0.18 WIP", "ImPlot v0.18 (в разработке)"));
    ImGui::BulletText(app_language::Tr("tinyxml2 %d.%d.%d", "tinyxml2 %d.%d.%d"), TINYXML2_MAJOR_VERSION, TINYXML2_MINOR_VERSION, TINYXML2_PATCH_VERSION);
    ImGui::BulletText(app_language::Tr("lfqueue", "lfqueue"));
    ImGui::Text(" ");
    ImGui::Text(app_language::Tr("Build date: %s %s", "Дата сборки: %s %s"), __DATE__, __TIME__);
#ifdef _MSC_VER
    ImGui::Text(app_language::Tr("Compiler: Microsoft Visual C++ %d", "Компилятор: Microsoft Visual C++ %d"), _MSC_VER);
#elif defined(__GNUC__)
    ImGui::Text(app_language::Tr("Compiler: GCC %d.%d.%d", "Компилятор: GCC %d.%d.%d"), __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(__clang__)
    ImGui::Text(app_language::Tr("Compiler: Clang %d.%d.%d", "Компилятор: Clang %d.%d.%d"), __clang_major__, __clang_minor__, __clang_patchlevel__);
#else
    ImGui::Text(app_language::Tr("Compiler: unknown", "Компилятор: неизвестен"));
#endif
    ImGui::Text(app_language::Tr("Platform: %s", "Платформа: %s"),
#ifdef _WIN64
        app_language::Tr("Windows 64-bit", "Windows 64-разрядная")
#elif _WIN32
        app_language::Tr("Windows 32-bit", "Windows 32-разрядная")
#elif __linux__
        "Linux"
#elif __APPLE__
        "macOS"
#else
        app_language::Tr("Unknown", "Неизвестно")
#endif
    );

    ImGui::Separator();
    if (ImGui::Button(app_language::Label("Copy to clipboard", "Копировать в буфер обмена")))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("J-Link Utils 2.0\n");
        ImGui::LogText("Built with SDL2, ImGui, ImPlot\n");
        ImGui::LogText("Build date: %s %s\n", __DATE__, __TIME__);
        ImGui::LogFinish();
    }

    ImGui::End();
}

static void MenuWindow(MainContext* ctx)
{
    // The native client area is the only source of truth. Never resize the OS
    // window to fit an internal ImGui window (or its collapsed title bar).
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("###MainWindow", nullptr, flags))
    {
        const char* title = ctx->File_label[0] ? ctx->File_label : "J-Link Utils";
        if (SDL_strcmp(SDL_GetWindowTitle(ctx->Wimdow), title) != 0)
            SDL_SetWindowTitle(ctx->Wimdow, title);
        ApplyMainWindowAction(ctx->Wimdow,
            DrawMainWindowTitleBar(ctx->Wimdow, title, g_MainWindowChrome), ctx->IsClosing);

        if (ImGui::BeginMenuBar())
        {
            static bool show_app_about = false, show_app_userguide = false;

            if (show_app_userguide) { ShowUserGuide(&show_app_userguide); }
            if (show_app_about) { ShowAbout(&show_app_about); }
            if (ctx->show_style_editor) { ShowStyleEditor(&ctx->show_style_editor); }

            if (ImGui::BeginMenu(app_language::Label("File", "Файл")))
            {
                ShowMenuFile(ctx);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(app_language::Label("Windows", "Окна")))
            {
                ShowMenuWindow(ctx);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(app_language::Label("Settings", "Настройки")))
            {
                ShowLanguageMenu();
                ImGui::Separator();
                ShowMenuSettings(ctx);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(app_language::Label("Help", "Справка")))
            {
                ImGui::MenuItem(app_language::Label("User Guide..", "Руководство пользователя.."), NULL, &show_app_userguide);
                ImGui::MenuItem(app_language::Label("About..", "О программе.."), NULL, &show_app_about);
                ImGui::EndMenu();
            }

            ShowConnectionStatus(false);
            const auto writeStatus = GetWriteProtectionStatus();
            ImGui::TextColored(writeStatus.readOnly ? ImVec4(0.3f, 1, 0.4f, 1) : ImVec4(1, 0.4f, 0.2f, 1),
                "%s", writeStatus.readOnly ? app_language::Tr("[READ ONLY]", "[ТОЛЬКО ЧТЕНИЕ]") : app_language::Tr("[WRITES ENABLED]", "[ЗАПИСЬ РАЗРЕШЕНА]"));
            if (!writeStatus.readOnly && ImGui::SmallButton(app_language::Label("Lock writes", "Заблокировать запись"))) LockTargetWrites(ctx);

            if (ctx->openfile)
                if (OpenFile(ctx)) ctx->openfile = false;

            if (ctx->closefile)
                if (CloseFile(ctx)) ctx->closefile = false;

            ImGui::EndMenuBar();
        }

        ShowWriteUnlockDialog(ctx);

        if (!prefs::Error().empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.25f, 1.0f), app_language::Tr("Settings could not be saved or loaded", "Не удалось сохранить или загрузить настройки"));
            ImGui::TextWrapped("%s", app_language::Diagnostic(prefs::Error()).c_str());
            if (ImGui::Button(app_language::Label("Retry saving settings", "Повторить сохранение настроек"))) {
                SaveIniSettings(ctx);
                prefs::Flush();
            }
        }

        if (g_Windows.empty()) {
            ImGui::TextDisabled("%s", ctx->File_path[0] != '\0'
                ? app_language::Tr("No windows loaded. Check the XML file.", "Окна не загружены. Проверьте файл XML.")
                : app_language::Tr("No XML file loaded. Use File > New or File > Open.", "Файл XML не загружен. Выберите «Файл > Создать» или «Файл > Открыть»."));
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}


// ------------------------------------------------------------------
// Renderer information output (for debugging)
// ------------------------------------------------------------------

static void SDL_snprintfcat(SDL_OUT_Z_CAP(maxlen) char* text, size_t maxlen, SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
{
    size_t length = SDL_strlen(text);
    va_list ap;
    va_start(ap, fmt);
    text += length;
    maxlen -= length;
    SDL_vsnprintf(text, maxlen, fmt, ap);
    va_end(ap);
}

static void SDL_PrintRendererFlag(char* text, size_t maxlen, Uint32 flag)
{
    switch (flag) {
    case SDL_RENDERER_SOFTWARE:
        SDL_snprintfcat(text, maxlen, "Software");
        break;
    case SDL_RENDERER_ACCELERATED:
        SDL_snprintfcat(text, maxlen, "Accelerated");
        break;
    case SDL_RENDERER_PRESENTVSYNC:
        SDL_snprintfcat(text, maxlen, "PresentVSync");
        break;
    case SDL_RENDERER_TARGETTEXTURE:
        SDL_snprintfcat(text, maxlen, "TargetTexturesSupported");
        break;
    default:
        SDL_snprintfcat(text, maxlen, "0x%8.8x", flag);
        break;
    }
}

static void SDL_PrintRenderer(SDL_RendererInfo* info)
{
    int i, count;
    char text[1024];

    SDL_Log("  Renderer %s:\n", info->name);

    SDL_snprintf(text, sizeof(text), "    Flags: 0x%8.8X", info->flags);
    SDL_snprintfcat(text, sizeof(text), " (");
    count = 0;
    for (i = 0; i < sizeof(info->flags) * 8; ++i) {
        Uint32 flag = (1 << i);
        if (info->flags & flag) {
            if (count > 0) {
                SDL_snprintfcat(text, sizeof(text), " | ");
            }
            SDL_PrintRendererFlag(text, sizeof(text), flag);
            ++count;
        }
    }
    SDL_snprintfcat(text, sizeof(text), ")");
    SDL_Log("%s\n", text);

    if (info->max_texture_width || info->max_texture_height) {
        SDL_Log("    Max Texture Size: %dx%d\n",
            info->max_texture_width, info->max_texture_height);
    }
}

// ------------------------------------------------------------------
// Program entry point
// ------------------------------------------------------------------

int main(int argc, char* argv[]) 
{
    // Also precede any error dialog: SDL_ShowSimpleMessageBox can initialize
    // video before the normal SDL_Init path.
    ConfigureDpiAwareness();
#ifdef _WIN32
    // Load once, before any SDK call or worker thread. Otherwise individual
    // wrapper calls can show repeated dialogs or return unusable fallback data.
    const bool checkRuntimeOnly = argc == 2 && strcmp(argv[1], "--check-jlink-runtime") == 0;
    const bool runtimeLoaded = JLINKARMDLL_LIB_LoadLibrary() == 0;
    if (checkRuntimeOnly) {
        if (!runtimeLoaded) fprintf(stderr, "J-Link runtime could not be loaded.\n");
        return runtimeLoaded ? 0 : 2; // No config, UI or hardware access for this check.
    }
#endif
    int window_flag = 0;

    MainContext* ctx = allocate_context();
    if (!ctx) return 1;

    LoadIniSettings(ctx);
#ifdef _WIN32
    // Read the saved language before presenting a startup error as well.
    if (!runtimeLoaded) {
#ifdef _WIN64
        const char* message = app_language::Tr("J-Link runtime could not be loaded. Place a licensed, compatible JLink_x64.dll next to this executable, or install the matching SEGGER J-Link software. No DLL is bundled.", "Не удалось загрузить среду выполнения J-Link. Поместите лицензированную совместимую JLink_x64.dll рядом с программой или установите соответствующее ПО SEGGER J-Link. DLL не входит в комплект.");
#else
        const char* message = app_language::Tr("J-Link runtime could not be loaded. Place a licensed, compatible JLinkARM.dll next to this executable, or install the matching SEGGER J-Link software. No DLL is bundled.", "Не удалось загрузить среду выполнения J-Link. Поместите лицензированную совместимую JLinkARM.dll рядом с программой или установите соответствующее ПО SEGGER J-Link. DLL не входит в комплект.");
#endif
        fprintf(stderr, "%s\n", message);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, app_language::Tr("J-Link runtime missing", "Отсутствует среда выполнения J-Link"), message, nullptr);
        free_context(ctx);
        return 2;
    }
#endif
    const bool restoreDesignMode = ctx->design_mode;
    ctx->design_mode = false; // SetEnabled must initialize editor state normally.

    // Was an XML path passed on the command line?
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        SDL_strlcpy(ctx->File_path, argv[1], MAX_PATH);
        printf("Loading XML from command line: %s\n", ctx->File_path);
    }

    ctx->CurrentXMLDoc = new tinyxml2::XMLDocument();
    if (ctx->CurrentXMLDoc->LoadFile(ctx->File_path) != tinyxml2::XML_SUCCESS) {
        // Load error; a popup could be shown here
        delete ctx->CurrentXMLDoc;
        ctx->CurrentXMLDoc = nullptr;
    }

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "1");
    SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "1");

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        free_context(ctx);
        SDL_Quit();
        return 1;
    }

    SDL_Log("SDL Initialized successfully");

    // GetSystemMetrics would mix physical desktop pixels with logical UI sizes.
    // SDL display bounds use the same coordinate system as SDL_CreateWindow.
    FitMainWindowToDisplay(ctx);

    if (ctx->TopMost)
        window_flag = SDL_WINDOW_ALWAYS_ON_TOP;

    ConfigureMainWindowChrome();
    const int initialX = ctx->Win_position_saved ? ctx->Win_x : SDL_WINDOWPOS_CENTERED;
    const int initialY = ctx->Win_position_saved ? ctx->Win_y : SDL_WINDOWPOS_CENTERED;
    ctx->Wimdow = SDL_CreateWindow("J-Link Utils", initialX, initialY, ctx->Win_width, ctx->Win_height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | window_flag);

    if (ctx->Wimdow == NULL)
    {
        DEBUG_PRINTF("Can't Create Window");
        free_context(ctx);
        SDL_Quit();
        return 1;
    }
    else
        SDL_Log("Window Initialized successfully");

    SetAppIcon(ctx->Wimdow);

    SDL_RendererInfo info;
    int n = SDL_GetNumRenderDrivers();
    if (n == 0) {
        SDL_Log("No built-in render drivers\n");
    }
    else {
        SDL_Log("Built-in render drivers:\n");
        for (int i = 0; i < n; ++i) {
            SDL_GetRenderDriverInfo(i, &info);
            SDL_PrintRenderer(&info);
        }
    }

    ctx->Renderer = SDL_CreateRenderer(ctx->Wimdow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (ctx->Renderer == NULL)
    {
        DEBUG_PRINTF("Can't Create Hardware Renderer");
        ctx->ErrRengerer = true;
        ctx->Renderer = SDL_CreateRenderer(ctx->Wimdow, -1, SDL_RENDERER_SOFTWARE);
        if (ctx->Renderer == NULL)
        {
            DEBUG_PRINTF("Can't Create Software Renderer");
            SDL_DestroyWindow(ctx->Wimdow);
            free_context(ctx);
            SDL_Quit();
            return 1;
        }
    }
    else
        SDL_Log("Renderer Initialized successfully");

    SDL_SetWindowMinimumSize(ctx->Wimdow, 240, 120);
    if (ctx->Win_maximized) SDL_MaximizeWindow(ctx->Wimdow);

    g_AppIconTexture = CreateAppIconTexture(ctx->Renderer);

    ImGui::CreateContext();
    ImPlot::CreateContext();

    static const std::string imguiIniPath = ApplicationFile("window.ini");
    ImGui::GetIO().IniFilename = imguiIniPath.c_str();

    ImGui_ImplSDLRenderer2_Init(ctx->Renderer);
    ImGui_ImplSDL2_InitForSDLRenderer(ctx->Wimdow, ctx->Renderer);

    // Load fonts once; the dynamic atlas handles DPI changes without invalidating
    // font pointers or changing the persisted font selection.
    LoadFonts(ctx);

    // Apply the saved style
    switch (ctx->Style)
    {
    case 0:  ImGui::StyleColorsClassic(); break;
    case 1:  ImGui::StyleColorsDark();    break;
    case 2:  ImGui::StyleColorsLight();   break;
    case 3:  ImGui::StyleColorsSoft();    break;
    case 4:  ImGui::StyleColorsBlender(); break;
    case 5:  ImGui::StyleColorsCyberpunk(); break;
    case 6:  ImGui::StyleColorsNord();    break;
    case 7:  ImGui::StyleColorsBlack();   break;
    case 8:  ImGui::StyleColorsGreen();   break;
    case 9:  ImGui::StyleColorsRed();     break;
    case 10: ImGui::StyleColorsEmbrace(); break;
    case 11: ImGui::StyleColorsDracula(); break;
    case 12: ImGui::StyleColorsVisualStudio(); break;
    case 13: ImGui::StyleColorsOneDark(); break;
    case 14: ImGui::StyleColorsLightClassic(); break;
    case 15: ImGui::StyleColorsCherry(); break;
    case 16: ImGui::StyleColorsNordLight(); break;
    case 17: ImGui::StyleColorsMatrix(); break;
    case 18: ImGui::StyleColorsHighContrast(); break;
    case 19: ImGui::StyleColorsAdobeSpectrum(); break;
    case 20: ImGui::StyleColorsDougBinksDark(); break;
    case 21: ImGui::StyleColorsEnemyMouse(); break;
    case 22: ImGui::StyleColorsLEDSynthMaster(); break;
    case 23: // Custom style
    {
        ImGuiStyle& style = ImGui::GetStyle();
        LoadStyleFromXML(ApplicationFile("custom.style.xml").c_str(), style);
    }
    break;
    default: ImGui::StyleColorsNord(); break;
    }

    if (SDL_SetWindowHitTest(ctx->Wimdow, MainWindowHitTest, &g_MainWindowChrome) != 0)
        SDL_Log("Native title-bar dragging/resizing is unavailable: %s", SDL_GetError());

    DesignMode::SetEnabled(ctx, restoreDesignMode);

    // Frame rate limiting
    auto last_time = std::chrono::steady_clock::now();
    const double target_frame_time = 1.0 / 60.0;
    auto lastSettingsSave = std::chrono::steady_clock::now();

    if (ctx->File_path[0] != 0)
    {
        sprintf(ctx->File_label, "J-Link Utils - %s", &ctx->File_path[getFileNameFromPath(ctx->File_path)]);
    }

    // Start the communication thread if Auto Connect is enabled
    if (ctx->auto_connect) {
        StartCommunication(ctx);
    }

    while (!ctx->IsClosing)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // A single input path: the backend consumes SDL's logical mouse
            // coordinates, precise wheel events and its own frame timing.
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Quit event received");
                ctx->IsClosing = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.windowID != SDL_GetWindowID(ctx->Wimdow)) break;
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    ctx->IsClosing = true;
                break;
            case SDL_KEYDOWN:
                break;
            case SDL_KEYUP:
                // Debug output, only in _DEBUG builds
#ifdef _DEBUG
                printf("Physical %s key acting as %s key\r\n",
                    SDL_GetScancodeName(event.key.keysym.scancode),
                    SDL_GetKeyName(event.key.keysym.sym));
#endif
                if (event.key.keysym.mod & KMOD_CTRL) {
                    switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_N:
                        NewFile(ctx);
                        break;
                    case SDL_SCANCODE_O:
                        ctx->openfile = true;
                        break;
                    case SDL_SCANCODE_Q:
                        ctx->closefile = true;
                        break;
                    case SDL_SCANCODE_E:
                        DesignMode::SetEnabled(ctx, !ctx->design_mode);
                        break;
                    default:
                        break;
                    }
                }
                break;
            default:
                break;
            }

        }

        CaptureMainWindowSettings(ctx);
        const auto settingsNow = std::chrono::steady_clock::now();
        if (settingsNow - lastSettingsSave >= std::chrono::seconds(1)) {
            SaveIniSettings(ctx); // Store only touches disk when values changed.
            prefs::Flush();
            lastSettingsSave = settingsNow;
        }
        if (ctx->IsClosing) break;
        const Uint32 flags = SDL_GetWindowFlags(ctx->Wimdow);
        ctx->StateMinimized = (flags & SDL_WINDOW_MINIMIZED) != 0;
        if (ctx->StateMinimized) {
            // Keep pumping restore/close events without rendering a hidden UI.
            SDL_Delay(25);
            last_time = std::chrono::steady_clock::now();
            continue;
        }
        PrepareUiFrame(ctx->Renderer);

        Draw_Window(ctx);
        MenuWindow(ctx);

        if (ctx->show_xml_editor && ctx->CurrentXMLDoc) {
            ShowWidgetEditor(&ctx->show_xml_editor, ctx);
        }

        //static MemoryEditor mem_edit;
        //mem_edit.DrawWindow("Memory Editor", ctx->IOMAP_Data, IOMAP_SIZE);

        if (ctx->ErrRengerer)
            ImGui::OpenPopup(app_language::Label("Error Rengerer", "Ошибка рендерера"), 0);

        if (ImGui::BeginPopupModal(app_language::Label("Error Rengerer", "Ошибка рендерера"), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text(app_language::Tr("Renderer Initialization failed!\nUsing Software mode\n\n", "Не удалось инициализировать рендерер!\nИспользуется программный режим\n\n"));
            ImGui::Separator();
            ImGui::Indent(60.0f);
            if (ImGui::Button(app_language::Label("OK", "ОК"), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); ctx->ErrRengerer = false; }
            ImGui::SetItemDefaultFocus();
            ImGui::EndPopup();
        }

        SDL_SetRenderDrawColor(ctx->Renderer, 0, 0, 0, 0);
        SDL_RenderClear(ctx->Renderer);
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx->Renderer);
        SDL_RenderPresent(ctx->Renderer);

        // Keep a steady frame rate
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_time).count();
        if (elapsed < target_frame_time) {
            SDL_Delay((Uint32)((target_frame_time - elapsed) * 1000));
        }
        // Measure the next frame from the end of the delay, otherwise the sleep is counted twice.
        last_time = std::chrono::steady_clock::now();
    }

    // Stop the communication thread before cleanup
    StopCommunication(ctx);
    SaveIniSettings(ctx);
    prefs::Flush();
    if (!prefs::Error().empty()) {
        const std::string message = app_language::Tr("Your latest settings could not be saved.\n\n", "Не удалось сохранить последние настройки.\n\n") + app_language::Diagnostic(prefs::Error());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, app_language::Tr("J-Link Utils - settings error", "J-Link Utils — ошибка настроек"), message.c_str(), ctx->Wimdow);
    }

    // Widgets own SDL textures: destroy them before their renderer and ImGui.
    ClearAllWindows();

    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplSDLRenderer2_Shutdown();

    if (g_AppIconTexture) SDL_DestroyTexture(g_AppIconTexture);
    SDL_DestroyRenderer(ctx->Renderer);
    SDL_DestroyWindow(ctx->Wimdow);

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    free_context(ctx);

    SDL_Quit();

    return 0;
}