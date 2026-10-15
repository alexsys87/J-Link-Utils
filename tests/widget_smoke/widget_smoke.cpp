// Smoke test: every widget, loaded from XML with missing, default and hostile property
// values, drawn with real Dear ImGui / ImPlot; the XML editor panel drawn for every element.
// The SDL, J-Link and data-exchange functions below are test doubles.
#include "Main.h"
#include "widget_common.h"
#include "WidgetTemplateFactory.h"
#include "DesignMode.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

// The editor's selection is static: include the implementation to drive it.
#include "../../Editor/XMLEditor.cpp"

namespace wtf = WidgetTemplateFactory;

static const char* g_case = "";   // printed when something fails
static std::string g_window;

extern "C" void SmokeAssertFailed(const char* expr, const char* file, int line) {
    std::fprintf(stderr, "ImGui assertion failed in case %s, window '%s': %s (%s:%d)\n", g_case, g_window.c_str(), expr, file, line);
    std::abort();
}

// ------------------------------------------------------------------
// Target memory and the data-exchange API (DataHelper.cpp in the program)
// ------------------------------------------------------------------
static std::vector<uint8_t> g_memory(IOMAP_SIZE);
static const uint32_t kBase = 0x20000000;

static int g_writes = 0;   // writes to the target, counted while the design mode is tested

static bool Mapped(uint32_t addr, uint32_t size) {
    return addr >= kBase && (uint64_t)addr - kBase + size <= IOMAP_SIZE;
}

template <class T>
static T Access(uint32_t* addr, T* val) {
    T out{};
    if (!addr || !Mapped(*addr, sizeof(T))) return out;
    uint8_t* p = g_memory.data() + (*addr - kBase);
    if (val) { std::memcpy(p, val, sizeof(T)); ++g_writes; }
    std::memcpy(&out, p, sizeof(T));
    return out;
}

extern "C" {
    bool DataReadWriteBoolean(MainContext*, XMLElement*, uint8_t* val, uint32_t* addr) { return Access(addr, val) != 0; }
    uint8_t DataReadWriteU08(MainContext*, XMLElement*, uint8_t* val, uint32_t* addr) { return Access(addr, val); }
    uint16_t DataReadWriteU16(MainContext*, XMLElement*, uint16_t* val, uint32_t* addr) { return Access(addr, val); }
    float DataReadWriteFT32(MainContext*, XMLElement*, float* val, uint32_t* addr) { return Access(addr, val); }
    uint32_t DataReadWriteU32(MainContext*, XMLElement*, uint32_t* val, uint32_t* addr) { return Access(addr, val); }
    uint64_t DataReadWriteU64(MainContext*, XMLElement*, uint64_t* val, uint32_t* addr) { return Access(addr, val); }
    uint16_t DataReadWriteArray(MainContext*, XMLElement*, uint8_t* data, uint32_t* addr, uint16_t size) {
        if (!addr || !Mapped(*addr, size)) return 0;
        if (data) { std::memcpy(g_memory.data() + (*addr - kBase), data, size); ++g_writes; }
        return size;
    }
    void StartCommunication(MainContext*) {}
    void StopCommunication(MainContext*) {}
    void GetSetMaxSpeed(void) {}

    // Images and GIFs: no renderer in the test, so nothing loads
    ImTextureID LoadTextureFromFile(SDL_Renderer*, const char*, int*, int*) { return ImTextureID(); }
    void FreeTexture(ImTextureID) {}
    int LoadGifFrames(SDL_Renderer*, const char*, std::vector<SDL_Texture*>&, std::vector<int>&) { return 0; }
}

