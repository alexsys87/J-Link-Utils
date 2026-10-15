#include "imgui.h"
#include "misc/freetype/imgui_freetype.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include "MainWindowChrome.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <unistd.h>

#define ACCESS access
const char* defaultFontDir = "/usr/share/fonts/";
struct MainContext { int FontSize = 2; };
// Extracted verbatim by run.py: the test exercises the actual app loader.
#include "loader.h"

static void TestFonts(bool fallback)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(800, 600);
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    MainContext ctx;
    if (fallback)
        defaultFontDir = "/nonexistent-font-test-directory/";
    LoadFonts(&ctx);
    assert(io.Fonts->FontLoader == ImGuiFreeType::GetFontLoader());
    assert(io.Fonts->FontLoaderFlags == 0); // Native/full hinting, grayscale AA.
    if (fallback)
        assert(io.Fonts->Fonts.Size == 1);
    else {
        assert(io.Fonts->Fonts.Size >= 10); // DejaVu 11..20px, unchanged ordering.
        assert(io.FontDefault == io.Fonts->Fonts[ctx.FontSize]);
        assert(io.FontDefault->LegacySize == 13.0f);
    }
    ImFont* selected = io.FontDefault ? io.FontDefault : io.Fonts->Fonts[0];
    const int count = io.Fonts->Fonts.Size;
    // Return to 100% as well: changing displays must not compound font scale.
    for (float scale : {1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 1.0f}) {
        io.DisplayFramebufferScale = ImVec2(scale, scale);
        ImGui::NewFrame();
        ImGui::Begin("Font regression");
        assert(ImGui::GetFont() == selected);
        assert(std::fabs(ImGui::GetFontSize() - selected->LegacySize) < 0.01f);
        ImFontBaked* baked = ImGui::GetFontBaked();
        // A newly rasterized size, not a scaled copy of the original bitmap.
        assert(std::fabs(baked->Size - ImGui::GetFontSize()) < 0.01f);
        assert(std::fabs(baked->RasterizerDensity - scale) < 0.01f);
        const ImFontGlyph* glyph = baked->FindGlyphNoFallback('A');
        assert(glyph && glyph->Visible);
        if (!fallback)
            assert(baked->FindGlyphNoFallback(0x0416)); // Cyrillic Ж.
        ImGui::TextUnformatted("J-Link Utils: 0123456789 ABC xyz");
        ImGui::End();
        ImGui::Render();
        assert(io.Fonts->Fonts.Size == count);
        // No GPU in this test. Acknowledge texture requests without uploading;
        // glyph rasterization itself uses the real FreeType implementation.
        for (ImTextureData* texture : io.Fonts->TexList) {
            if (texture->Status == ImTextureStatus_WantCreate) {
                texture->SetTexID(static_cast<ImTextureID>(1));
                texture->SetStatus(ImTextureStatus_OK);
            } else if (texture->Status == ImTextureStatus_WantUpdates)
                texture->SetStatus(ImTextureStatus_OK);
            else if (texture->Status == ImTextureStatus_WantDestroy) {
                texture->SetTexID(ImTextureID_Invalid);
                texture->SetStatus(ImTextureStatus_Destroyed);
            }
        }
    }
    // Exercise all user-selectable fonts, not just the default 13px entry.
    for (ImFont* font : io.Fonts->Fonts) {
        ImFontBaked* baked = font->GetFontBaked(font->LegacySize);
        assert(baked->FindGlyphNoFallback('W'));
    }
    ImGui::DestroyContext();
}

static bool IsRed(SDL_Surface* surface, int x, int y)
{
    Uint8 r, g, b, a;
    const Uint32* row = reinterpret_cast<const Uint32*>(
        static_cast<const Uint8*>(surface->pixels) + y * surface->pitch);
    SDL_GetRGBA(row[x], surface->format, &r, &g, &b, &a);
    return r > 240 && g < 10 && b < 10;
}

