static void WriteFile(const std::string& path, const std::string& text) {
    std::ofstream file(path);
    file << text;
    assert(file.good());
}

static void TestSemanticDashboardIdentity(const std::string& dir, const std::string& ini) {
    g_PreferenceDocument = dir + "/groups.xml";
    XMLDocument doc;
    assert(doc.Parse("<Window><Text>Groups</Text><Items>"
        "<Group><Items><PlotLines><Text>A</Text><Address>#20000000</Address></PlotLines></Items></Group>"
        "<Group><Items><PlotLines><Text>B</Text><Address>#20000004</Address></PlotLines></Items></Group>"
        "</Items></Window>") == XML_SUCCESS);
    auto* items = doc.FirstChildElement()->FirstChildElement("Items");
    auto* groupA = items->FirstChildElement();
    auto* groupB = groupA->NextSiblingElement();
    auto* plotA = groupA->FirstChildElement("Items")->FirstChildElement();
    const auto identity = PreferenceElementIdentity(plotA);
    assert(!identity.empty());
    auto* heading = doc.NewElement("Text");
    heading->SetText("Legacy heading");
    items->InsertFirstChild(heading);
    assert(PreferenceElementIdentity(plotA) == identity);
    heading->SetText("Changed heading");
    assert(PreferenceElementIdentity(plotA) == identity);
    items->DeleteChild(heading);
    assert(PreferenceElementIdentity(plotA) == identity);
    const auto key = prefs::ScopedKey(dir + "/groups.xml", "PlotLines", identity);
    prefs::WriteDouble(key + ".Frequency", 17.5);
    prefs::WriteBool(key + ".Realtime", false);
    items->InsertFirstChild(groupB);
    assert(PreferenceElementIdentity(plotA) == identity);
    items->DeleteChild(groupB);
    assert(PreferenceElementIdentity(plotA) == identity);
    PlotRestore restored;
    restored.SetPreferenceKey(prefs::ScopedKey(dir + "/groups.xml", "PlotLines", PreferenceElementIdentity(plotA)));
    assert(restored.freq == 17.5f && !restored.state.pause_check);

    // Matching descendants of anonymous Groups are just as ambiguous as direct
    // siblings. A previously saved singleton must not donate prefs to its copy.
    auto* groupCopy = groupA->DeepClone(&doc)->ToElement();
    items->InsertEndChild(groupCopy);
    auto* plotCopy = groupCopy->FirstChildElement("Items")->FirstChildElement();
    assert(PreferenceElementIdentity(plotA).empty() && PreferenceElementIdentity(plotCopy).empty());
    items->DeleteChild(groupA);
    assert(prefs::Flush() && prefs::Load(ini));
    assert(PreferenceElementIdentity(plotCopy).empty());

    g_PreferenceDocument = dir + "/matching.xml";
    XMLDocument matching;
    assert(matching.Parse("<Window><Text>Matching</Text><Items>"
        "<PlotLines><Text>Signal</Text><Address>#20000000</Address><Scale>1</Scale></PlotLines>"
        "</Items></Window>") == XML_SUCCESS);
    items = matching.FirstChildElement()->FirstChildElement("Items");
    auto* first = items->FirstChildElement();
    const auto singletonIdentity = PreferenceElementIdentity(first);
    assert(!singletonIdentity.empty());
    const auto singletonKey = prefs::ScopedKey(g_PreferenceDocument, "PlotLines", singletonIdentity);
    prefs::WriteDouble(singletonKey + ".Frequency", 41);
    prefs::WriteBool(singletonKey + ".Realtime", false);
    auto* second = first->DeepClone(&matching)->ToElement();
    second->FirstChildElement("Scale")->SetText(100);
    items->InsertEndChild(second);
    assert(PreferenceElementIdentity(first).empty() && PreferenceElementIdentity(second).empty());
    PlotRestore ambiguous;
    ambiguous.SetPreferenceKey(PreferenceElementIdentity(second));
    assert(ambiguous.m_preferenceKey.empty() && ambiguous.freq == 3);
    // The persisted ambiguity marker prevents reuse of the singleton key, even
    // after removing the original and reloading settings in a fresh process.
    items->DeleteChild(first);
    assert(prefs::Flush() && prefs::Load(ini));
    const auto survivor = PreferenceElementIdentity(second);
    assert(survivor.empty());
    PlotRestore afterDelete;
    afterDelete.SetPreferenceKey(survivor);
    assert(afterDelete.freq == 3 && afterDelete.state.pause_check);
    // A genuinely distinct semantic identity can persist normally again.
    second->FirstChildElement("Text")->SetText("Distinct signal");
    assert(!PreferenceElementIdentity(second).empty());

    g_PreferenceDocument = dir + "/windows.xml";
    XMLDocument windows;
    assert(windows.Parse("<Window><Text>Same</Text></Window>") == XML_SUCCESS);
    auto* window = windows.FirstChildElement();
    const auto windowIdentity = PreferenceElementIdentity(window);
    assert(!windowIdentity.empty());
    prefs::WriteBool(prefs::ScopedKey(g_PreferenceDocument, "Windows", windowIdentity), false);
    auto* windowCopy = window->DeepClone(&windows)->ToElement();
    windows.InsertEndChild(windowCopy);
    assert(PreferenceElementIdentity(window).empty());
    assert(PreferenceElementIdentity(windowCopy).empty());
    windows.DeleteChild(window);
    assert(prefs::Flush() && prefs::Load(ini));
    assert(PreferenceElementIdentity(windowCopy).empty()); // cannot inherit the deleted window's hidden state
    g_WindowPreferenceKeys = {""};
    g_Windows = {{false}};
    SaveDashboardWindowVisibility();
    assert(!prefs::Dirty()); // ambiguous visibility cannot write a shared empty key
}