static double g_time = 0.0;
double DataTimeNow(void) { return g_time; }
uint32_t DataCompletedCycles(void) { return (uint32_t)(g_time * 1000.0); }
size_t DataReadSamples(MainContext*, const void*, uint32_t addr, uint16_t size, std::vector<DataSample>& out) {
    out.clear();
    for (int i = 0; i < 3; ++i) {
        DataSample s{};
        s.time = g_time - 0.002 * (2 - i);
        if (size <= sizeof(s.bytes) && Mapped(addr, size)) std::memcpy(s.bytes, g_memory.data() + (addr - kBase), size);
        out.push_back(s);
    }
    return out.size();
}
const char* file_open_dialog(const char*) { return nullptr; }
const char* file_save_dialog(const char*) { return nullptr; }

// ------------------------------------------------------------------
// SDL functions the widgets call
// ------------------------------------------------------------------
extern "C" {
    char* SDL_GetBasePath(void) {
        const char* dir = std::getenv("SMOKE_DIR");
        std::string path = std::string(dir ? dir : ".") + "/";
        return strdup(path.c_str());
    }
    void SDL_free(void* p) { std::free(p); }
    void* SDL_malloc(size_t n) { return std::malloc(n); }
    char* SDL_strdup(const char* s) { return strdup(s); }
    size_t SDL_strlen(const char* s) { return std::strlen(s); }
    void* SDL_memset(void* d, int c, size_t n) { return std::memset(d, c, n); }
    size_t SDL_strlcpy(char* d, const char* s, size_t n) {
        const size_t len = std::strlen(s);
        if (n) { const size_t c = len < n - 1 ? len : n - 1; std::memcpy(d, s, c); d[c] = '\0'; }
        return len;
    }
    size_t SDL_strlcat(char* d, const char* s, size_t n) {
        const size_t used = strnlen(d, n);
        return used + SDL_strlcpy(d + used, s, n - used);
    }
    int SDL_strcasecmp(const char* a, const char* b) { return strcasecmp(a, b); }
    int SDL_LockMutex(SDL_mutex*) { return 0; }
    int SDL_UnlockMutex(SDL_mutex*) { return 0; }
    int SDL_CondSignal(SDL_cond*) { return 0; }
    void SDL_DestroyTexture(SDL_Texture*) {}
    int SDL_QueryTexture(SDL_Texture*, Uint32*, int*, int* w, int* h) { if (w) *w = 0; if (h) *h = 0; return -1; }
    void SDL_Log(const char*, ...) {}
}

// ------------------------------------------------------------------
// Frames
// ------------------------------------------------------------------
static void FillMemory(int pattern) {
    static std::mt19937 rng(1234);
    switch (pattern % 4) {
    case 0: std::fill(g_memory.begin(), g_memory.end(), 0); break;
    case 1: std::fill(g_memory.begin(), g_memory.end(), 0xFF); break;
    case 2: // quiet NaN floats everywhere
        for (size_t i = 0; i + 4 <= g_memory.size(); i += 4) {
            const uint32_t nan = 0x7FC00000;
            std::memcpy(&g_memory[i], &nan, 4);
        }
        break;
    default: for (auto& b : g_memory) b = (uint8_t)rng(); break;
    }
}

static void Frame(MainContext* ctx, ImVec2 mouse, bool down, bool editor) {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    g_time += io.DeltaTime;
    io.AddMousePosEvent(mouse.x, mouse.y);
    io.AddMouseButtonEvent(0, down);
    ImGui::NewFrame();
    Draw_Window(ctx);
    if (editor) {
        bool open = true;
        ShowWidgetEditor(&open, ctx);
    }
    ImGui::Render();
}

// Shows each window alone and moves and clicks the mouse over it.
static void DrawWindows(MainContext* ctx) {
    for (size_t w = 0; w < g_Windows.size(); ++w) {
        for (size_t i = 0; i < g_Windows.size(); ++i) g_Windows[i].show = (i == w);
        g_window = g_Windows[w].name;
        ImGuiWindow* window = nullptr;
        for (int f = 0; f < 8; ++f) {
            FillMemory(f);
            ImVec2 mouse(-100.0f, -100.0f);
            if (window) {
                // Sweep a 3x3 grid over the window, pressing on the middle frames
                const float fx = (f % 3 + 0.5f) / 3.0f, fy = ((f / 3) % 3 + 0.5f) / 3.0f;
                mouse = ImVec2(window->Pos.x + window->Size.x * fx, window->Pos.y + window->Size.y * fy);
            }
            Frame(ctx, mouse, f >= 2 && f <= 5, false);
            window = ImGui::FindWindowByName(g_Windows[w].name.c_str());
        }
    }
    for (auto& win : g_Windows) win.show = true;
    g_window = "(editor)";
}

