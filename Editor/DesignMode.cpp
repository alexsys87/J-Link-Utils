// DesignMode.cpp
// Mouse editing of the widgets in the open document. See DesignMode.h.
//
// Every widget is wrapped in a PlacedWidget, which in design mode measures the widget and
// registers its rectangle here. After all XML windows are drawn, EndFrame() finds the widget
// under the mouse (the innermost one), handles clicks and drags, draws the outlines and the
// Properties window. Leaf widgets are drawn without the mouse, so buttons, sliders and
// inputs cannot write to the target while they are being edited; containers keep it, so tree
// nodes, headers and tabs still open and close.
//
// Elements are identified by their pointers in ctx->CurrentXMLDoc. The widgets only compare
// them; an element is used only after ui_safety::ContainsXMLNode() found it in the document.
#include "DesignMode.h"
#include "XMLEditor.h"
#include "XMLSafety.h"
#include "WidgetTemplateFactory.h"
#include "Preferences.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace wtf = WidgetTemplateFactory;
using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLNode;

namespace DesignMode {

namespace {

// A widget on screen. A widget inside a Grid has one box per cell.
struct Box {
    const XMLElement* key;
    ImGuiWindow* window;     // ImGui window the widget is drawn in
    ImRect rect;             // screen rectangle
    ImRect clip;             // visible part of that window
    int depth;               // nesting level: inner widgets win over their containers
    bool movable;
    float itemWidth;         // ImGui item width while the widget was drawn
};

struct WindowBox {
    const XMLElement* key;
    ImGuiWindow* window;
};

enum class Drag { None, Pending, Move, Resize };

// Resize handles: bit 0 = width, bit 1 = height
enum Handle { HandleNone = 0, HandleRight = 1, HandleBottom = 2, HandleCorner = 3 };

// How a widget's size is stored
enum class Sizing {
    None,
    Size,       // <Width>/<Height>
    Uniform,    // one value that scales the whole widget (<Radius>, <PixelSize>, <Scale>)
    ItemWidth,  // <ItemWidth> of a standard input field
    Bands       // Equalizer: <BandWidth> per band and <Height>
};

struct SizeSpec {
    Sizing mode = Sizing::None;
    const char* prop = nullptr;   // Uniform: the property
    bool x = false, y = false;    // axes that can change
};

struct State {
    bool active = false;
    int depth = 0;
    int blocked = 0;                     // leaf widgets being drawn without the mouse
    ImVec2 savedMouse;
    std::vector<Box> boxes;
    std::vector<WindowBox> windows;
    ImGuiWindow* propertiesWindow = nullptr;

    XMLElement* selected = nullptr;      // a widget or a <Window>

    Drag drag = Drag::None;
    const XMLElement* dragKey = nullptr;
    ImGuiWindow* dragWindow = nullptr;
    ImVec2 dragMouse;                    // mouse at the start
    ImVec2 dragLocal;                    // widget position at the start, window coordinates
    ImVec2 dragPos;                      // current position while moving, window coordinates
    ImRect dragRect;                     // widget rectangle at the start
    float dragItemWidth = 0.0f;
    int handle = HandleNone;
    ImVec2 resizeSize;                   // current size while resizing

    bool snap = true;
    int grid = 8;
    bool outlines = true;

    // Context menu "Add widget here"
    const XMLElement* menuTarget = nullptr;
    ImVec2 menuPos;                      // window coordinates of the right click
    bool openAddHere = false;