static void TestFullDpiRendering()
{
    ConfigureDpiAwareness();
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
    SDL_Window* window = SDL_CreateWindow("DPI test", 0, 0, 640, 480, SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
    assert(window);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    defaultFontDir = "/usr/share/fonts/";
    MainContext ctx;
    LoadFonts(&ctx);
    ImFont* selected = io.FontDefault;
    const ImGuiStyle baseline = ImGui::GetStyle();

    // Same logical window, different physical backing surfaces: the SDL backend
    // must discover the framebuffer ratio. Reuse the ImGui context and fonts so
    // a round trip also detects accumulated layout/style/font scaling.
    for (float scale : {1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 1.25f, 1.0f}) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
            0, (int)(640 * scale), (int)(480 * scale), 32, SDL_PIXELFORMAT_RGBA32);
        assert(surface);
        SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
        assert(renderer);
        assert(ImGui_ImplSDL2_InitForSDLRenderer(window, renderer));
        assert(ImGui_ImplSDLRenderer2_Init(renderer));

        int clicks = 0;
        int titleClicks[3] = {};
        MainWindowChrome chrome;
        // Two warm-up frames let a newly created ImGui window leave its initial
        // hidden/layout frame before the mouse press is delivered.
        for (int phase = 0; phase < 14; ++phase) {
            // Real SDL event -> platform backend -> ImGui logical hit testing.
            SDL_Event event = {};
            event.type = SDL_MOUSEMOTION;
            event.motion.windowID = SDL_GetWindowID(window);
            const int control = phase >= 11 ? 2 : phase >= 8 ? 1 : 0;
            const float titleHeight = 13.0f + ImGui::GetStyle().FramePadding.y * 2.0f;
            const float buttonWidth = titleHeight * 1.6f;
            event.motion.x = phase < 5 ? 100 : (int)(640 - buttonWidth * (2.5f - control));
            event.motion.y = phase < 5 ? 100 : (int)(titleHeight * 0.5f);
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (phase == 2 || phase == 3 || phase == 5 || phase == 6 || phase == 8 || phase == 9 || phase == 11 || phase == 12) {
                event = {};
                event.type = (phase == 2 || phase == 5 || phase == 8 || phase == 11) ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                event.button.windowID = SDL_GetWindowID(window);
                event.button.button = SDL_BUTTON_LEFT;
                ImGui_ImplSDL2_ProcessEvent(&event);
            }
            PrepareUiFrame(renderer); // The actual production frame setup.
            assert(io.DisplaySize.x == 640 && io.DisplaySize.y == 480);
            assert(std::fabs(io.DisplayFramebufferScale.x - scale) < 0.01f);
            assert(std::fabs(io.DisplayFramebufferScale.y - scale) < 0.01f);
            assert(ImGui::GetStyle().FontScaleDpi == 1.0f);
            assert(ImGui::GetStyle().FramePadding.x == baseline.FramePadding.x);
            assert(ImGui::GetStyle().WindowPadding.y == baseline.WindowPadding.y);
            assert(io.FontDefault == selected);
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(640, 480));
            ImGui::Begin("###ChromeTest", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
            const MainWindowAction action = DrawMainWindowTitleBar(window, "DPI widgets with a long title", chrome);
            if (action != MainWindowAction::None) {
                const int index = action == MainWindowAction::Minimize ? 0 : action == MainWindowAction::ToggleMaximize ? 1 : 2;
                ++titleClicks[index];
                assert(phase == 6 || phase == 9 || phase == 12);
                assert(index == control);
            }
            assert(ImGui::GetFontSize() == 13.0f);
            assert(std::fabs(ImGui::GetFontBaked()->RasterizerDensity - scale) < 0.01f);
            const SDL_Point title = {100, 10}, buttons = {620, 10}, menu = {100, 30};
            assert(MainWindowHitTest(window, &title, &chrome) == SDL_HITTEST_DRAGGABLE);
            assert(MainWindowHitTest(window, &buttons, &chrome) == SDL_HITTEST_NORMAL);
            assert(MainWindowHitTest(window, &menu, &chrome) == SDL_HITTEST_NORMAL);
            const SDL_Point edges[] = {{0, 0}, {320, 0}, {639, 0}, {0, 240}, {639, 240}, {0, 479}, {320, 479}, {639, 479}};
            const SDL_HitTestResult expected[] = {SDL_HITTEST_RESIZE_TOPLEFT, SDL_HITTEST_RESIZE_TOP, SDL_HITTEST_RESIZE_TOPRIGHT,
                SDL_HITTEST_RESIZE_LEFT, SDL_HITTEST_RESIZE_RIGHT, SDL_HITTEST_RESIZE_BOTTOMLEFT, SDL_HITTEST_RESIZE_BOTTOM, SDL_HITTEST_RESIZE_BOTTOMRIGHT};
            for (int i = 0; i < 8; ++i)
                assert(MainWindowHitTest(window, &edges[i], &chrome) == expected[i]);
            chrome.blocked = true;
            assert(MainWindowHitTest(window, &title, &chrome) == SDL_HITTEST_NORMAL);
            assert(MainWindowHitTest(window, &edges[0], &chrome) == SDL_HITTEST_NORMAL);
            chrome.blocked = false;
            ImGui::SetCursorPos(ImVec2(40, 80));
            if (ImGui::Button("Logical button", ImVec2(120, 40))) ++clicks;
            assert(ImGui::GetItemRectMin().x == 40 && ImGui::GetItemRectMin().y == 80);
            assert(ImGui::GetItemRectSize().x == 120 && ImGui::GetItemRectSize().y == 40);
            ImGui::TextUnformatted("DPI: 0123456789");
            ImGui::End();

            // Untextured geometry + clipping, through the real renderer backend.
            // This catches missing/double geometry scaling and incorrect scissors.
            ImDrawList* draw = ImGui::GetForegroundDrawList();
            draw->PushClipRect(ImVec2(220, 220), ImVec2(260, 260));
            draw->AddRectFilled(ImVec2(200, 200), ImVec2(280, 280), IM_COL32(255, 0, 0, 255));
            draw->PopClipRect();
            ImGui::Render();
            assert(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) == 0);
            assert(SDL_RenderClear(renderer) == 0);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
            assert(IsRed(surface, (int)(240 * scale), (int)(240 * scale)));
            assert(!IsRed(surface, (int)(210 * scale), (int)(240 * scale)));
            assert(!IsRed(surface, (int)(270 * scale), (int)(240 * scale)));
        }
        assert(clicks == 1);
        for (int count : titleClicks) assert(count == 1);
        bool closing = false;
        ApplyMainWindowAction(window, MainWindowAction::None, closing);
        assert(!closing);
        ApplyMainWindowAction(window, MainWindowAction::Close, closing);
        assert(closing);
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_FreeSurface(surface);
    }
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main()
{
    TestFonts(false);
    TestFonts(true);
    TestFullDpiRendering();
    puts("FreeType fonts: system/fallback, Cyrillic, selection and 100-200% DPI passed");
    puts("ImGui chrome: all eight resize zones, title/buttons/menu separation and control clicks at 100-200% DPI passed");
    puts("Full DPI: logical layout, SDL mouse clicks, framebuffer density and rendered clipping passed");
}
