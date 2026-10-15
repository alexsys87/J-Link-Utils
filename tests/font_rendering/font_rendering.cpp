#include "imgui.h"
#include "misc/freetype/imgui_freetype.h"
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
        ImGui::GetStyle().FontScaleDpi = scale;
        ImGui::NewFrame();
        ImGui::Begin("Font regression");
        assert(ImGui::GetFont() == selected);
        // ImGui rounds the final logical size to the nearest pixel.
        assert(std::fabs(ImGui::GetFontSize() - std::floor(selected->LegacySize * scale + 0.5f)) < 0.01f);
        ImFontBaked* baked = ImGui::GetFontBaked();
        // A newly rasterized size, not a scaled copy of the original bitmap.
        assert(std::fabs(baked->Size - ImGui::GetFontSize()) < 0.01f);
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

int main()
{
    TestFonts(false);
    TestFonts(true);
    puts("FreeType fonts: system/fallback, Cyrillic, selection and 100-200% DPI passed");
}
