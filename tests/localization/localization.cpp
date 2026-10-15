#include "Localization.h"
#include "LocalizedDiagnostics.h"
#include "Preferences.h"
#include "UserGuide.h"
#include "imgui_internal.h"
#include "WidgetTemplateFactory.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "language_menu.h" // Extracted from production main.cpp by run.py.

static void CheckPairs()
{
    const std::pair<const char*, const char*> pairs[] = {
#include "pairs.h"
    };
    for (const auto& pair : pairs) {
        app_language::Set(app_language::Language::English);
        const char* en = app_language::Label(pair.first, pair.second);
        assert(std::strcmp(en, pair.first) == 0);
        assert(std::strcmp(app_language::Tr(pair.first, pair.second), pair.first) == 0);
        app_language::Set(app_language::Language::Russian);
        assert(std::strcmp(app_language::Tr(pair.first, pair.second), pair.second) == 0);
        const char* ru = app_language::Label(pair.first, pair.second);
        assert(ImHashStr(en) == ImHashStr(ru));
        assert(ru == app_language::Label(pair.first, pair.second));
        if (pair.first[0] != '#') assert(std::strstr(ru, "###"));
    }
    // Cached pointers may be retained by ImGui: later inserts cannot invalidate them.
    const char* retained = app_language::Label("persistent##old", "постоянный");
    const std::string value(retained);
    for (int i = 0; i < 4096; ++i)
        app_language::Label(("id" + std::to_string(i)).c_str(), "новая надпись");
    assert(value == retained);
    for (const auto* label : {"Legacy##hidden", "Visible###stable", "One###two###three", "##invisible"})
        assert(ImHashStr(label) == ImHashStr(app_language::Label(label, "Перевод")));
    assert(app_language::SetFromCode("ru") && app_language::IsRussian());
    assert(app_language::SetFromCode("en") && !app_language::IsRussian());
    assert(!app_language::SetFromCode("bad") && !app_language::IsRussian());
    std::printf("Checked %zu bilingual pairs and stable ImGui IDs\n", sizeof(pairs) / sizeof(pairs[0]));
}

static std::string Templates()
{
    tinyxml2::XMLDocument document;
    for (const auto& def : WidgetTemplateFactory::Widgets())
        document.InsertEndChild(WidgetTemplateFactory::CreateNode(&document, def));
    tinyxml2::XMLPrinter printer;
    document.Print(&printer);
    return printer.CStr();
}

static void CheckGuideAndSchema()
{
    app_language::Set(app_language::Language::English);
    const auto original = Templates();
    int englishCount = 0, russianCount = 0;
    const auto* english = user_guide::EnglishSections(englishCount);
    const auto* russian = user_guide::RussianSections(russianCount);
    assert(englishCount == 13 && russianCount == englishCount);
    app_language::Set(app_language::Language::Russian);
    assert(original == Templates()); // XML, addresses, types and default captions never translate.
    for (int i = 0; i < englishCount; ++i) {
        assert(std::strcmp(english[i].title, russian[i].title) != 0);
        assert(std::strlen(russian[i].text) > 100);
        assert(user_guide::Matches(ImGuiTextFilter("SWD"), russian[0], &english[0]));
    }
    const user_guide::Section sample{"ЁЖ И ПАМЯТЬ", "ПОДКЛЮЧЕНИЕ и русский текст"};
    assert(user_guide::Matches(ImGuiTextFilter("ёж"), sample));
    assert(user_guide::Matches(ImGuiTextFilter("ПАМЯТЬ"), sample));
    assert(!user_guide::Matches(ImGuiTextFilter("-подключение"), sample));
    assert(!user_guide::Matches(ImGuiTextFilter("-ёж,память"), sample));
    assert(user_guide::Matches(ImGuiTextFilter("Quick start"), russian[0], &english[0]));
    assert(user_guide::Matches(ImGuiTextFilter("БЫСТРЫЙ СТАРТ"), english[0], &russian[0]));
}

static void CheckDiagnostics()
{
    const std::string error = "Read failed at 536870912 (-1/4 bytes). Cache invalidated; pending writes discarded.";
    app_language::Set(app_language::Language::English);
    assert(app_language::Diagnostic(error) == error);
    app_language::Set(app_language::Language::Russian);
    const auto localized = app_language::Diagnostic(error);
    assert(localized.find("Ошибка чтения") == 0 && localized.find("536870912 (-1/4") != std::string::npos);
    assert(app_language::Diagnostic("Cannot read settings file: C:\\Папка\\test.ini") ==
        "Не удалось открыть файл настроек для чтения: C:\\Папка\\test.ini");
    assert(app_language::Diagnostic("Target writes are locked (read-only).") != "Target writes are locked (read-only).");
    assert(app_language::Diagnostic("Unknown SDK error 0x1234") == "Unknown SDK error 0x1234");
}

static void DrawFrame(ImGuiID activate = 0)
{
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(640, 420));
    ImGui::Begin("Menu test", nullptr, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
        // Open the real production submenu, then activate a real ImGui MenuItem.
        ImGui::OpenPopup(app_language::Label("Language / Язык", "Язык / Language"));
        if (activate) {
            ImGui::GetCurrentContext()->NavActivateId = activate;
            ImGui::GetCurrentContext()->NavActivateDownId = activate;
        }
        ShowLanguageMenu();
        ImGui::EndMenuBar();
    }
    ImGui::End();
    bool open = true;
    user_guide::Show(&open);
    ImGui::Render();
    assert(ImGui::GetDrawData()->TotalVtxCount > 0);
}

int main(int argc, char** argv)
{
    assert(argc == 2);
    CheckPairs();
    CheckGuideAndSchema();
    CheckDiagnostics();
    assert(prefs::Load(argv[1]));
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(960, 720);
    io.DeltaTime = 1.0f / 60;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontDefault();
    unsigned char* pixels; int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    for (const auto language : {app_language::Language::English, app_language::Language::Russian}) {
        app_language::Set(language);
        DrawFrame(); DrawFrame();
        auto* menu = ImGui::FindWindowByName("###Menu_00");
        assert(menu);
        const char* target = language == app_language::Language::English ? "Русский" : "English";
        DrawFrame(menu->GetID(target));
        assert(app_language::current != language);
        assert(prefs::ReadString("SETTINGS.Language") == app_language::Code());
        auto* guide = ImGui::FindWindowByName("User Guide");
        assert(guide);
        const auto guideId = guide->ID;
        DrawFrame();
        assert(ImGui::FindWindowByName("User Guide")->ID == guideId);
        assert(prefs::Flush() && prefs::Load(argv[1]));
        assert(app_language::SetFromCode(prefs::ReadString("SETTINGS.Language")));
    }
    ImGui::DestroyContext();
    std::puts("Both language menus, live switching, guide, schema, diagnostics and INI tests passed");
}