// Design mode: one frame with the given mouse buttons and keys (keys are pressed and released).
static void DesignFrame(MainContext* ctx, ImVec2 mouse, bool left, bool right = false,
    std::initializer_list<ImGuiKey> keys = {}, bool ctrl = false, bool shift = false) {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    g_time += io.DeltaTime;
    io.AddMousePosEvent(mouse.x, mouse.y);
    io.AddMouseButtonEvent(0, left);
    io.AddMouseButtonEvent(1, right);
    io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
    io.AddKeyEvent(ImGuiMod_Shift, shift);
    for (ImGuiKey k : keys) io.AddKeyEvent(k, true);
    ImGui::NewFrame();
    Draw_Window(ctx);
    ImGui::Render();
    for (ImGuiKey k : keys) io.AddKeyEvent(k, false);
}

// Clicks, drags (moving widgets, and resizing them where a handle is hit), right clicks and
// the shortcuts over every window in design mode. Leaf widgets must not write to the target.
static void DrawDesign(MainContext* ctx) {
    g_window = "(design mode)";
    std::fill(g_memory.begin(), g_memory.end(), 0);   // no DoneAddr flag set: nothing to clear
    DesignMode::SetEnabled(ctx, true);
    if (!ctx->design_mode) { std::fprintf(stderr, "case %s: design mode not enabled\n", g_case); std::exit(1); }
    g_writes = 0;
    const uint32_t revision = ctx->xml_revision;
    const ImVec2 off(-100.0f, -100.0f);
    DesignFrame(ctx, off, false);
    const bool bigContainers = std::strncmp(g_case, "inside ", 7) == 0;
    const size_t windowCount = g_Windows.size();
    for (size_t w = 0; w < windowCount && w < g_Windows.size(); ++w) {
        for (size_t i = 0; i < g_Windows.size(); ++i) g_Windows[i].show = (i == w);
        g_window = g_Windows[w].name + " (design)";
        const std::string name = g_Windows[w].name;
        DesignFrame(ctx, off, false);
        for (int point = 0; point < 16; ++point) {
            ImGuiWindow* window = ImGui::FindWindowByName(name.c_str());
            if (!window) break;
            const ImVec2 at(window->Pos.x + window->Size.x * ((point % 4) + 0.5f) / 4.0f,
                            window->Pos.y + window->Size.y * ((point / 4) + 0.5f) / 4.0f);
            DesignFrame(ctx, at, true);                                   // press: select
            DesignFrame(ctx, ImVec2(at.x + 9.0f, at.y + 7.0f), true);     // drag
            DesignFrame(ctx, ImVec2(at.x + 23.0f, at.y + 17.0f), true);
            DesignFrame(ctx, ImVec2(at.x + 23.0f, at.y + 17.0f), false);  // release: commit
            DesignFrame(ctx, ImVec2(at.x + 23.0f, at.y + 17.0f), false, false, { ImGuiKey_RightArrow, ImGuiKey_DownArrow });
            if (point % 5 == 0) {
                DesignFrame(ctx, at, false, true);                        // context menu
                DesignFrame(ctx, at, false, false);
                DesignFrame(ctx, off, false, false, { ImGuiKey_Escape });
            }
            // Duplicating a container that holds every widget (four times in a Grid) would exceed
            // the vertex limit of one ImGui window, so these cases do not duplicate.
            if (point % 7 == 3 && !bigContainers) DesignFrame(ctx, at, false, false, { ImGuiKey_D }, true);
            if (point % 7 == 5) DesignFrame(ctx, at, false, false, { ImGuiKey_Delete });        // delete
            if (point % 4 == 1) DesignFrame(ctx, at, false, false, { ImGuiKey_Z }, true);       // undo
            if (point % 8 == 2) DesignFrame(ctx, at, false, false, { ImGuiKey_Z }, true, true); // redo
        }
    }
    for (auto& win : g_Windows) win.show = true;
    DesignFrame(ctx, off, false);
    if (g_writes != 0) {
        std::fprintf(stderr, "case %s: %d writes to the target in design mode\n", g_case, g_writes);
        std::exit(1);
    }
    // The clicks and drags must have edited something (moves, nudges, deletions)
    if (ctx->xml_revision == revision) {
        std::fprintf(stderr, "case %s: the design mode changed nothing\n", g_case);
        std::exit(1);
    }
    std::printf("design mode: %u changes\n", (unsigned)(ctx->xml_revision - revision));
    DesignMode::SetEnabled(ctx, false);
    DesignMode::Reset();
    g_window = "(editor)";
}