    std::string status;                  // result of the last save
    double statusTime = -100.0;
};

State S;

// ------------------------------------------------------------------
// Undo history: the whole document as text after every change
// ------------------------------------------------------------------
struct History {
    std::vector<std::string> states;
    size_t cursor = 0;                   // states[cursor] is the document now
    uint32_t revision = 0;               // ctx->xml_revision of states[cursor]
    const void* mergeA = nullptr;        // last edit, merged with the next one of the same value
    const void* mergeB = nullptr;
    double mergeTime = 0.0;
};

History H;
constexpr size_t kHistoryLimit = 100;

std::string Serialize(XMLDocument* doc) {
    tinyxml2::XMLPrinter printer;
    doc->Print(&printer);
    return printer.CStr();
}

// Records the document as it is when it was changed elsewhere (the XML editor), so that
// undo steps back over that change too instead of losing it.
void HistorySync(MainContext* ctx) {
    if (!H.states.empty() && H.revision == ctx->xml_revision) return;
    if (!H.states.empty()) H.states.resize(H.cursor + 1);
    H.states.push_back(Serialize(ctx->CurrentXMLDoc));
    H.cursor = H.states.size() - 1;
    H.revision = ctx->xml_revision;
    H.mergeA = H.mergeB = nullptr;
}

// Call before changing the document...
void BeginEdit(MainContext* ctx) {
    HistorySync(ctx);
}

// ...and after. Edits of the same value (a, b) within a moment form one undo step, so typing
// a caption is undone at once.
void EndEdit(MainContext* ctx, const void* a = nullptr, const void* b = nullptr) {
    MarkXMLModified(ctx);
    const double now = ImGui::GetTime();
    const bool merge = a && a == H.mergeA && b == H.mergeB && now - H.mergeTime < 1.5 &&
        H.cursor > 0 && H.cursor + 1 == H.states.size();
    std::string state = Serialize(ctx->CurrentXMLDoc);
    if (merge) {
        H.states[H.cursor] = std::move(state);
    }
    else {
        H.states.resize(H.cursor + 1);
        H.states.push_back(std::move(state));
        if (H.states.size() > kHistoryLimit) H.states.erase(H.states.begin());
        H.cursor = H.states.size() - 1;
    }
    H.revision = ctx->xml_revision;
    H.mergeA = a;
    H.mergeB = b;
    H.mergeTime = now;
}

bool CanUndo(const MainContext* ctx) {
    return !H.states.empty() && (H.cursor > 0 || H.revision != ctx->xml_revision);
}

bool CanRedo(const MainContext* ctx) {
    return !H.states.empty() && H.revision == ctx->xml_revision && H.cursor + 1 < H.states.size();
}

// Element indices from the document down to the element, to find the selection again after
// the document was replaced.
std::vector<int> PathOf(const XMLElement* e) {
    std::vector<int> path;
    for (const XMLNode* n = e; n && n->Parent(); n = n->Parent()) {
        int index = 0;
        for (const XMLElement* s = n->Parent()->FirstChildElement(); s && s != n; s = s->NextSiblingElement()) ++index;
        path.push_back(index);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

XMLElement* FindPath(XMLDocument* doc, const std::vector<int>& path) {
    XMLNode* n = doc;
    for (int index : path) {
        XMLElement* c = n->FirstChildElement();
        for (int i = 0; c && i < index; ++i) c = c->NextSiblingElement();
        if (!c) return nullptr;
        n = c;
    }
    return n == doc ? nullptr : n->ToElement();
}

void CancelDrag() {
    S.drag = Drag::None;
    S.dragKey = nullptr;
    S.dragWindow = nullptr;
}

void RestoreHistory(MainContext* ctx) {
    XMLDocument* doc = ctx->CurrentXMLDoc;
    const std::vector<int> path = S.selected ? PathOf(S.selected) : std::vector<int>();
    const std::string& text = H.states[H.cursor];
    doc->Clear();
    doc->Parse(text.c_str(), text.size());
    // Every element was replaced: drop the pointers into the old ones.
    ResetXMLEditorState();
    CancelDrag();
    S.boxes.clear();
    S.windows.clear();
    S.menuTarget = nullptr;
    S.selected = path.empty() ? nullptr : FindPath(doc, path);
    MarkXMLModified(ctx);
    H.revision = ctx->xml_revision;
    H.mergeA = H.mergeB = nullptr;
}

void Undo(MainContext* ctx) {
    if (!CanUndo(ctx)) return;
    HistorySync(ctx);
    if (H.cursor == 0) return;
    --H.cursor;
    RestoreHistory(ctx);
}

void Redo(MainContext* ctx) {
    if (!CanRedo(ctx)) return;
    ++H.cursor;
    RestoreHistory(ctx);
}

// ------------------------------------------------------------------
// Document helpers
// ------------------------------------------------------------------
// The element behind a key, if it is still in the document.
XMLElement* Live(MainContext* ctx, const XMLElement* key) {
    if (!key || !ctx->CurrentXMLDoc || !ui_safety::ContainsXMLNode(ctx->CurrentXMLDoc, key)) return nullptr;
    return const_cast<XMLElement*>(key);
}

bool IsWindow(const XMLElement* e) {
    return e && wtf::DefinitionOf(e) == &wtf::WindowDef();
}

// A widget in a widget list (not a list item such as a plot line)
const wtf::NodeDef* WidgetDef(const XMLElement* e) {
    const wtf::NodeDef* def = wtf::DefinitionOf(e);
    return (def && def != &wtf::WindowDef() && wtf::FindWidget(e->Name()) == def) ? def : nullptr;
}

bool IsSelectable(const XMLElement* e) {
    return IsWindow(e) || WidgetDef(e);
}

// Holds widgets: a container or a window
bool HoldsWidgets(const wtf::NodeDef* def) {
    return def && def->listTag && !def->itemTag;
}

// The widget or window whose list holds the element
XMLElement* OwnerOf(XMLElement* e) {
    XMLNode* list = e ? e->Parent() : nullptr;
    XMLNode* owner = list ? list->Parent() : nullptr;
    XMLElement* o = owner ? owner->ToElement() : nullptr;
    return IsSelectable(o) ? o : nullptr;
}

const char* Caption(const XMLElement* e) {
    for (const char* key : { "Text", "Label", "Name" }) {
        const char* c = ui_safety::XmlChildText(const_cast<XMLElement*>(e), key, nullptr);
        if (c && *c) return c;
    }
    return "";
}

std::string Describe(const XMLElement* e) {
    std::string s = std::string("<") + e->Name() + ">";
    const char* caption = Caption(e);
    if (*caption) s += std::string(" ") + caption;
    return s;
}

const char* PropText(const XMLElement* e, const wtf::PropDef& p, bool* present) {
    if (p.attribute) {
        const char* v = e->Attribute(p.name);
        *present = v != nullptr;
        return v ? v : p.defaultValue;
    }
    const XMLElement* c = e->FirstChildElement(p.name);
    *present = c != nullptr;
    if (!c) return p.defaultValue;
    const char* text = c->GetText();
    return text ? text : "";
}

bool HasProp(const XMLElement* e, const char* name) {
    return e->FirstChildElement(name) != nullptr;
}

float PropFloat(const XMLElement* e, const wtf::NodeDef* def, const char* name) {
    const wtf::PropDef* p = wtf::FindProp(def, name);
    float v = 0.0f;
    const XMLElement* c = e->FirstChildElement(name);
    if (c && c->QueryFloatText(&v) == tinyxml2::XML_SUCCESS && std::isfinite(v)) return v;
    return (p && tinyxml2::XMLUtil::ToFloat(p->defaultValue, &v) && std::isfinite(v)) ? v : 0.0f;
}

void SetPropText(XMLElement* e, const wtf::PropDef& p, const char* value) {
    if (p.attribute) {
        e->SetAttribute(p.name, value);
        return;
    }
    XMLElement* c = e->FirstChildElement(p.name);
    if (!c) c = wtf::AddProperty(e, p);
    c->SetText(value);
}

void SetProp(XMLElement* e, const wtf::NodeDef* def, const char* name, const std::string& value) {
    const wtf::PropDef* p = wtf::FindProp(def, name);
    if (p) SetPropText(e, *p, value.c_str());
}

void RemoveProp(XMLElement* e, const wtf::PropDef& p) {
    if (p.attribute) e->DeleteAttribute(p.name);
    else if (XMLElement* c = e->FirstChildElement(p.name)) e->DeleteChild(c);
}

std::string FormatInt(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", (int)std::lround(v));
    return buf;
}

SizeSpec SizeSpecOf(const wtf::NodeDef* def) {
    SizeSpec spec;
    if (!def || !def->placeable) return spec;
    if (std::strcmp(def->tag, "Equalizer") == 0) {
        spec.mode = Sizing::Bands;
        spec.x = spec.y = true;
        return spec;
    }
    spec.x = wtf::FindProp(def, "Width") != nullptr;
    spec.y = wtf::FindProp(def, "Height") != nullptr;
    if (spec.x || spec.y) {
        spec.mode = Sizing::Size;
        return spec;
    }
    for (const char* prop : { "Radius", "PixelSize", "Scale" }) {
        if (wtf::FindProp(def, prop)) {
            spec.mode = Sizing::Uniform;
            spec.prop = prop;
            spec.x = spec.y = true;
            return spec;
        }
    }
    if (def->itemWidth) {
        spec.mode = Sizing::ItemWidth;
        spec.x = true;
    }
    return spec;
}

// ------------------------------------------------------------------
// Coordinates
// ------------------------------------------------------------------
// Screen position of window coordinate (0, 0), the origin ImGui::SetCursorPos() uses
ImVec2 Origin(const ImGuiWindow* w) {
    return ImVec2(w->Pos.x - w->Scroll.x, w->Pos.y - w->Scroll.y);
}

float Snap(float v) {
    if (!S.snap || ImGui::GetIO().KeyAlt || S.grid <= 1) return std::round(v);
    return std::round(v / (float)S.grid) * (float)S.grid;
}

bool InWindowChain(ImGuiWindow* outer, ImGuiWindow* w) {
    for (; w; w = (w->Flags & ImGuiWindowFlags_ChildWindow) ? w->ParentWindow : nullptr)
        if (w == outer) return true;
    return false;
}

const Box* FirstBox(const XMLElement* key) {
    for (const Box& b : S.boxes)
        if (b.key == key) return &b;
    return nullptr;
}

// Position of a widget in window coordinates: <PosX>/<PosY>, or where the flow put it.
ImVec2 CurrentPos(const XMLElement* e, const wtf::NodeDef* def) {
    if (HasProp(e, "PosX") || HasProp(e, "PosY")) return ImVec2(PropFloat(e, def, "PosX"), PropFloat(e, def, "PosY"));
    if (const Box* b = FirstBox(e)) {
        const ImVec2 o = Origin(b->window);
        return ImVec2(std::round(b->rect.Min.x - o.x), std::round(b->rect.Min.y - o.y));
    }
    return ImVec2(0.0f, 0.0f);
}

// ------------------------------------------------------------------
// Hit testing
// ------------------------------------------------------------------
// Innermost widget under the mouse: in the hovered window first, then in the windows that
// hold it (the empty part of a ChildWindow belongs to the ChildWindow widget).
const Box* BoxUnderMouse() {
    ImGuiContext& gc = *GImGui;
    const ImVec2 m = ImGui::GetIO().MousePos;
    if (!gc.HoveredWindow || !ImGui::IsMousePosValid(&m)) return nullptr;
    for (ImGuiWindow* w = gc.HoveredWindow; w; w = (w->Flags & ImGuiWindowFlags_ChildWindow) ? w->ParentWindow : nullptr) {
        const Box* best = nullptr;
        for (const Box& b : S.boxes) {
            if (b.window != w || !b.clip.Contains(m)) continue;
            ImRect r = b.rect;
            r.Expand(2.0f);   // thin widgets (a separator) stay clickable
            if (!r.Contains(m)) continue;
            if (!best || b.depth >= best->depth) best = &b;   // later ones are drawn on top
        }
        if (best) return best;
    }
    return nullptr;
}

const WindowBox* WindowUnderMouse() {
    ImGuiWindow* hovered = GImGui->HoveredWindow;
    if (!hovered) return nullptr;
    for (const WindowBox& w : S.windows)
        if (w.window == hovered->RootWindow) return &w;
    return nullptr;
}

ImRect HandleRect(const ImRect& r, int handle) {
    const float h = 4.0f;
    ImVec2 c;
    if (handle == HandleRight) c = ImVec2(r.Max.x, (r.Min.y + r.Max.y) * 0.5f);
    else if (handle == HandleBottom) c = ImVec2((r.Min.x + r.Max.x) * 0.5f, r.Max.y);
    else c = r.Max;
    return ImRect(c.x - h, c.y - h, c.x + h, c.y + h);
}

// Tab above the top left corner for moving a widget whose area the mouse cannot grab
// (a tree node or a header, which open on a click).
ImRect GripRect(const ImRect& r) {
    return ImRect(r.Min.x, r.Min.y - 10.0f, r.Min.x + 18.0f, r.Min.y);
}

bool HandleAllowed(const SizeSpec& spec, int handle) {
    if (spec.mode == Sizing::None) return false;
    if (handle == HandleRight) return spec.x;
    if (handle == HandleBottom) return spec.y;
    return spec.x && spec.y;
}

struct HandleHit {
    const Box* box = nullptr;
    int handle = HandleNone;
    bool grip = false;
};

HandleHit HandleUnderMouse() {
    HandleHit hit;
    const wtf::NodeDef* def = S.selected ? WidgetDef(S.selected) : nullptr;
    ImGuiWindow* hovered = GImGui->HoveredWindow;
    const ImVec2 m = ImGui::GetIO().MousePos;
    if (!def || !hovered) return hit;
    const SizeSpec spec = SizeSpecOf(def);
    for (const Box& b : S.boxes) {
        if (b.key != S.selected || !InWindowChain(b.window, hovered)) continue;
        for (int handle : { HandleCorner, HandleRight, HandleBottom }) {
            ImRect r = HandleRect(b.rect, handle);
            r.Expand(2.0f);
            if (HandleAllowed(spec, handle) && r.Contains(m) && b.clip.Contains(m)) {
                hit.box = &b;
                hit.handle = handle;
                return hit;
            }
        }
        if (b.movable && GripRect(b.rect).Contains(m)) {
            hit.box = &b;
            hit.grip = true;
            return hit;
        }
    }
    return hit;
}

// ------------------------------------------------------------------
// Editing operations
// ------------------------------------------------------------------
void Select(MainContext* ctx, const XMLElement* key) {
    XMLElement* e = Live(ctx, key);
    S.selected = IsSelectable(e) ? e : nullptr;
}

void StartDrag(const Box& b, Drag kind, int handle) {
    S.drag = kind;
    S.dragKey = b.key;
    S.dragWindow = b.window;
    S.dragMouse = ImGui::GetIO().MouseClickedPos[0];
    S.dragRect = b.rect;
    S.dragLocal = ImVec2(std::round(b.rect.Min.x - Origin(b.window).x), std::round(b.rect.Min.y - Origin(b.window).y));
    S.dragPos = S.dragLocal;
    S.dragItemWidth = b.itemWidth;
    S.handle = handle;
    S.resizeSize = b.rect.GetSize();
}

void SetPosition(MainContext* ctx, XMLElement* e, ImVec2 pos, const void* merge) {
    const wtf::NodeDef* def = WidgetDef(e);
    if (!def || !def->placeable) return;
    BeginEdit(ctx);
    SetProp(e, def, "PosX", FormatInt(std::max(0.0f, pos.x)));
    SetProp(e, def, "PosY", FormatInt(std::max(0.0f, pos.y)));
    EndEdit(ctx, e, merge);
}

void ResetPosition(MainContext* ctx, XMLElement* e) {
    const wtf::NodeDef* def = WidgetDef(e);
    if (!def || !(HasProp(e, "PosX") || HasProp(e, "PosY"))) return;
    BeginEdit(ctx);
    for (const char* name : { "PosX", "PosY" })
        if (const wtf::PropDef* p = wtf::FindProp(def, name)) RemoveProp(e, *p);
    EndEdit(ctx);
}

// New size of the widget being resized, from the mouse
ImVec2 ResizeTarget(const SizeSpec& spec) {
    const ImVec2 m = ImGui::GetIO().MousePos;
    const ImVec2 o = Origin(S.dragWindow);
    const ImVec2 old = S.dragRect.GetSize();
    ImVec2 size = old;
    if (S.handle & HandleRight) size.x = Snap(S.dragRect.Max.x + m.x - S.dragMouse.x - o.x) + o.x - S.dragRect.Min.x;
    if (S.handle & HandleBottom) size.y = Snap(S.dragRect.Max.y + m.y - S.dragMouse.y - o.y) + o.y - S.dragRect.Min.y;
    size = ImMax(size, ImVec2(4.0f, 4.0f));
    if (spec.mode == Sizing::Uniform) {
        const float rx = old.x > 0.0f ? size.x / old.x : 1.0f;
        const float ry = old.y > 0.0f ? size.y / old.y : 1.0f;
        const float ratio = S.handle == HandleRight ? rx : S.handle == HandleBottom ? ry : std::max(rx, ry);
        size = ImVec2(old.x * ratio, old.y * ratio);
    }
    return size;
}

// Writes the new size into the widget's own size properties.
void CommitResize(MainContext* ctx, XMLElement* e, ImVec2 size) {
    const wtf::NodeDef* def = WidgetDef(e);
    const SizeSpec spec = SizeSpecOf(def);
    const ImVec2 old = S.dragRect.GetSize();
    const float dx = size.x - old.x, dy = size.y - old.y;
    if (spec.mode == Sizing::None || (std::fabs(dx) < 0.5f && std::fabs(dy) < 0.5f)) return;
    BeginEdit(ctx);
    switch (spec.mode) {
    case Sizing::Size:
        // The properties size the drawing, the rectangle may add a label: change them by the
        // difference. Zero or negative means "automatic": take the new rectangle.
        if (spec.x && (S.handle & HandleRight)) {
            const float w = PropFloat(e, def, "Width");
            SetProp(e, def, "Width", FormatInt(std::max(1.0f, w > 0.0f ? w + dx : size.x)));
        }
        if (spec.y && (S.handle & HandleBottom)) {
            const float h = PropFloat(e, def, "Height");
            SetProp(e, def, "Height", FormatInt(std::max(1.0f, h > 0.0f ? h + dy : size.y)));
        }
        break;
    case Sizing::Uniform: {
        const float ratio = old.x > 0.0f ? size.x / old.x : 1.0f;
        const float v = PropFloat(e, def, spec.prop) * ratio;
        if (std::strcmp(spec.prop, "Radius") == 0) SetProp(e, def, spec.prop, FormatInt(std::max(1.0f, v)));
        else SetProp(e, def, spec.prop, wtf::FormatFloat(std::max(0.05f, std::round(v * 100.0f) / 100.0f)));
        break;
    }
    case Sizing::ItemWidth: {
        const float w = PropFloat(e, def, "ItemWidth");
        SetProp(e, def, "ItemWidth", FormatInt(std::max(8.0f, (w > 0.0f ? w : S.dragItemWidth) + dx)));
        break;
    }
    case Sizing::Bands: {
        if (S.handle & HandleRight) {
            const float bands = std::max(1.0f, PropFloat(e, def, "Bands"));
            SetProp(e, def, "BandWidth", FormatInt(std::max(2.0f, PropFloat(e, def, "BandWidth") + dx / bands)));
        }
        if (S.handle & HandleBottom) SetProp(e, def, "Height", FormatInt(std::max(10.0f, PropFloat(e, def, "Height") + dy)));
        break;
    }
    default:
        break;
    }
    EndEdit(ctx);
}

void DeleteSelected(MainContext* ctx) {
    XMLElement* e = S.selected;
    if (!e || !e->Parent()) return;
    XMLElement* owner = OwnerOf(e);
    BeginEdit(ctx);
    e->Parent()->DeleteChild(e);
    S.selected = owner;
    S.boxes.clear();
    EndEdit(ctx);
}

void DuplicateSelected(MainContext* ctx) {
    XMLElement* e = S.selected;
    if (!e || !e->Parent()) return;
    BeginEdit(ctx);
    XMLElement* copy = e->DeepClone(ctx->CurrentXMLDoc)->ToElement();
    e->Parent()->InsertAfterChild(e, copy);
    if (IsWindow(copy)) {
        // Windows with the same title would be one ImGui window
        XMLElement* text = copy->FirstChildElement("Text");
        if (!text) text = wtf::AddProperty(copy, *wtf::FindProp(&wtf::WindowDef(), "Text"));
        const std::string title = std::string(text->GetText() ? text->GetText() : "Window") + " (copy)";
        text->SetText(title.c_str());
        for (const char* name : { "PosX", "PosY" }) {
            if (XMLElement* p = copy->FirstChildElement(name)) {
                float v = 0.0f;
                p->QueryFloatText(&v);
                p->SetText(FormatInt(v + 30.0f).c_str());
            }
        }
    }
    else if (HasProp(copy, "PosX") || HasProp(copy, "PosY")) {
        const wtf::NodeDef* def = WidgetDef(copy);
        const float step = (float)std::max(S.grid, 10);
        SetProp(copy, def, "PosX", FormatInt(PropFloat(copy, def, "PosX") + step));
        SetProp(copy, def, "PosY", FormatInt(PropFloat(copy, def, "PosY") + step));
    }
    S.selected = copy;
    EndEdit(ctx);
}

// Earlier or later in its list; later widgets are drawn on top of earlier ones.
void MoveInList(MainContext* ctx, XMLElement* e, bool later) {
    XMLNode* parent = e ? e->Parent() : nullptr;
    if (!parent) return;
    if (later) {
        XMLElement* next = e->NextSiblingElement();
        if (!next) return;
        BeginEdit(ctx);
        parent->InsertAfterChild(next, e);
    }
    else {
        XMLElement* prev = e->PreviousSiblingElement();
        if (!prev) return;
        BeginEdit(ctx);
        XMLNode* before = prev->PreviousSibling();
        if (before) parent->InsertAfterChild(before, e);
        else parent->InsertFirstChild(e);
    }
    EndEdit(ctx);
}

XMLElement* AddWindow(MainContext* ctx) {
    BeginEdit(ctx);
    XMLElement* w = wtf::CreateNewWindow(ctx->CurrentXMLDoc);
    ctx->CurrentXMLDoc->InsertEndChild(w);
    S.selected = w;
    EndEdit(ctx);
    return w;
}

// Inserts a new widget next to 'target' (into it when it holds widgets); with a position,
// the widget is placed there.
void AddWidget(MainContext* ctx, const wtf::NodeDef& def, XMLElement* target, const ImVec2* pos) {
    XMLDocument* doc = ctx->CurrentXMLDoc;
    BeginEdit(ctx);
    XMLElement* widget = wtf::CreateNode(doc, def);
    if (!wtf::InsertWidget(doc, target, widget)) {
        // No window yet: make one
        XMLElement* window = wtf::CreateNewWindow(doc);
        doc->InsertEndChild(window);
        wtf::EnsureList(window, wtf::WindowDef())->InsertEndChild(widget);
    }
    if (pos && def.placeable) {
        SetProp(widget, &def, "PosX", FormatInt(std::max(0.0f, pos->x)));
        SetProp(widget, &def, "PosY", FormatInt(std::max(0.0f, pos->y)));
    }
    S.selected = widget;
    EndEdit(ctx);
}

void Save(MainContext* ctx) {
    S.statusTime = ImGui::GetTime();
    if (ctx->File_path[0] != '\0' && ctx->CurrentXMLDoc->SaveFile(ctx->File_path) == tinyxml2::XML_SUCCESS) {
        ctx->xml_modified = false;
        S.status = "Saved";
    }
    else {
        S.status = std::string("Cannot save ") + ctx->File_path;
    }
}

// ------------------------------------------------------------------
// Mouse and keyboard
// ------------------------------------------------------------------
void HandleMouse(MainContext* ctx, const Box* hovered, const WindowBox* hoveredWindow) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiContext& gc = *GImGui;

    switch (S.drag) {
    case Drag::None: {
        const HandleHit hit = HandleUnderMouse();
        if (hit.handle == HandleRight) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        else if (hit.handle == HandleBottom) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        else if (hit.handle == HandleCorner) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        else if (hit.grip) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // A tree node, header, tab or scrollbar under the mouse takes the click itself:
            // select, but do not start moving.
            const bool itemHovered = gc.HoveredId != 0 && !gc.HoveredIdIsDisabled;
            if (hit.box && hit.handle) StartDrag(*hit.box, Drag::Resize, hit.handle);
            else if (hit.box && hit.grip) StartDrag(*hit.box, Drag::Move, HandleNone);
            else if (hovered) {
                Select(ctx, hovered->key);
                if (S.selected && hovered->movable && !itemHovered) StartDrag(*hovered, Drag::Pending, HandleNone);
            }
            else if (hoveredWindow) {
                Select(ctx, hoveredWindow->key);
            }
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && (hovered || hoveredWindow)) {
            Select(ctx, hovered ? hovered->key : hoveredWindow->key);
            S.menuTarget = S.selected;
            ImGuiWindow* w = gc.HoveredWindow;
            S.menuPos = ImVec2(std::round(io.MousePos.x - Origin(w).x), std::round(io.MousePos.y - Origin(w).y));
            S.menuPos = ImVec2(Snap(S.menuPos.x), Snap(S.menuPos.y));
            ImGui::OpenPopup("##DesignContext");
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && S.selected && (hovered || hoveredWindow) &&
            S.propertiesWindow)
            ImGui::FocusWindow(S.propertiesWindow);
        break;
    }
    case Drag::Pending:
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            CancelDrag();
            break;
        }
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) break;
        S.drag = Drag::Move;
        [[fallthrough]];
    case Drag::Move: {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        const float minY = S.dragWindow->TitleBarHeight + S.dragWindow->MenuBarHeight;
        S.dragPos.x = std::max(0.0f, Snap(S.dragLocal.x + io.MousePos.x - S.dragMouse.x));
        S.dragPos.y = std::max(minY, Snap(S.dragLocal.y + io.MousePos.y - S.dragMouse.y));
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CancelDrag();
        }
        else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            XMLElement* e = Live(ctx, S.dragKey);
            const ImVec2 pos = S.dragPos;
            CancelDrag();
            if (e) SetPosition(ctx, e, pos, nullptr);
        }
        break;
    }
    case Drag::Resize: {
        const SizeSpec spec = SizeSpecOf(WidgetDef(S.dragKey));
        S.resizeSize = ResizeTarget(spec);
        ImGui::SetMouseCursor(S.handle == HandleRight ? ImGuiMouseCursor_ResizeEW :
            S.handle == HandleBottom ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CancelDrag();
        }
        else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            XMLElement* e = Live(ctx, S.dragKey);
            const ImVec2 size = S.resizeSize;
            if (e) CommitResize(ctx, e, size);
            CancelDrag();
        }
        break;
    }
    }
}