static void TestWatchExportAndDuplicateIds(const std::string& dir, const std::string& ini) {
    const auto watchPath = dir + "/export-watch.xml";
    const auto exportPath = dir + "/snapshot.xml";
    std::strcpy(WatchWidget::s_xmlPath, watchPath.c_str());
    const std::string legacy = "<Watch><Items><WatchLine><Text>Export</Text><Address>16</Address>"
        "<DataType>3</DataType><Value>777</Value></WatchLine></Items></Watch>";
    WriteFile(watchPath, legacy);
    WatchWidget::LoadFromXML(watchPath.c_str());
    auto& rows = WatchWidget::s_rows;
    const auto legacyIdentity = WatchWidget::RowPreferenceIdentity(rows[0]);
    rows[0].value = 999999;
    WatchWidget::SaveToXML(exportPath.c_str(), true);
    assert(rows[0].preferenceId.empty() && WatchWidget::RowPreferenceIdentity(rows[0]) == legacyIdentity);
    XMLDocument unchanged, snapshot;
    assert(unchanged.LoadFile(watchPath.c_str()) == XML_SUCCESS);
    assert(snapshot.LoadFile(exportPath.c_str()) == XML_SUCCESS);
    auto* originalRow = unchanged.FirstChildElement()->FirstChildElement()->FirstChildElement();
    auto* exportedRow = snapshot.FirstChildElement()->FirstChildElement()->FirstChildElement();
    assert(!originalRow->Attribute("PreferenceId") && !exportedRow->Attribute("PreferenceId"));
    assert(std::string(originalRow->FirstChildElement("Value")->GetText()) == "777");
    assert(std::string(exportedRow->FirstChildElement("Value")->GetText()) == "999999");
    rows[0].bitMode = true;
    WatchWidget::SaveRowPreferences(rows[0]);
    assert(prefs::Flush() && prefs::Load(ini));
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(rows[0].bitMode && rows[0].preferenceId.empty()); // export did not orphan later UI writes

    // Normal saves (but not exports or failed saves) still migrate legacy IDs.
    WatchWidget::SaveToXML((dir + "/missing-directory/watch.xml").c_str());
    assert(rows[0].preferenceId.empty() && !prefs::Dirty());
    WatchWidget::SaveToXML(watchPath.c_str());
    const auto migratedId = rows[0].preferenceId;
    assert(!migratedId.empty());
    WatchWidget::SaveToXML(exportPath.c_str(), true);
    assert(rows[0].preferenceId == migratedId);
    assert(prefs::Flush() && prefs::Load(ini));
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(rows[0].preferenceId == migratedId && rows[0].bitMode);

    WriteFile(watchPath, "<Watch><Items>"
        "<WatchLine PreferenceId='copied'><Text>A</Text><Address>16</Address><DataType>3</DataType></WatchLine>"
        "<WatchLine PreferenceId='copied'><Text>B</Text><Address>32</Address><DataType>3</DataType></WatchLine>"
        "</Items></Watch>");
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(!rows[0].bitMode && !rows[1].hexMode);
    rows[0].bitMode = true;
    rows[1].hexMode = true;
    WatchWidget::SaveToXML(exportPath.c_str(), true);
    assert(rows[0].preferenceId == "copied" && rows[1].preferenceId == "copied");
    WatchWidget::SaveToXML(watchPath.c_str());
    const auto firstId = rows[0].preferenceId, secondId = rows[1].preferenceId;
    assert(firstId != "copied" && secondId != "copied" && firstId != secondId);
    std::swap(rows[0], rows[1]);
    WatchWidget::SaveToXML(watchPath.c_str());
    assert(prefs::Flush() && prefs::Load(ini));
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(rows[0].preferenceId == secondId && rows[0].hexMode && !rows[0].bitMode);
    assert(rows[1].preferenceId == firstId && rows[1].bitMode && !rows[1].hexMode);
    assert(!prefs::Dirty());
}

