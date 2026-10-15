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

static MainWindowChrome g_MainWindowChrome;

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
    if (fontIndex >= 0 && fontIndex < io.Fonts->Fonts.Size) {
        io.FontDefault = io.Fonts->Fonts[fontIndex];
    }
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

    if (ImGui::MenuItem("New", "Ctrl+N")) {
        NewFile(ctx);
    }
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
        OpenFile(ctx);
    }
    if (ImGui::MenuItem("Close", "Ctrl+Q")) {
        CloseFile(ctx);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", "Alt+F4")) {
        ctx->IsClosing = true;
    }
}

// ------------------------------------------------------------------
// Helpers to save/load the style as XML
// ------------------------------------------------------------------
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

    return doc.SaveFile(filename) == tinyxml2::XML_SUCCESS;
}

bool LoadStyleFromXML(const char* filename, ImGuiStyle& style)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename) != tinyxml2::XML_SUCCESS)
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
void ShowStyleEditor(bool* p_open)
{
    // Reference style for the per-color Save/Revert buttons; the menu actions below update it too.
    static ImGuiStyle ref_saved_style = ImGui::GetStyle();

    ImGui::Begin("Style Editor", p_open, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save to XML..."))
            {
                const char* filename = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE,
                    "XML files\0*.xml\0All files\0*.*\0", NULL, "style.xml");
                if (filename)
                {
                    if (!SaveStyleToXML(filename, ImGui::GetStyle()))
                        ImGui::OpenPopup("Error##SaveStyle");
                }
            }
            if (ImGui::MenuItem("Load from XML..."))
            {
                const char* filename = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN,
                    "XML files\0*.xml\0All files\0*.*\0", NULL, NULL);
                if (filename)
                {
                    if (LoadStyleFromXML(filename, ImGui::GetStyle()))
                    {
                        // Update the saved reference style
                        ref_saved_style = ImGui::GetStyle();
                    }
                    else
                        ImGui::OpenPopup("Error##LoadStyle");
                }
            }
            if (ImGui::MenuItem("Save as Custom"))
            {
                if (!SaveStyleToXML("custom.style.xml", ImGui::GetStyle()))
                    ImGui::OpenPopup("Error##SaveStyle");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset to Default"))
            {
                ImGui::GetStyle() = ImGuiStyle();
                ref_saved_style = ImGui::GetStyle();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Error popups
    if (ImGui::BeginPopupModal("Error##SaveStyle", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Failed to save style to XML file.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Error##LoadStyle", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Failed to load style from XML file.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiStyle* ref = &ref_saved_style;

        ImGui::Spacing();

        if (ImGui::BeginTabItem("Sizes"))
        {
            ImGui::Text("Main");
            ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
            ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
            ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
            ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
            ImGui::Text("Borders");
            ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::Text("Rounding");
            ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::Text("Alignment");
            ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
            ImGui::Combo("WindowMenuButtonPosition", (int*)&style.WindowMenuButtonPosition, "Left\0Right\0");
            ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::Text("Safe Area Padding");
            ImGui::SliderFloat2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Colors"))
        {
            static ImGuiTextFilter filter;
            filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

            static int alpha_mode = 0; // 0=Opaque, 1=Alpha, 2=Both
            ImGui::RadioButton("Opaque", &alpha_mode, 0); ImGui::SameLine();
            ImGui::RadioButton("Alpha", &alpha_mode, 1); ImGui::SameLine();
            ImGui::RadioButton("Both", &alpha_mode, 2); ImGui::SameLine();

            ImGui::BeginChild("##colors", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
            ImGui::PushItemWidth(-160);
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                const char* name = ImGui::GetStyleColorName(i);
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
                    if (ImGui::Button("Save")) ref->Colors[i] = style.Colors[i];
                    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                    if (ImGui::Button("Revert")) style.Colors[i] = ref->Colors[i];
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
        ImGui::MenuItem(g_Windows[i].name.c_str(), "", &g_Windows[i].show);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Top Most", "", &ctx->TopMost)) { SaveIniSettings(ctx); }

    int font = FontSelector("Fonts##Selector");
    if (font != -1)
    {
        ctx->FontSize = font;
        SaveIniSettings(ctx);
    }

    ImGui::Separator();

    // Style list (includes Adobe Spectrum, Doug Binks Dark, Enemy Mouse, LED Synth Master)
    if (ImGui::Combo("Style", &ctx->Style,
        "Classic\0Blue\0Light\0Soft\0Blender\0Cyberpunk\0Nord\0Black\0Green\0Red\0Embrace\0Dracula\0"
        "Visual Studio\0One Dark\0Light Classic\0Cherry\0Nord Light\0Matrix\0High Contrast\0"
        "Adobe Spectrum\0Doug Binks Dark\0Enemy Mouse\0LED Synth Master\0Custom\0"))
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
            if (!LoadStyleFromXML("custom.style.xml", style))
            {
                // if the file is missing, nothing changes
            }
        }
        break;
        default: ImGui::StyleColorsNord(); break;
        }
        SaveIniSettings(ctx);
    }
    ImGui::MenuItem("Style Editor", NULL, &ctx->show_style_editor);

    ImGui::Separator();
    if (ImGui::MenuItem("XML Editor", NULL, &ctx->show_xml_editor)) {
        // nothing to do
    }
    bool design = ctx->design_mode;
    if (ImGui::MenuItem("Design Mode", "Ctrl+E", &design, DesignMode::CanEnable(ctx))) {
        DesignMode::SetEnabled(ctx, design);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", DesignMode::CanEnable(ctx)
            ? "Edit the widgets with the mouse: select, move, resize, and change their properties"
            : "Open or create an XML file first");
    }
}

static const char* ConnectionStateText(JLinkConnectionState state)
{
    switch (state) {
    case JLinkConnectionState::Connecting: return "Connecting";
    case JLinkConnectionState::Connected: return "Connected";
    case JLinkConnectionState::Reconnecting: return "Reconnecting";
    case JLinkConnectionState::Failed: return "Connection failed";
    case JLinkConnectionState::Stopping: return "Disconnecting";
    default: return "Disconnected";
    }
}

static void ShowConnectionStatus(bool details)
{
    const auto status = GetJLinkConnectionStatus();
    const ImVec4 color = status.state == JLinkConnectionState::Connected ? ImVec4(0, 1, 0, 1) :
        status.state == JLinkConnectionState::Failed ? ImVec4(1, 0.35f, 0.25f, 1) : ImVec4(1, 0.75f, 0.2f, 1);
    ImGui::TextColored(color, "J-Link: %s%s", ConnectionStateText(status.state),
        status.lastError.empty() && !status.writeMayHaveCompleted ? "" : " (!)");
    if (ImGui::IsItemHovered() && (!status.lastError.empty() || status.writeMayHaveCompleted))
        ImGui::SetTooltip("Last error: %s%s", status.lastError.c_str(), status.writeMayHaveCompleted ?
            "\nA write may have partially completed. Check the target; pending writes were discarded." : "");
    if (details) {
        if (status.attempt) ImGui::Text("Attempt %u of 5", status.attempt);
        if (status.retryInMs) ImGui::Text("Next attempt in %.1f s", status.retryInMs / 1000.0);
        if (!status.lastError.empty()) {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 500.0f);
            ImGui::TextWrapped("Last error: %s", status.lastError.c_str());
            ImGui::PopTextWrapPos();
        }
        if (status.writeMayHaveCompleted)
            ImGui::TextWrapped("A write may have partially completed. Check the target before writing again; pending writes were discarded.");
        if (status.state == JLinkConnectionState::Failed)
            ImGui::TextDisabled("Automatic attempts stopped. Fix the cause and press Connect to retry.");
    }
}

void ShowMenuSettings(MainContext* ctx)
{
    // --- MCU base address (hex input) ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("MCU Memory address 0x");
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
        ImGui::SetTooltip("Base address of the MCU memory region (e.g., 0x20000000)");

    // --- Auto Connect checkbox ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Auto Connect");
    ImGui::SameLine();
    if (ImGui::Checkbox("##autoconnect", &ctx->auto_connect)) {
        SaveIniSettings(ctx);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Automatically connect to J-Link on program start.");

    // Cancel remains available during connection and retry backoff.
    ImGui::SameLine();
    if (IsCommunicationRequested()) {
        if (ImGui::Button(ctx->StartComunication ? "Disconnect" : "Cancel connection"))
            StopCommunication(ctx);
    } else if (ImGui::Button("Connect")) {
        StartCommunication(ctx);
    }
    ShowConnectionStatus(true);

    // --- J-Link device selection with filterable list ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("J-Link device");
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
    ImGui::TextDisabled("(current)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Currently selected device. Select a new one from the list below.");

    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    ImGui::InputText("##filter", filter_buf, sizeof(filter_buf));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        filter_buf[0] = '\0';
        strcpy(prev_filter, "");
        filtered_indices.clear();
        for (size_t i = 0; i < all_devices.size(); ++i)
            filtered_indices.push_back((int)i);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%d found)", (int)filtered_indices.size());

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
        ImGui::SetTooltip("Click to select target device for J-Link");
}

void ShowUserGuide(bool* p_open)
{
    if (!ImGui::Begin("User Guide", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGui::BulletText("Double-click on title bar to collapse window.");
    ImGui::BulletText("Click and drag on lower right corner to resize window.");
    ImGui::BulletText("Click and drag on any empty space to move window.");
    ImGui::BulletText("TAB/SHIFT+TAB to cycle through keyboard editable fields.");
    ImGui::BulletText("CTRL+Click on a slider or drag box to input value as text.");
    if (io.FontAllowUserScaling)
        ImGui::BulletText("CTRL+Mouse Wheel to zoom window contents.");
    ImGui::BulletText("Mouse Wheel to scroll.");
    ImGui::BulletText("While editing text:");
    ImGui::Indent();
    ImGui::BulletText("Hold SHIFT or use mouse to select text.");
    ImGui::BulletText("CTRL+Left/Right to word jump.");
    ImGui::BulletText("CTRL+A or double-click to select all.");
    ImGui::BulletText("CTRL+X,CTRL+C,CTRL+V to use clipboard.");
    ImGui::BulletText("CTRL+Z,CTRL+Y to undo/redo.");
    ImGui::BulletText("ESCAPE to revert.");
    ImGui::BulletText("You can apply arithmetic operators +,*,/ on numerical values.");
    ImGui::Unindent();
    ImGui::End();
}

void ShowAbout(bool* p_open)
{
    if (!ImGui::Begin("About J-Link Utils", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    if (g_AppIconTexture) {
        ImGui::Image((ImTextureID)(intptr_t)g_AppIconTexture, ImVec2(64.0f, 64.0f));
        ImGui::SameLine();
    }
    ImGui::BeginGroup();
    ImGui::Text("J-Link Utils");
    ImGui::Text("Version 2.0");
    ImGui::EndGroup();
    ImGui::Separator();
    ImGui::Text("A utility application for debugging and configuring microcontrollers via J-Link.");
    ImGui::Text("Built with SDL2, ImGui, and ImPlot.");
    ImGui::Text(" ");
    ImGui::Text("Libraries used:");
    ImGui::BulletText("SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    ImGui::BulletText("Dear ImGui %s", ImGui::GetVersion());
    ImGui::BulletText("ImPlot v0.18 WIP");
    ImGui::BulletText("tinyxml2 %d.%d.%d", TINYXML2_MAJOR_VERSION, TINYXML2_MINOR_VERSION, TINYXML2_PATCH_VERSION);
    ImGui::BulletText("lfqueue");
    ImGui::Text(" ");
    ImGui::Text("Build date: %s %s", __DATE__, __TIME__);
#ifdef _MSC_VER
    ImGui::Text("Compiler: Microsoft Visual C++ %d", _MSC_VER);
#elif defined(__GNUC__)
    ImGui::Text("Compiler: GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(__clang__)
    ImGui::Text("Compiler: Clang %d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#else
    ImGui::Text("Compiler: unknown");
#endif
    ImGui::Text("Platform: %s",
#ifdef _WIN64
        "Windows 64-bit"
#elif _WIN32
        "Windows 32-bit"
#elif __linux__
        "Linux"
#elif __APPLE__
        "macOS"
#else
        "Unknown"
#endif
    );

    ImGui::Separator();
    if (ImGui::Button("Copy to clipboard"))
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

            if (ImGui::BeginMenu("File"))
            {
                ShowMenuFile(ctx);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows"))
            {
                ShowMenuWindow(ctx);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings"))
            {
                ShowMenuSettings(ctx);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::MenuItem("User Guide..", NULL, &show_app_userguide);
                ImGui::MenuItem("About..", NULL, &show_app_about);
                ImGui::EndMenu();
            }

            ShowConnectionStatus(false);

            if (ctx->openfile)
                if (OpenFile(ctx)) ctx->openfile = false;

            if (ctx->closefile)
                if (CloseFile(ctx)) ctx->closefile = false;

            ImGui::EndMenuBar();
        }

        if (g_Windows.empty()) {
            ImGui::TextDisabled("%s", ctx->File_path[0] != '\0'
                ? "No windows loaded. Check the XML file."
                : "No XML file loaded. Use File > New or File > Open.");
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
    if (JLINKARMDLL_LIB_LoadLibrary() != 0) {
#ifdef _WIN64
        const char* message = "J-Link runtime could not be loaded. Place a licensed, compatible JLink_x64.dll next to this executable, or install the matching SEGGER J-Link software. No DLL is bundled.";
#else
        const char* message = "J-Link runtime could not be loaded. Place a licensed, compatible JLinkARM.dll next to this executable, or install the matching SEGGER J-Link software. No DLL is bundled.";
#endif
        fprintf(stderr, "%s\n", message);
        if (!checkRuntimeOnly)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "J-Link runtime missing", message, nullptr);
        return 2;
    }
    if (checkRuntimeOnly) return 0; // Loader check only, not a hardware test.
#endif
    int window_flag = 0;

    MainContext* ctx = allocate_context();
    if (!ctx) return 1;

    LoadIniSettings(ctx);

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
    SDL_Rect usableBounds;
    if (SDL_GetDisplayUsableBounds(0, &usableBounds) == 0) {
        if (ctx->Win_width > usableBounds.w) ctx->Win_width = usableBounds.w;
        if (ctx->Win_height > usableBounds.h) ctx->Win_height = usableBounds.h;
    }

    if (ctx->TopMost)
        window_flag = SDL_WINDOW_ALWAYS_ON_TOP;

    ConfigureMainWindowChrome();
    ctx->Wimdow = SDL_CreateWindow("J-Link Utils", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ctx->Win_width, ctx->Win_height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | window_flag);

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

    g_AppIconTexture = CreateAppIconTexture(ctx->Renderer);

    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui::GetIO().IniFilename = "window.ini";

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
        LoadStyleFromXML("custom.style.xml", style);
    }
    break;
    default: ImGui::StyleColorsNord(); break;
    }

    if (SDL_SetWindowHitTest(ctx->Wimdow, MainWindowHitTest, &g_MainWindowChrome) != 0)
        SDL_Log("Native title-bar dragging/resizing is unavailable: %s", SDL_GetError());

    // Frame rate limiting
    auto last_time = std::chrono::steady_clock::now();
    const double target_frame_time = 1.0 / 60.0;

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

        if (ctx->IsClosing) break;
        const Uint32 flags = SDL_GetWindowFlags(ctx->Wimdow);
        ctx->StateMinimized = (flags & SDL_WINDOW_MINIMIZED) != 0;
        if (ctx->StateMinimized) {
            // Keep pumping restore/close events without rendering a hidden UI.
            SDL_Delay(25);
            last_time = std::chrono::steady_clock::now();
            continue;
        }
        // Preserve normal logical dimensions, not a maximized monitor's size.
        if (!(flags & SDL_WINDOW_MAXIMIZED))
            SDL_GetWindowSize(ctx->Wimdow, &ctx->Win_width, &ctx->Win_height);
        PrepareUiFrame(ctx->Renderer);

        Draw_Window(ctx);
        MenuWindow(ctx);

        if (ctx->show_xml_editor && ctx->CurrentXMLDoc) {
            ShowWidgetEditor(&ctx->show_xml_editor, ctx);
        }

        //static MemoryEditor mem_edit;
        //mem_edit.DrawWindow("Memory Editor", ctx->IOMAP_Data, IOMAP_SIZE);

        if (ctx->ErrRengerer)
            ImGui::OpenPopup("Error Rengerer", 0);

        if (ImGui::BeginPopupModal("Error Rengerer", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Renderer Initialization failed!\nUsing Software mode\n\n");
            ImGui::Separator();
            ImGui::Indent(60.0f);
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); ctx->ErrRengerer = false; }
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