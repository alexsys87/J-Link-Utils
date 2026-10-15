#include "UserGuide.h"
#include <imgui_internal.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static void CheckFilters()
{
    int count = 0;
    const auto* sections = user_guide::Sections(count);
    assert(count >= 13);
    for (int i = 0; i < count; ++i) {
        assert(sections[i].title[0] && sections[i].text[0]);
        assert(user_guide::Matches(ImGuiTextFilter(), sections[i]));
        assert(!user_guide::Matches(ImGuiTextFilter("no-such-guide-token-987654321"), sections[i]));
        for (int j = 0; j < i; ++j) assert(std::strcmp(sections[i].title, sections[j].title) != 0);
    }
    const user_guide::Section sample = {"Connection help", "Use Connect; keep READ ONLY until verified."};
    assert(user_guide::Matches(ImGuiTextFilter("connection"), sample));
    assert(user_guide::Matches(ImGuiTextFilter("read only"), sample));
    assert(user_guide::Matches(ImGuiTextFilter("missing,verified"), sample));
    assert(!user_guide::Matches(ImGuiTextFilter("-connection"), sample));
    assert(!user_guide::Matches(ImGuiTextFilter("-verified,connection"), sample));
}

static void CheckRendering(ImVec2 display, float scale)
{
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = display;
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    io.DeltaTime = 1.0f / 60.0f;
    // A legacy CPU font atlas is sufficient for headless layout/draw assertions.
    io.Fonts->AddFontDefault();
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    assert(pixels && width > 0 && height > 0);

    bool open = true;
    ImGuiWindow* content = nullptr;
    int count = 0;
    const auto* sections = user_guide::Sections(count);
    for (int frame = 0; frame < 5; ++frame) {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        user_guide::Show(&open);
        ImGui::Render();
        assert(ImGui::GetDrawData()->Valid);
        assert(ImGui::GetDrawData()->TotalVtxCount > 0);
        auto* window = ImGui::FindWindowByName("User Guide");
        assert(window && window->Size.x <= display.x && window->Size.y <= display.y);
        assert(!(window->Flags & ImGuiWindowFlags_AlwaysAutoResize));
        for (auto* w : ImGui::GetCurrentContext()->Windows)
            if (w->ParentWindow == window && (w->Flags & ImGuiWindowFlags_ChildWindow)) content = w;
        assert(content);
        // Open every actual header, then render all documentation. This also
        // exercises literal printf examples (%d, %) without formatting them.
        for (int i = 0; i < count; ++i)
            content->StateStorage.SetInt(content->GetID(sections[i].title), 1);
        if (frame > 1) assert(content->ScrollMax.y > 0.0f);
    }
    // Exercise Begin(false)'s balanced early-return path on a collapsed guide.
    ImGui::NewFrame();
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Always);
    user_guide::Show(&open);
    ImGui::Render();
    assert(ImGui::FindWindowByName("User Guide")->Collapsed);
    ImGui::DestroyContext();
}

int main()
{
    CheckFilters();
    for (float scale : {1.0f, 1.5f, 2.0f}) {
        CheckRendering(ImVec2(1280, 900), scale);
        CheckRendering(ImVec2(640, 480), scale);
        CheckRendering(ImVec2(360, 280), scale);
    }
    std::puts("User Guide filtering and real-ImGui rendering passed (3 sizes x 3 DPI scales)");
}