// Shortcuts work while an XML window or the Properties window has the focus.
bool KeysForUs() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || S.drag != Drag::None) return false;
    ImGuiWindow* focused = GImGui->NavWindow;
    if (!focused) return true;
    if (focused->RootWindow == S.propertiesWindow) return true;
    for (const WindowBox& w : S.windows)
        if (w.window == focused->RootWindow) return true;
    return false;
}

void HandleKeys(MainContext* ctx) {
    if (!KeysForUs()) return;
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) { if (io.KeyShift) Redo(ctx); else Undo(ctx); }
        else if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) Redo(ctx);
        else if (ImGui::IsKeyPressed(ImGuiKey_D, false)) DuplicateSelected(ctx);
        else if (ImGui::IsKeyPressed(ImGuiKey_S, false)) Save(ctx);
        return;
    }
    if (!S.selected) return;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        S.selected = nullptr;
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        DeleteSelected(ctx);
        return;
    }
    // Arrows move a widget by a pixel, with Shift by a grid step.
    const wtf::NodeDef* def = WidgetDef(S.selected);
    if (!def || !def->placeable) return;
    const float step = io.KeyShift ? (float)std::max(S.grid, 1) : 1.0f;
    ImVec2 d(0.0f, 0.0f);
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) d.x -= step;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) d.x += step;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) d.y -= step;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) d.y += step;
    if (d.x != 0.0f || d.y != 0.0f) {
        const ImVec2 p = CurrentPos(S.selected, def);
        static const char nudge = 0;
        SetPosition(ctx, S.selected, ImVec2(p.x + d.x, p.y + d.y), &nudge);
    }
}