int main(int argc, char** argv) {
    assert(argc == 2);
    app_language::SetFromCode(std::getenv("TEST_LANGUAGE") ? std::getenv("TEST_LANGUAGE") : "en");
    const std::string dir = argv[1];
    const std::string ini = dir + "/settings.ini";
    assert(prefs::Load(ini));
    g_PreferenceDocument = dir + "/a.xml";

    XMLDocument doc;
    assert(doc.Parse("<Window><Text>Dashboard</Text><Items>"
        "<PlotLines><Text>Same title</Text><Address>#20000000</Address><Freq>1</Freq></PlotLines>"
        "<PlotLines><Text>Same title</Text><Address>#20000004</Address><Freq>1</Freq></PlotLines>"
        "<PlotLines><Text>Multi</Text><Items><Line><Name>A</Name><Address>#20000008</Address></Line></Items></PlotLines>"
        "</Items></Window>") == XML_SUCCESS);
    XMLElement* window = doc.FirstChildElement();
    XMLElement* items = window->FirstChildElement("Items");
    XMLElement* a = items->FirstChildElement();
    XMLElement* b = a->NextSiblingElement();
    XMLElement* multi = b->NextSiblingElement();
    const auto identityA = PreferenceElementIdentity(a);
    const auto identityB = PreferenceElementIdentity(b);
    assert(identityA != identityB); // same title, different address
    const auto multiBefore = PreferenceElementIdentity(multi);
    multi->FirstChildElement("Items")->FirstChildElement()->FirstChildElement("Address")->SetText("#2000000C");
    assert(multiBefore != PreferenceElementIdentity(multi));
    items->InsertFirstChild(b); // reordering unrelated siblings does not reassign prefs
    assert(identityA == PreferenceElementIdentity(a));
    assert(identityB == PreferenceElementIdentity(b));
    a->FirstChildElement("Freq")->SetText(99);
    a->InsertEndChild(doc.NewElement("PosX"))->ToElement()->SetText(75);
    assert(identityA == PreferenceElementIdentity(a)); // XML edits/rebuild do not reset runtime prefs
    XMLElement* copy = a->DeepClone(&doc)->ToElement();
    items->InsertEndChild(copy);
    assert(PreferenceElementIdentity(a).empty() && PreferenceElementIdentity(copy).empty()); // no positional guessing
    assert(PreferenceIdentityPart("a:b") + PreferenceIdentityPart("c") !=
        PreferenceIdentityPart("a") + PreferenceIdentityPart("b:c"));

    const auto plotA = prefs::ScopedKey(dir + "/a.xml", "PlotLines", identityA);
    const auto plotB = prefs::ScopedKey(dir + "/a.xml", "PlotLines", identityB);
    const auto otherDashboard = prefs::ScopedKey(dir + "/b.xml", "PlotLines", identityA);
    assert(plotA != plotB && plotA != otherDashboard);
    prefs::WriteDouble(plotA + ".Frequency", 17.5);
    prefs::WriteDouble(plotA + ".TimeWindow", 23);
    prefs::WriteBool(plotA + ".Realtime", false);
    const auto visibility = prefs::ScopedKey(dir + "/a.xml", "Windows", PreferenceElementIdentity(window));
    const auto otherVisibility = prefs::ScopedKey(dir + "/b.xml", "Windows", PreferenceElementIdentity(window));
    g_WindowPreferenceKeys = {visibility};
    g_Windows = {{false}};
    SaveDashboardWindowVisibility();
    assert(prefs::Flush());
    SaveDashboardWindowVisibility();
    assert(!prefs::Dirty()); // drawing an unchanged dashboard does not stage writes
    assert(prefs::ReadBool(otherVisibility, true));

    PlotRestore restored;
    restored.SetPreferenceKey(plotA);
    assert(restored.freq == 17.5f && restored.state.timeWindow == 23);
    assert(!restored.state.pause_check && restored.state.paused && restored.resized == 1);
    PlotRestore isolated;
    isolated.SetPreferenceKey(plotB);
    assert(isolated.freq == 3 && isolated.state.pause_check);
    isolated.SetPreferenceKey(otherDashboard);
    assert(isolated.freq == 3);

    const auto watchPath = dir + "/watch.xml";
    std::strcpy(WatchWidget::s_xmlPath, watchPath.c_str());
    const std::string rowA = "<WatchLine><Text>A</Text><Address>16</Address><DataType>3</DataType><Value>777</Value></WatchLine>";
    const std::string rowB = "<WatchLine><Text>B</Text><Address>32</Address><DataType>3</DataType></WatchLine>";
    auto writeWatch = [&](const std::string& rows) { WriteFile(watchPath, "<Watch><Items>" + rows + "</Items></Watch>"); };
    writeWatch(rowA + rowB);
    WatchWidget::LoadFromXML(watchPath.c_str());
    auto& rows = WatchWidget::s_rows;
    assert(rows.size() == 2 && !rows[0].bitMode && !rows[0].hexMode);
    const auto legacyIdentity = WatchWidget::RowPreferenceIdentity(rows[0]);
    rows[0].value = 999999;
    rows[0].enabled = !rows[0].enabled;
    assert(legacyIdentity == WatchWidget::RowPreferenceIdentity(rows[0]));
    rows[0].bitMode = true;
    WatchWidget::SaveRowPreferences(rows[0]);
    assert(prefs::Flush());
    // A UI preference write must not rewrite the XML or snapshot the current target.
    XMLDocument untouched;
    assert(untouched.LoadFile(watchPath.c_str()) == XML_SUCCESS);
    assert(std::string(untouched.FirstChildElement()->FirstChildElement()->FirstChildElement()->FirstChildElement("Value")->GetText()) == "777");
    writeWatch(rowB + rowA);
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(!rows[0].bitMode && rows[1].bitMode); // reordered legacy XML, no index keys
    writeWatch(rowB + "<WatchLine><Text>A</Text><Address>48</Address><DataType>3</DataType></WatchLine>");
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(!rows[1].bitMode); // import with reused row number/title but different address
    writeWatch(rowA + rowA);
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(!rows[0].bitMode && !rows[1].bitMode); // ambiguous legacy duplicates are not guessed
    rows[0].bitMode = true;
    rows[1].hexMode = true;
    WatchWidget::SaveToXML(watchPath.c_str()); // normal config-save adds optional stable IDs
    const auto firstId = rows[0].preferenceId;
    assert(!firstId.empty() && firstId != rows[1].preferenceId);
    std::swap(rows[0], rows[1]);
    WatchWidget::SaveToXML(watchPath.c_str());
    assert(prefs::Flush());
    assert(prefs::Load(ini)); // process-style settings reload, then new widget/row objects
    WatchWidget::LoadFromXML(watchPath.c_str());
    assert(rows[1].preferenceId == firstId && rows[1].bitMode && !rows[1].hexMode);
    assert(rows[0].hexMode && !rows[0].bitMode);
    PlotRestore afterRestart;
    afterRestart.SetPreferenceKey(plotA);
    assert(afterRestart.freq == 17.5f && !afterRestart.state.pause_check);
    assert(!prefs::ReadBool(visibility, true) && prefs::ReadBool(otherVisibility, true));
    assert(!prefs::Dirty()); // loading prefs/rebuilding does not dirty the settings
    TestSemanticDashboardIdentity(dir, ini);
    TestWatchExportAndDuplicateIds(dir, ini);
    std::puts("Widget preferences: semantic identity, ambiguity isolation, safe export, ID repair, rebuild/reload passed");
}