// Draws the editor's property panel for every node of the document.
static void DrawEditor(MainContext* ctx, tinyxml2::XMLNode* node) {
    for (tinyxml2::XMLNode* n = node; n; n = n->NextSibling()) {
        g_selectedNode = n;
        Frame(ctx, ImVec2(-100, -100), false, true);
        if (g_selectedNode == n && n->FirstChild()) DrawEditor(ctx, n->FirstChild());
    }
}

// ------------------------------------------------------------------
// Documents
// ------------------------------------------------------------------
using Values = std::vector<const char*>;

// One window per widget; a text in front of each one (Popup needs a previous item).
static void AddWindow(tinyxml2::XMLDocument& doc, const std::string& title, tinyxml2::XMLElement* widget) {
    tinyxml2::XMLElement* window = doc.NewElement("Window");
    tinyxml2::XMLElement* text = doc.NewElement("Text");
    text->SetText(title.c_str());
    window->InsertEndChild(text);
    tinyxml2::XMLElement* items = doc.NewElement("Items");
    tinyxml2::XMLElement* label = doc.NewElement("Text");
    label->SetText("before");
    items->InsertEndChild(label);
    items->InsertEndChild(widget);
    window->InsertEndChild(items);
    doc.InsertEndChild(window);
}

// Gives every property (and every item property) the value.
static void SetAll(tinyxml2::XMLElement* e, const char* value) {
    const wtf::NodeDef* def = wtf::DefinitionOf(e);
    if (!def) return;
    for (const wtf::PropDef& p : def->props) {
        if (p.attribute) e->SetAttribute(p.name, value);
        else {
            tinyxml2::XMLElement* c = e->FirstChildElement(p.name);
            if (!c) c = wtf::AddProperty(e, p);
            c->SetText(value);
        }
    }
    if (def->listTag) {
        tinyxml2::XMLElement* list = e->FirstChildElement(def->listTag);
        for (tinyxml2::XMLElement* item = list ? list->FirstChildElement() : nullptr; item; item = item->NextSiblingElement())
            SetAll(item, value);
    }
}

static int RunCase(MainContext* ctx, const char* name, tinyxml2::XMLDocument& doc) {
    g_case = name;
    std::printf("case: %s\n", name);
    const std::string path = std::string(std::getenv("SMOKE_DIR") ? std::getenv("SMOKE_DIR") : ".") + "/case.xml";
    if (doc.SaveFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        std::fprintf(stderr, "cannot write %s\n", path.c_str());
        std::exit(1);
    }
    g_Windows.clear();
    ctx->file_loaded = false;
    LoadWindowsFromXML(ctx, path.c_str());
    int windows = 0;
    for (tinyxml2::XMLElement* w = doc.FirstChildElement("Window"); w; w = w->NextSiblingElement("Window")) ++windows;
    if ((int)g_Windows.size() != windows) {
        std::fprintf(stderr, "case %s: %zu windows loaded, %d expected\n", name, g_Windows.size(), windows);
        std::exit(1);
    }
    for (auto& win : g_Windows) {
        if (!win.root) { std::fprintf(stderr, "case %s: window %s is empty\n", name, win.name.c_str()); std::exit(1); }
    }
    DrawWindows(ctx);
    ctx->CurrentXMLDoc = &doc;
    DrawEditor(ctx, doc.FirstChild());
    g_selectedNode = nullptr;
    const int loaded = (int)g_Windows.size();
    DrawDesign(ctx);
    ResetXMLEditorState();
    ClearAllWindows();
    ctx->CurrentXMLDoc = nullptr;
    return loaded;
}