// ------------------------------------------------------------------
// Drawing
// ------------------------------------------------------------------
const ImU32 kAccent = IM_COL32(255, 160, 20, 255);
const ImU32 kHover = IM_COL32(255, 160, 20, 150);
const ImU32 kFaint = IM_COL32(150, 150, 150, 60);

void DrawHandle(ImDrawList* dl, const ImRect& r) {
    dl->AddRectFilled(r.Min, r.Max, kAccent);
    dl->AddRect(r.Min, r.Max, IM_COL32(0, 0, 0, 200));
}

// Grid lines while dragging, in the window the widget moves in
void DrawGrid(ImGuiWindow* w) {
    if (!S.snap || ImGui::GetIO().KeyAlt || S.grid <= 1) return;
    float step = (float)S.grid;
    while (step < 16.0f) step *= 2.0f;
    const ImRect clip = w->InnerClipRect;
    const ImVec2 o = Origin(w);
    ImDrawList* dl = w->DrawList;
    dl->PushClipRect(clip.Min, clip.Max, false);
    for (float x = o.x + std::ceil((clip.Min.x - o.x) / step) * step; x < clip.Max.x; x += step)
        dl->AddLine(ImVec2(x, clip.Min.y), ImVec2(x, clip.Max.y), IM_COL32(128, 128, 128, 40));
    for (float y = o.y + std::ceil((clip.Min.y - o.y) / step) * step; y < clip.Max.y; y += step)
        dl->AddLine(ImVec2(clip.Min.x, y), ImVec2(clip.Max.x, y), IM_COL32(128, 128, 128, 40));
    dl->PopClipRect();
}

