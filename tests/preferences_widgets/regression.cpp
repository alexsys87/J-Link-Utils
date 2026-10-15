static void WriteFile(const std::string& path, const std::string& text) {
    std::ofstream file(path);
    file << text;
    assert(file.good());
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string dir = argv[1];
    const std::string ini = dir + "/settings.ini";
    assert(prefs::Load(ini));

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
    assert(identityA != PreferenceElementIdentity(copy)); // even exact copies are distinct
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
    std::puts("Widget preferences: identity isolation, XML migration, rebuild/reload passed");
}