// ------------------------------------------------------------------
// Editor model checks (no drawing)
// ------------------------------------------------------------------
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "check failed: %s (line %d, %s)\n", #cond, __LINE__, g_case); std::exit(1); } } while (0)

static void CheckTemplates() {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* window = wtf::CreateNewWindow(&doc);
    doc.InsertEndChild(window);
    CHECK(wtf::DefinitionOf(window) == &wtf::WindowDef());
    tinyxml2::XMLElement* items = window->FirstChildElement("Items");
    CHECK(items != nullptr);
    for (const wtf::NodeDef& def : wtf::Widgets()) {
        g_case = def.tag;
        tinyxml2::XMLElement* e = wtf::CreateNode(&doc, def);
        items->InsertEndChild(e);
        CHECK(wtf::DefinitionOf(e) == &def || (!def.listed && wtf::DefinitionOf(e) == wtf::FindWidget(def.tag)));
        // Every child of a template is a known property or the list
        for (tinyxml2::XMLElement* c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
            const bool isList = def.listTag && std::strcmp(c->Name(), def.listTag) == 0;
            CHECK(isList || wtf::FindProp(&def, c->Name()) != nullptr);
            if (isList) {
                for (tinyxml2::XMLElement* item = c->FirstChildElement(); item; item = item->NextSiblingElement()) {
                    const wtf::NodeDef* itemDef = wtf::DefinitionOf(item);
                    CHECK(itemDef != nullptr);
                    for (tinyxml2::XMLElement* p = item->FirstChildElement(); p; p = p->NextSiblingElement())
                        CHECK(wtf::FindProp(itemDef, p->Name()) != nullptr || (itemDef->listTag && std::strcmp(p->Name(), itemDef->listTag) == 0));
                }
            }
        }
        // Templates hold every current property; adding the missing ones completes the widget
        for (const wtf::PropDef* p : wtf::MissingProperties(e, def)) {
            CHECK(!p->inTemplate);
            wtf::AddProperty(e, *p);
        }
        CHECK(wtf::MissingProperties(e, def).empty());
        // Properties stay in front of the list
        if (def.listTag) CHECK(e->LastChildElement() == e->FirstChildElement(def.listTag));
        // A widget with items gets a new one
        if (def.itemTag) {
            tinyxml2::XMLElement* item = wtf::CreateItem(&doc, def);
            CHECK(item != nullptr);
            wtf::EnsureList(e, def)->InsertEndChild(item);
            CHECK(wtf::DefinitionOf(item) != nullptr);
        }
    }

    // Insertion of new widgets
    g_case = "InsertWidget";
    tinyxml2::XMLDocument d2;
    CHECK(!wtf::InsertWidget(&d2, nullptr, d2.NewElement("Button")));       // no window yet
    tinyxml2::XMLElement* w2 = wtf::CreateNewWindow(&d2);
    d2.InsertEndChild(w2);
    tinyxml2::XMLElement* list = w2->FirstChildElement("Items");
    tinyxml2::XMLElement* a = wtf::CreateNode(&d2, *wtf::FindWidget("Gauge"));
    CHECK(wtf::InsertWidget(&d2, nullptr, a) && a->Parent() == list);           // nothing selected: first window
    tinyxml2::XMLElement* b = wtf::CreateNode(&d2, *wtf::FindWidget("Gauge"));
    CHECK(wtf::InsertWidget(&d2, a, b) && a->NextSiblingElement() == b);        // same type twice, after the selection
    tinyxml2::XMLElement* group = wtf::CreateNode(&d2, *wtf::FindWidget("Group"));
    CHECK(wtf::InsertWidget(&d2, a->FirstChildElement("Min"), group) && a->NextSiblingElement() == group); // property selected
    tinyxml2::XMLElement* c = wtf::CreateNode(&d2, *wtf::FindWidget("Text"));
    CHECK(wtf::InsertWidget(&d2, group, c) && c->Parent() == group->FirstChildElement("Items"));       // into a container
    tinyxml2::XMLElement* plot = wtf::CreateNode(&d2, *wtf::FindWidget("PlotLines"));
    CHECK(wtf::InsertWidget(&d2, w2, plot) && plot->Parent() == list);         // window selected
    tinyxml2::XMLElement* line = plot->FirstChildElement("Items")->FirstChildElement("Line");
    tinyxml2::XMLElement* d = wtf::CreateNode(&d2, *wtf::FindWidget("Text"));
    CHECK(wtf::InsertWidget(&d2, line, d) && plot->NextSiblingElement() == d);  // plot line selected: after the plot
    CHECK(wtf::DefinitionOf(line) == wtf::FindItem("PlotLines", "Line"));
    CHECK(wtf::DefinitionOf(line->FirstChildElement("Name")) == nullptr);

    // Value helpers
    g_case = "values";
    uint32_t addr = 0;
    CHECK(wtf::ParseAddress("0x20000010", &addr) && addr == 0x20000010);
    CHECK(wtf::ParseAddress("#20000010", &addr) && addr == 0x20000010);
    CHECK(wtf::ParseAddress("536870928", &addr) && addr == 0x20000010);
    CHECK(!wtf::ParseAddress("-1", &addr) && !wtf::ParseAddress("0x1FFFFFFFF", &addr) && !wtf::ParseAddress("12abc", &addr));
    float rgba[4];
    CHECK(wtf::ParseColor("#80FF0000", rgba) && rgba[0] == 1.0f && rgba[1] == 0.0f && rgba[3] > 0.49f && rgba[3] < 0.51f);
    CHECK(!wtf::ParseColor("#FF00", rgba) && !wtf::ParseColor("#GG0000", rgba));
    CHECK(wtf::FormatColor(rgba, true) == "#80FF0000");
    CHECK(wtf::FormatFloat(0.1f) == "0.1" && wtf::FormatFloat(100000.5f) == "100000.5" && wtf::FormatFloat(3.0f) == "3");
    std::printf("Editor model checks passed\n");
}