void DrawOverlay(const Box* hovered) {
    const wtf::NodeDef* selectedDef = S.selected ? WidgetDef(S.selected) : nullptr;
    const SizeSpec spec = SizeSpecOf(selectedDef);

    if ((S.drag == Drag::Move || S.drag == Drag::Resize) && S.dragWindow) DrawGrid(S.dragWindow);

    for (const Box& b : S.boxes) {
        const bool selected = b.key == S.selected;
        if (!selected && &b != hovered && !S.outlines) continue;
        ImDrawList* dl = b.window->DrawList;
        dl->PushClipRect(b.clip.Min, b.clip.Max, false);
        if (selected) {
            dl->AddRect(ImVec2(b.rect.Min.x - 1, b.rect.Min.y - 1), ImVec2(b.rect.Max.x + 1, b.rect.Max.y + 1), kAccent, 0.0f, 0, 2.0f);
            if (S.drag == Drag::None) {
                for (int handle : { HandleRight, HandleBottom, HandleCorner })
                    if (HandleAllowed(spec, handle)) DrawHandle(dl, HandleRect(b.rect, handle));
                if (b.movable) {
                    const ImRect g = GripRect(b.rect);
                    dl->AddRectFilled(g.Min, g.Max, kAccent, 2.0f, ImDrawFlags_RoundCornersTop);
                    for (int i = 0; i < 3; ++i) {
                        const float x = g.Min.x + 5.0f + i * 4.0f;
                        dl->AddLine(ImVec2(x, g.Min.y + 3.0f), ImVec2(x, g.Max.y - 2.0f), IM_COL32(0, 0, 0, 160));
                    }
                }
            }
        }
        else if (&b == hovered) {
            dl->AddRect(b.rect.Min, b.rect.Max, kHover, 0.0f, 0, 1.5f);
        }
        else {
            dl->AddRect(b.rect.Min, b.rect.Max, kFaint);
        }
        dl->PopClipRect();
    }

    // The selected window
    if (S.selected && IsWindow(S.selected)) {
        for (const WindowBox& w : S.windows) {
            if (w.key != S.selected) continue;
            ImDrawList* dl = w.window->DrawList;
            const ImRect r = w.window->Rect();
            dl->PushClipRect(r.Min, r.Max, false);
            dl->AddRect(r.Min, r.Max, kAccent, w.window->WindowRounding, 0, 3.0f);
            dl->PopClipRect();
        }
    }

    // Size or position next to the widget being changed
    if (S.drag == Drag::Resize || S.drag == Drag::Move) {
        ImDrawList* dl = S.dragWindow->DrawList;
        const ImRect clip = S.dragWindow->InnerClipRect;
        char label[64];
        ImVec2 at;
        dl->PushClipRect(clip.Min, clip.Max, false);
        if (S.drag == Drag::Resize) {
            const ImVec2 max(S.dragRect.Min.x + S.resizeSize.x, S.dragRect.Min.y + S.resizeSize.y);
            dl->AddRectFilled(S.dragRect.Min, max, IM_COL32(255, 160, 20, 40));
            dl->AddRect(S.dragRect.Min, max, kAccent, 0.0f, 0, 2.0f);
            std::snprintf(label, sizeof(label), "%.0f x %.0f", S.resizeSize.x, S.resizeSize.y);
            at = ImVec2(max.x + 6.0f, max.y - ImGui::GetTextLineHeight());
        }
        else {
            std::snprintf(label, sizeof(label), "x %.0f  y %.0f", S.dragPos.x, S.dragPos.y);
            const ImVec2 o = Origin(S.dragWindow);
            at = ImVec2(o.x + S.dragPos.x, o.y + S.dragPos.y - ImGui::GetTextLineHeight() - 4.0f);
        }
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddRectFilled(ImVec2(at.x - 3, at.y - 1), ImVec2(at.x + ts.x + 3, at.y + ts.y + 1), IM_COL32(0, 0, 0, 190), 3.0f);
        dl->AddText(at, IM_COL32(255, 255, 255, 255), label);
        dl->PopClipRect();
    }
}

// ------------------------------------------------------------------
// Context menu (right click)
// ------------------------------------------------------------------
void ContextMenu(MainContext* ctx) {
    if (ImGui::BeginPopup("##DesignContext")) {
        XMLElement* e = S.selected;
        if (!e) {
            ImGui::CloseCurrentPopup();
        }
        else {
            const wtf::NodeDef* def = WidgetDef(e);
            ImGui::TextDisabled("%s", Describe(e).c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Add widget here...")) S.openAddHere = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", HoldsWidgets(wtf::DefinitionOf(e)) ? "Into this container, at the mouse position"
                                                                              : "After this widget, at the mouse position");
            if (ImGui::MenuItem("Properties") && S.propertiesWindow) ImGui::FocusWindow(S.propertiesWindow);
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) DuplicateSelected(ctx);
            if (ImGui::MenuItem("Move earlier", nullptr, false, e->PreviousSiblingElement() != nullptr)) MoveInList(ctx, e, false);
            if (ImGui::MenuItem("Move later", nullptr, false, e->NextSiblingElement() != nullptr)) MoveInList(ctx, e, true);
            if (def && def->placeable) {
                if (ImGui::MenuItem("Back to flow layout", nullptr, false, HasProp(e, "PosX") || HasProp(e, "PosY")))
                    ResetPosition(ctx, e);
            }
            if (XMLElement* owner = OwnerOf(e)) {
                const std::string label = "Select " + Describe(owner);
                if (ImGui::MenuItem(label.c_str())) S.selected = owner;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del")) DeleteSelected(ctx);
        }
        ImGui::EndPopup();
    }

    if (S.openAddHere) {
        ImGui::OpenPopup("##DesignAddHere");
        S.openAddHere = false;
    }
    if (const wtf::NodeDef* chosen = WidgetPickerPopup("##DesignAddHere")) {
        XMLElement* target = Live(ctx, S.menuTarget);
        if (target) AddWidget(ctx, *chosen, target, &S.menuPos);
    }
}