// Every element of a document is a window, widget, list, item or a property the editor knows.
static int CountUnknown(tinyxml2::XMLElement* e, const wtf::NodeDef* ownerDef) {
    int unknown = 0;
    for (tinyxml2::XMLElement* c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
        const wtf::NodeDef* def = wtf::DefinitionOf(c);
        if (def) { unknown += CountUnknown(c, def); continue; }
        const bool isList = ownerDef && ownerDef->listTag && std::strcmp(c->Name(), ownerDef->listTag) == 0;
        if (isList) { unknown += CountUnknown(c, nullptr); continue; }
        if (ownerDef && wtf::FindProp(ownerDef, c->Name()) && !c->FirstChildElement()) continue;
        if (ownerDef == &wtf::WindowDef() && std::strcmp(c->Name(), "IOMap") == 0) continue;
        std::fprintf(stderr, "unknown element <%s> in <%s> (line %d)\n", c->Name(), e->Name(), c->GetLineNum());
        ++unknown;
    }
    return unknown;
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc > 1 && std::strcmp(argv[1], "--dump-schema") == 0) {
        auto dump = [](const char* tag, const wtf::NodeDef& def) {
            std::printf("%s:", tag);
            for (const wtf::PropDef& p : def.props) std::printf(" %s", p.name);
            std::printf("\n");
        };
        for (const wtf::NodeDef& def : wtf::Widgets()) {
            dump(def.tag, def);
            if (def.itemTag && wtf::FindItem(def.tag, def.itemTag) != wtf::FindWidget(def.itemTag)) {
                const std::string tag = std::string(def.tag) + "/" + def.itemTag;
                dump(tag.c_str(), *wtf::FindItem(def.tag, def.itemTag));
            }
        }
        return 0;
    }

    CheckTemplates();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1600, 1000);
    io.IniFilename = nullptr;
    unsigned char* pixels = nullptr;
    int texW = 0, texH = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &texW, &texH);

    MainContext* ctx = new MainContext();
    ctx->IOMAP_Data = g_memory.data();
    ctx->mcu_memory_base = kBase;
    ctx->StartComunication = true;
    ctx->Sync = true;

    int windows = 0;
    // 0. Every demo widget/editor interaction stays in logical coordinates at
    // different framebuffer densities, including a return to 100%.
    // The separate SDL DPI test verifies actual physical rendering and fonts.
    for (float density : {1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 1.0f}) {
        io.DisplayFramebufferScale = ImVec2(density, density);
        tinyxml2::XMLDocument doc;
        const char* demo = std::getenv("SMOKE_DEMO");
        CHECK(demo && doc.LoadFile(demo) == tinyxml2::XML_SUCCESS);
        int unknown = 0;
        for (tinyxml2::XMLElement* w = doc.FirstChildElement("Window"); w; w = w->NextSiblingElement("Window"))
            unknown += CountUnknown(w, &wtf::WindowDef());
        CHECK(unknown == 0);
        const std::string name = "examples/demo.xml at density " + std::to_string(density);
        windows += RunCase(ctx, name.c_str(), doc);
    }
    io.DisplayFramebufferScale = ImVec2(1, 1);
    // 1. No properties at all
    {
        tinyxml2::XMLDocument doc;
        for (const wtf::NodeDef& def : wtf::Widgets()) AddWindow(doc, std::string("empty ") + def.tag, doc.NewElement(def.tag));
        windows += RunCase(ctx, "no properties", doc);
    }
    // 2. Editor templates
    {
        tinyxml2::XMLDocument doc;
        for (const wtf::NodeDef& def : wtf::Widgets()) AddWindow(doc, std::string("template ") + def.tag, wtf::CreateNode(&doc, def));
        windows += RunCase(ctx, "templates", doc);
    }
    // 3. Hostile values in every property
    const Values hostile = { "0", "-1", "1", "2147483647", "-2147483648", "4294967295", "99999999999",
        "1e30", "-1e30", "nan", "inf", "-inf", "", "%s%n%s%n", "#", "garbage", "0.000001" };
    for (const char* value : hostile) {
        tinyxml2::XMLDocument doc;
        for (const wtf::NodeDef& def : wtf::Widgets()) {
            tinyxml2::XMLElement* e = wtf::CreateNode(&doc, def);
            AddWindow(doc, std::string(def.tag) + " = '" + value + "'", e);
            SetAll(e, value);
        }
        const std::string name = std::string("all properties = '") + value + "'";
        windows += RunCase(ctx, name.c_str(), doc);
    }
    // 4. Every widget inside every container
    for (const wtf::NodeDef& container : wtf::Widgets()) {
        if (!container.listTag || container.itemTag) continue;
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLElement* c = wtf::CreateNode(&doc, container);
        tinyxml2::XMLElement* list = c->FirstChildElement(container.listTag);
        for (const wtf::NodeDef& def : wtf::Widgets()) list->InsertEndChild(wtf::CreateNode(&doc, def));
        AddWindow(doc, std::string("inside ") + container.tag, c);
        const std::string name = std::string("inside ") + container.tag;
        windows += RunCase(ctx, name.c_str(), doc);
    }

    g_Windows.clear();
    delete ctx;
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    std::printf("Widget smoke test passed (%d windows)\n", windows);
    return 0;
}