// ------------------------------------------------------------------
// Properties window
// ------------------------------------------------------------------
// One row per property: present ones, and missing ones that matter (shown with their default
// value and created when edited).
void PropertyTable(MainContext* ctx, XMLElement* e, const wtf::NodeDef* def, bool skipPosition) {
    if (!ImGui::BeginTable("##props", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        return;
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
    for (size_t i = 0; i < def->props.size(); ++i) {
        const wtf::PropDef& p = def->props[i];
        if (skipPosition && (std::strcmp(p.name, "PosX") == 0 || std::strcmp(p.name, "PosY") == 0)) continue;
        bool present = false;
        const char* text = PropText(e, p, &present);
        if (!present && !p.inTemplate && !p.layout) continue;   // older formats only when used

        ImGui::PushID((int)i);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        if (present) ImGui::TextUnformatted(p.name);
        else ImGui::TextDisabled("%s", p.name);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s%s", p.help ? p.help : p.name, present ? "" : "\n(not set: the default is used)");
        }
        ImGui::TableNextColumn();
        // Addresses show their value next to the field
        const float width = p.kind == wtf::PropKind::Address
            ? -(ImGui::CalcTextSize("= 0x00000000").x + ImGui::GetStyle().ItemSpacing.x) : -FLT_MIN;
        std::string value;
        if (EditPropertyValue(p, text, value, width)) {
            BeginEdit(ctx);
            SetPropText(e, p, value.c_str());
            EndEdit(ctx, e, &p);
        }
        ImGui::TableNextColumn();
        if (present) {
            if (ImGui::SmallButton("x")) {
                BeginEdit(ctx);
                RemoveProp(e, p);
                EndEdit(ctx);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove: use the default");
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

// Up, down and delete buttons for an element of a list. Returns true when the list changed.
bool ListButtons(MainContext* ctx, XMLElement* item) {
    bool changed = false;
    ImGui::SameLine();
    ImGui::BeginDisabled(!item->PreviousSiblingElement());
    if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { MoveInList(ctx, item, false); changed = true; }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!item->NextSiblingElement());
    if (!changed && ImGui::ArrowButton("##down", ImGuiDir_Down)) { MoveInList(ctx, item, true); changed = true; }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (!changed && ImGui::SmallButton("x")) {
        BeginEdit(ctx);
        item->Parent()->DeleteChild(item);
        EndEdit(ctx);
        changed = true;
    }
    return changed;
}

// Plot lines, combo entries, zones...: edited in place. Returns true when the list changed.
bool ItemList(MainContext* ctx, XMLElement* e, const wtf::NodeDef* def) {
    const wtf::NodeDef* itemDef = wtf::FindItem(def->tag, def->itemTag);
    if (!itemDef) return false;
    XMLElement* list = e->FirstChildElement(def->listTag);
    int count = 0;
    for (XMLElement* it = list ? list->FirstChildElement(def->itemTag) : nullptr; it; it = it->NextSiblingElement(def->itemTag)) ++count;
    char header[64];
    std::snprintf(header, sizeof(header), "%s (%d)###items", def->itemTag, count);
    if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) return false;
    int index = 0;
    for (XMLElement* it = list ? list->FirstChildElement(def->itemTag) : nullptr; it; it = it->NextSiblingElement(def->itemTag), ++index) {
        ImGui::PushID(it);
        const char* caption = Caption(it);
        const bool open = ImGui::TreeNodeEx("##item", ImGuiTreeNodeFlags_AllowOverlap, "%d. %s", index + 1, *caption ? caption : def->itemTag);
        if (ListButtons(ctx, it)) {
            if (open) ImGui::TreePop();
            ImGui::PopID();
            return true;
        }
        if (open) {
            PropertyTable(ctx, it, itemDef, false);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    const std::string add = std::string("Add ") + def->itemTag;
    if (ImGui::Button(add.c_str())) {
        BeginEdit(ctx);
        XMLElement* item = wtf::CreateItem(ctx->CurrentXMLDoc, *def);
        if (item) wtf::EnsureList(e, *def)->InsertEndChild(item);
        EndEdit(ctx);
        return true;
    }
    return false;
}

// Widgets in a container or window (Tabs of a TabBar). Returns true when the list changed.
bool ChildList(MainContext* ctx, XMLElement* e, const wtf::NodeDef* def) {
    XMLElement* list = e->FirstChildElement(def->listTag);
    int count = 0;
    for (XMLElement* c = list ? list->FirstChildElement() : nullptr; c; c = c->NextSiblingElement()) ++count;
    char header[64];
    std::snprintf(header, sizeof(header), "Widgets (%d)###children", count);
    if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) return false;
    for (XMLElement* c = list ? list->FirstChildElement() : nullptr; c; c = c->NextSiblingElement()) {
        ImGui::PushID(c);
        const std::string label = Describe(c);
        const bool known = WidgetDef(c) != nullptr;
        if (!known) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.3f, 1.0f));
        const bool clicked = ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowOverlap,
            ImVec2(ImGui::GetContentRegionAvail().x - 3.0f * ImGui::GetFrameHeightWithSpacing(), 0.0f));
        if (clicked && known) {
            S.selected = c;
            ImGui::PopID();
            return true;
        }
        if (!known) {
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Not a widget here; the loader ignores it.");
        }
        if (ListButtons(ctx, c)) {
            ImGui::PopID();
            return true;
        }
        ImGui::PopID();
    }
    if (def->itemTag) {
        const std::string add = std::string("Add ") + def->itemTag;
        if (ImGui::Button(add.c_str())) {
            BeginEdit(ctx);
            XMLElement* item = wtf::CreateItem(ctx->CurrentXMLDoc, *def);
            if (item) wtf::EnsureList(e, *def)->InsertEndChild(item);
            S.selected = item;
            EndEdit(ctx);
            return true;
        }
    }
    else {
        if (ImGui::Button("Add widget##child")) ImGui::OpenPopup("##DesignAddChild");
        if (const wtf::NodeDef* chosen = WidgetPickerPopup("##DesignAddChild")) {
            AddWidget(ctx, *chosen, e, nullptr);
            return true;
        }
    }
    return false;
}

void Toolbar(MainContext* ctx) {
    if (ImGui::Button("Add widget")) ImGui::OpenPopup("##DesignAddWidget");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Into the selected container or window, or after the selected widget");
    if (const wtf::NodeDef* chosen = WidgetPickerPopup("##DesignAddWidget")) AddWidget(ctx, *chosen, S.selected, nullptr);
    ImGui::SameLine();
    if (ImGui::Button("Add window")) AddWindow(ctx);
    ImGui::SameLine();
    ImGui::BeginDisabled(!S.selected);
    if (ImGui::Button("Duplicate")) DuplicateSelected(ctx);
    ImGui::SameLine();
    if (ImGui::Button("Delete")) DeleteSelected(ctx);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!CanUndo(ctx));
    if (ImGui::Button("Undo")) Undo(ctx);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Ctrl+Z");
    ImGui::SameLine();
    ImGui::BeginDisabled(!CanRedo(ctx));
    if (ImGui::Button("Redo")) Redo(ctx);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Ctrl+Y");
    ImGui::SameLine();
    if (ImGui::Button("Save")) Save(ctx);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+S: write %s", ctx->File_path);
    ImGui::SameLine();
    if (ImGui::GetTime() - S.statusTime < 3.0) ImGui::TextUnformatted(S.status.c_str());
    else if (ctx->xml_modified) ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "unsaved");

    if (ImGui::Checkbox("Snap to grid", &S.snap)) prefs::WriteBool("Design.Snap", S.snap);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hold Alt while dragging to place freely");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    if (ImGui::InputInt("##grid", &S.grid)) {
        S.grid = std::clamp(S.grid, 1, 200);
        prefs::WriteInt("Design.Grid", S.grid);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Outlines", &S.outlines)) prefs::WriteBool("Design.Outlines", S.outlines);
}

void WidgetPanel(MainContext* ctx, XMLElement* e, const wtf::NodeDef* def) {
    ImGui::Text("%s", Describe(e).c_str());
    if (XMLElement* owner = OwnerOf(e)) {
        ImGui::SameLine();
        const std::string label = "in " + Describe(owner);
        if (ImGui::SmallButton(label.c_str())) {
            S.selected = owner;
            return;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select the parent");
    }

    if (def->placeable && ImGui::CollapsingHeader("Layout", ImGuiTreeNodeFlags_DefaultOpen)) {
        const Box* box = FirstBox(e);
        const bool repeated = box && !box->movable;
        bool absolute = HasProp(e, "PosX") || HasProp(e, "PosY");
        if (repeated) {
            ImGui::TextDisabled("Placed by the Grid around it");
        }
        else if (ImGui::Checkbox("Own position", &absolute)) {
            if (absolute) SetPosition(ctx, e, CurrentPos(e, def), nullptr);
            else ResetPosition(ctx, e);
        }
        if (!repeated && ImGui::IsItemHovered())
            ImGui::SetTooltip("Placed at PosX/PosY in the window instead of after the previous widget.\nDragging a widget sets it.");
        if (!repeated && absolute) {
            int xy[2] = { (int)PropFloat(e, def, "PosX"), (int)PropFloat(e, def, "PosY") };
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt2("##pos", xy)) {
                static const char pos = 0;
                SetPosition(ctx, e, ImVec2((float)xy[0], (float)xy[1]), &pos);
            }
        }
        if (box) {
            const ImVec2 o = Origin(box->window);
            ImGui::TextDisabled("On screen: %.0f x %.0f at %.0f, %.0f", box->rect.GetWidth(), box->rect.GetHeight(),
                box->rect.Min.x - o.x, box->rect.Min.y - o.y);
        }
        else {
            ImGui::TextDisabled("Not visible now");
        }
    }

    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        PropertyTable(ctx, e, def, def->placeable);
    }
    if (!Live(ctx, e)) return;
    if (def->listTag && def->itemTag && !wtf::FindWidget(def->itemTag)) {
        if (ItemList(ctx, e, def)) return;
    }
    else if (def->listTag) {
        if (ChildList(ctx, e, def)) return;
    }
}

void WindowPanel(MainContext* ctx, XMLElement* e) {
    const wtf::NodeDef* def = &wtf::WindowDef();
    ImGui::Text("Window %s", Caption(e));
    ImGui::TextDisabled("Move and resize it with the mouse to store its geometry.");
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) PropertyTable(ctx, e, def, false);
    if (!Live(ctx, e)) return;
    ChildList(ctx, e, def);
}

void ShowProperties(MainContext* ctx) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - 440.0f, vp->WorkPos.y + 60.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 600.0f), ImGuiCond_FirstUseEver);
    bool open = true;
    if (ImGui::Begin("Properties - design mode###DesignProperties", &open)) {
        S.propertiesWindow = ImGui::GetCurrentWindow();
        Toolbar(ctx);
        ImGui::Separator();
        XMLElement* e = S.selected;
        if (!e) {
            ImGui::TextWrapped("Click a widget to select it. Drag it to move it, drag the orange handles to resize it, "
                "right-click for more. Arrows move the selection by a pixel (Shift: a grid step), Del deletes it.");
            ImGui::Spacing();
            ImGui::TextDisabled("Buttons, sliders and inputs do not react in design mode.");
        }
        else if (IsWindow(e)) {
            WindowPanel(ctx, e);
        }
        else if (const wtf::NodeDef* def = WidgetDef(e)) {
            WidgetPanel(ctx, e, def);
        }
    }
    else {
        S.propertiesWindow = ImGui::GetCurrentWindow();
    }
    ImGui::End();
    if (!open) SetEnabled(ctx, false);
}

} // namespace

// ------------------------------------------------------------------
// Public functions
// ------------------------------------------------------------------
bool CanEnable(const MainContext* ctx) {
    return ctx && ctx->CurrentXMLDoc != nullptr;
}

void SetEnabled(MainContext* ctx, bool enabled) {
    enabled = enabled && CanEnable(ctx);
    if (enabled == ctx->design_mode) return;
    ctx->design_mode = enabled;
    CancelDrag();
    S.selected = nullptr;
    S.menuTarget = nullptr;
    // An input field that has the keyboard must not keep it while its widget is disabled.
    if (ImGui::GetCurrentContext()) ImGui::ClearActiveID();
}

void Reset() {
    CancelDrag();
    S.selected = nullptr;
    S.menuTarget = nullptr;
    S.boxes.clear();
    S.windows.clear();
    H = History();
}

void BeginFrame(bool active) {
    static bool preferencesLoaded = false;
    if (!preferencesLoaded) {
        S.snap = prefs::ReadBool("Design.Snap", true);
        S.grid = prefs::ReadInt("Design.Grid", 8, 1, 200);
        S.outlines = prefs::ReadBool("Design.Outlines", true);
        preferencesLoaded = true;
    }
    S.active = active;
    S.depth = 0;
    S.blocked = 0;
    S.boxes.clear();
    S.windows.clear();
    if (!active) CancelDrag();
}

bool IsActive() {
    return S.active;
}

void BeginWindow(const XMLElement* key) {
    if (!S.active) return;
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    // A click on the content selects a widget instead of moving the window (the title bar still does).
    w->BgClickFlags = ImGuiWindowBgClickFlags_None;
    if (key) S.windows.push_back({ key, w });
}

void CommitWindowGeometry(MainContext* ctx, const XMLElement* key, ImVec2 pos, ImVec2 size) {
    XMLElement* e = Live(ctx, key);
    if (!e || !IsWindow(e)) return;
    const wtf::NodeDef* def = &wtf::WindowDef();
    BeginEdit(ctx);
    SetProp(e, def, "PosX", FormatInt(pos.x));
    SetProp(e, def, "PosY", FormatInt(pos.y));
    SetProp(e, def, "Width", FormatInt(size.x));
    SetProp(e, def, "Height", FormatInt(size.y));
    static const char geometry = 0;
    EndEdit(ctx, e, &geometry);
}

void EndFrame(MainContext* ctx) {
    if (!ctx->design_mode || !ctx->CurrentXMLDoc) return;
    if (S.selected && !(ui_safety::ContainsXMLNode(ctx->CurrentXMLDoc, S.selected) && IsSelectable(S.selected)))
        S.selected = nullptr;
    if (S.drag != Drag::None && !Live(ctx, S.dragKey)) CancelDrag();

    const Box* hovered = nullptr;
    const WindowBox* hoveredWindow = nullptr;
    if (S.active) {
        hovered = BoxUnderMouse();
        hoveredWindow = WindowUnderMouse();
        HandleMouse(ctx, hovered, hoveredWindow);
    }
    HandleKeys(ctx);   // may replace the document (undo): the boxes are cleared then
    if (hovered && S.boxes.empty()) hovered = nullptr;
    DrawOverlay(hovered);
    ContextMenu(ctx);
    ShowProperties(ctx);
}

bool DraggedPosition(const XMLElement* key, ImVec2* pos) {
    if (S.drag != Drag::Move || key != S.dragKey) return false;
    *pos = S.dragPos;
    return true;
}

void BeginWidget(bool container) {
    ++S.depth;
    if (container) return;
    // Leaf widgets are drawn without the mouse and out of the keyboard navigation: a click
    // selects them instead of pressing a button or writing a value to the target.
    if (S.blocked++ == 0) {
        ImGuiIO& io = ImGui::GetIO();
        S.savedMouse = io.MousePos;
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }
    ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop | ImGuiItemFlags_NoNav, true);
}

void EndWidget(bool container, const XMLElement* key, ImGuiWindow* window, const ImRect& rect, bool movable, float itemWidth) {
    const int depth = S.depth--;
    if (!container) {
        ImGui::PopItemFlag();
        if (--S.blocked == 0) ImGui::GetIO().MousePos = S.savedMouse;
    }
    if (!key || !window || window->SkipItems) return;
    ImRect r = rect;
    if (!(r.GetHeight() >= 1.0f)) return;   // SameLine, Spacing...: nothing to show
    // Separators and columns do not report their width: they span the window.
    if (r.GetWidth() < 1.0f) r.Max.x = std::max(r.Min.x + 1.0f, window->WorkRect.Max.x);
    S.boxes.push_back({ key, window, r, window->InnerClipRect, depth, movable, itemWidth });
}

} // namespace DesignMode
