#pragma once

#include <imgui.h>

// Kept independent of the transport and Windows headers so the complete guide
// can be exercised with real ImGui without a J-Link or a native window.
namespace user_guide {

struct Section {
    const char* title;
    const char* text;
};

inline const Section* Sections(int& count)
{
    static const Section sections[] = {
        {"1. Quick start", R"guide(J-Link Utils monitors and changes MCU memory through a SEGGER J-Link using SWD. XML files describe the windows and widgets; you can build a dashboard without changing C++ code.

1. Install the SEGGER J-Link software/driver. Use the DLL matching the application: JLink_x64.dll for x64, JLinkARM.dll for x86. The proprietary DLL is not included in the application package.
2. Connect and power the target; check SWD wiring and the reference voltage. In Settings, filter the J-Link device list and select the exact MCU.
3. Set MCU Memory address to the start of the region you want to inspect (default 0x20000000). All widget addresses must fit inside the mapped region.
4. Press Connect and wait for Connected. Use File > Open to load an XML dashboard, or File > New to create one. You can edit layouts while disconnected.
5. Start in READ ONLY. Check addresses, stored types and target safety before explicitly enabling writes in Settings.

The current transport selects SWD at 8000 kHz; this is not a selectable UI setting. Only use targets and wiring suitable for that speed.)guide"},
        {"2. Files, windows and appearance", R"guide(File > New (Ctrl+N) chooses a filename and opens the editor with an empty document. Add a Window and widgets, then Save to write the file. File > Open (Ctrl+O) replaces the current dashboard; File > Close (Ctrl+Q) closes it. File > Quit or Alt+F4 exits. Save your edits before opening, creating or closing another document, or exiting: there is no unsaved-changes confirmation.

The first command-line argument can be an XML path, for example:
"J-Link Utils.exe" "C:\dashboards\motor.xml"

Windows lists the loaded dashboard windows; toggle an entry to hide or show it. The same menu contains Top Most, the font selector, Style, Style Editor, XML Editor and Design Mode.

Style Editor can Save to XML, Load from XML, Save as Custom and Reset to Default. Save as Custom writes custom.style.xml; choose Custom in the Style list to use it. Style files are not dashboard files.

Connection preferences, dashboard path, normal main-window geometry/maximized state, editor visibility, Design Mode/grid options, Watch display options, dashboard window visibility and PlotLines options are stored in settings.ini. ImGui layout is stored in window.ini. These files and custom.style.xml use the executable directory, independent of the shortcut's working directory. Move any legacy files from another working directory beside the EXE before launching; keep backups. Top Most takes effect immediately. Target write permission is never saved.

Preferences save about once a second when changed and at normal exit. A failed settings save leaves the previous INI intact and displays an error with a retry button. The executable directory must be writable. Invalid numeric preferences use safe defaults; an unavailable saved font falls back to an installed font. XML layouts, styles and grid sizes use logical units at 96 DPI; Windows display scaling is applied automatically. Do not multiply XML sizes for each monitor.

The main title bar has minimize, maximize/restore and close buttons. Drag the title bar to move the application and its edges/corners to resize it. Minimize pauses rendering, not the J-Link communication thread.)guide"},
        {"3. Connection, addressing and recovery", R"guide(Settings > MCU Memory address maps up to 1 MiB from the base address, limited by the end of the 32-bit address space. Widget addresses are physical MCU addresses, NOT offsets. With base 0x20000000, offset 0x10 is written as Address 0x20000010. The complete value or array must fit in the mapping.

XML address fields accept 0x20000010, #20000010 or decimal. A leading zero without x is interpreted as octal. Use # or 0x for hexadecimal. The Settings address inputs display hexadecimal digits.

Changing the base or selected MCU stops communication and discards pending requests; press Connect again. Auto Connect only requests a connection at application startup; it does not enable writes.

The main menu shows Connecting, Connected, Reconnecting or Connection failed. Settings adds the last error, attempt count and retry delay. There are at most five attempts, with delays of 0.5, 1, 2 and 4 seconds. After Connection failed, fix the cause and press Connect. Cancel connection / Disconnect stops retries.

After an I/O failure, cached values, plot samples and pending writes are cleared. Subscriptions resume after reconnection, but writes remain locked. A failed write may have partially completed: inspect the target before writing again. Failed or queued writes are NOT replayed. Zeros in a cleared cache are not measured values; always check connection status.

Disconnect can cancel retry waits immediately, but must wait for an already-running synchronous J-Link DLL call. A blocked DLL call has no guaranteed cancellation deadline.)guide"},
        {"4. Target write protection - read this before writing", R"guide(READ ONLY is the default, including after Auto Connect. It blocks all target writes: controls, Watch, memory editors and automatic DoneAddr acknowledgements. Reading still works. This software lock does not replace hardware safety measures.

While locked, open Settings > Target write protection:
- Optionally enable Limit writable addresses (draft), enter First address and Last address (inclusive), then press Apply address limit. Until Apply, the form is only a draft. Every byte of a write must fit in the active range.
- Optional value limits by address can restrict up to 32 non-overlapping values. Enter the physical address, actual stored type and decimal Minimum/Maximum; press Add value limit. Supported types are U8/U16/U32/U64, I8/I16/I32/I64 and Float32/Float64. Remove deletes a rule. Values outside a rule are rejected, not clamped.
- Rules require the exact address and full stored width. Partial or overlapping byte edits are rejected. Match the actual MCU representation: use Float32 for float bits even if a widget transports them as U32. Protected floating-point values cannot be NaN or infinity.

Connect, press Enable writes..., verify the device, mapping and active limits, acknowledge the risk, then press Enable for this connection. This permits ALL allowed writes for this connection, not just one click. Addresses without value rules still follow the global lock, address range and MCU mapping.

Lock writes in the main menu or Lock target writes now in Settings revokes permission and discards unsent writes. A write already submitted to the SDK cannot be recalled. Disconnect, I/O failure, reconnect or protection changes lock writes again. Rejected writes are not kept for later execution.

Permission and protection rules last only for the current program run; rules survive reconnect within that run, permission does not. Settings shows the last protection error and blocked-request count. Recheck them if a control appears ineffective.)guide"},
        {"5. XML structure and common properties", R"guide(The loader expects top-level Window elements, not an extra enclosing root element. Multiple Window siblings are supported. Each Window has Text (its name) and Items (its widgets). Container widgets use their own Items. XML tag names are case-sensitive; unknown widgets are skipped.

Minimal read-only dashboard (the address is an example, not a verified variable on your target):
<Window>
  <Text>My dashboard</Text>
  <Items>
    <Text><Text>Live counter</Text></Text>
    <TextFormatted>
      <Format>Counter: %d</Format>
      <Address>0x20000010</Address>
      <LittleEndian>true</LittleEndian>
    </TextFormatted>
  </Items>
</Window>

Use the editor's property controls to select valid types and modes. Address selects the stored location; Text/Label names the widget; Width/Height or ItemWidth control supported dimensions. Min/Max, Format, Color and other properties are widget-specific, not universal. Hover a property label for its description. Colors use #RRGGBB or #AARRGGBB.

For generic DataType properties the numeric codes are: 0 Boolean, 1 signed 8-bit, 2 unsigned 8-bit, 3 signed 16-bit, 4 unsigned 16-bit, 5 signed 32-bit, 6 unsigned 32-bit, 7 Float32, 8 Float32 inverse, 9 signed 64-bit, 10 unsigned 64-bit. Not every widget accepts every type; some use different choices (for example Equalizer uses float or int16). Use the property's dropdown rather than assuming codes apply everywhere.

LittleEndian defaults to true where supported (normal Cortex-M byte order). Set false for big-endian stored data. FLOAT_INV reverses the selected byte order. Check both width and signedness when values look wrong.

Window PosX/PosY/Width/Height optionally set initial geometry. Widget PosX/PosY use window coordinates, with 0,0 at the top-left of the title bar. Without an explicit position, widgets follow the flow layout. Optional properties have loader defaults; malformed ranges, dimensions and types are sanitized, but this does NOT validate whether a physical address is safe.)guide"},
        {"6. XML Editor (Widget Editor)", R"guide(Open Windows > XML Editor after creating or opening an XML file. The Widget Editor tree and Design Mode share the same document.

Select a Window, widget or list entry in the tree to edit its properties. Addresses, numbers, checkboxes, colors and enum dropdowns use purpose-built controls. Hover labels for help. Add property lists supported properties not yet present; orange entries are ignored by the loader for that widget.

Add Widget inserts into the selected container, after the selected widget, or at the end of the first window. Add child widget and list-specific buttons such as Add Line / Add Combo add nested elements. Delete this node removes the selected element; be careful when deleting a container with children.

Save writes the current XML file and reloads its windows. Changes can be previewed live through the shared document before saving; they are not durable until saved. Keep a backup before major layout edits. Editing XML is not a substitute for locking target writes.)guide"},
        {"7. Design Mode and layout editing", R"guide(Open/create a dashboard, then use Windows > Design Mode or Ctrl+E. The toolbar and Properties window let you add windows/widgets, duplicate, delete, undo, redo and save. Changes rebuild the dashboard immediately; Save (Ctrl+S) writes them to disk.

- Click a widget to select the deepest nested item; click empty window space to select the window. Use Properties or the context menu to select a parent.
- Drag a widget to move it. The default grid is 8 logical units; hold Alt to bypass snapping. The small label above a tree/header widget lets you drag it without toggling its contents. Widgets inside Grid stay in the grid.
- Drag orange right, bottom or corner handles to resize. The editor updates the appropriate Width/Height, Radius, PixelSize, Scale, BandWidth or ItemWidth property.
- Move/resize dashboard windows normally; their geometry is recorded in Window properties.
- Right-click for Add widget here, duplicate, ordering, Back to flow layout, parent selection and delete. Later widgets draw over earlier ones. Back to flow layout removes the explicit position.
- Properties shows existing values normally and missing defaults in grey. Editing a default creates the property; x removes it. Lists, zones and child widgets can be edited in place.

While a dashboard window or Properties has focus (and you are not entering text): arrow keys move by 1 unit; Shift+arrows use the grid step; Del deletes; Ctrl+D duplicates; Ctrl+Z undoes; Ctrl+Y or Ctrl+Shift+Z redoes; Ctrl+S saves; Esc deselects or cancels a drag. Undo/redo also covers XML-editor changes.

Controls do not react to normal value-editing clicks in Design Mode, but live reads continue. Trees, headers and tabs still open to reach nested widgets. Keep READ ONLY active for the safest editing workflow.)guide"},
        {"8. Widget catalogue", R"guide(Use Add Widget for templates and the property panel for each widget's exact options. examples/demo.xml demonstrates all 65 supported XML widget tags.

Text/help: Text, TextColored, TextFormatted (integer), TextFormattedF (float), LabelText, SelectableText, Tooltip, HelpMarker, PlotTooltip.

Controls: Button, GlossyButton, Checkbox, RadioButton (Radio list), ComboBox (Combo list), ImputInt, ImputFloat, SliderInt, SliderFloat, CustomVerticalSlider, NeumorphicKnob, StyledKnob, SegmentButton (Segment list), RotarySwitch, Keypad, BitControl, BitControl8Bit. The XML input tags really are spelled ImputInt and ImputFloat, not InputInt/InputFloat. Slider appearance/mode choices are available in the editor.

Indicators: ProgressBar, Gauge, RPMIndicator (Zone list), VerticalGauge, VUMeter, Tank, Equalizer, LedIndicator, SevenSegment, MechanicalSwitch2, GlossyDisplay, LCD_HD44780.

Charts: PlotLines, Histogram, DataLog. Memory and variables: MemoryView, MemoryEditor, Watch. Media: Image, Gif.

Containers: Group, TreeNode, Grid, FixedWidth, TabBar with Tab children, Column, ChildWindow, Header, Popup, HintBox.

Flow/layout helpers: SameLine, NewLine, Spacing, Separator, Dummy, Indent, AlignTextToFramePadding, SetCursorPos. Use Items for nested widgets; use the editor's list buttons for Line, Radio, Combo, Segment and Zone entries rather than inserting unrelated widgets into these lists.

Image and Gif file paths are resolved from the executable folder, not from the dashboard's folder. Copy the referenced assets there.)guide"},
        {"9. PlotLines, DataLog and Histogram", R"guide(PlotLines is for live trends. Samples come from the communication thread with timestamps, not one value per rendered frame. Freq controls downsampling (up to 1000 Hz); it does not guarantee that the probe can deliver that rate. Group nearby variables to reduce J-Link calls.

Open the plot's Options to change Real Time Update, Freq (Hz) and Time Window (s). These runtime choices are saved per dashboard/plot in settings.ini; a saved preference overrides the XML startup value. Multiple Items > Line entries can each have Name, Address, DataType, LittleEndian, Scale, LineColor and LineWeight. Each line has its own history; the legacy BufferId property does not select PlotLines histories. Set Freq explicitly: a new editor template uses 100 Hz, but the loader's missing-property fallback is 1 Hz.

For signals faster than polling, use DataLog or Histogram: the firmware fills an array at its own sample rate. Set Size to its element count and choose matching Line Address/DataType and scaling. Multiple Line entries are supported. ScaleX/ScaleY and Smooth control presentation; they do not change the firmware's sample rate.

With DoneAddr, the target signals that a capture is ready and must hold the buffer stable. The application reads the complete capture, then acknowledges by clearing the flag. READ ONLY blocks this acknowledgement too: a capture protocol can appear stuck until you deliberately enable writes. Include both the buffer and flag in the MCU mapping and allow the flag's actual stored width in your write policy.

Do not interpret a reconnection gap or cleared cache as valid samples. Check the connection state and firmware's capture protocol before diagnosing a flat or empty plot.)guide"},
        {"10. Watch and memory tools", R"guide(Watch provides editable rows with enable, physical address, name, value and description. Right-click an empty area for New Row, Export... or Import... (Watch XML). Right-click a row for Delete Row, stored type, Bit View, Hex View and little Endian. Choose the real stored type before interpreting or editing a value. Watch configuration changes are autosaved to its configured XML file; target value edits are not saved as configuration. This is separate from saving the dashboard layout.

MemoryView uses BaseAddress, NumBytes and BytesPerLine. Edit a hexadecimal byte and press Enter to commit. MemoryEditor uses BaseAddress, Size and Cols, with options including ShowAscii, ShowHexII, ShowPreview and ReadOnly. UseExternal must be true for MemoryEditor to access target memory; the default false edits a local buffer only. These sizes are bytes, unlike the element count in an array plot.

The global target lock and limits cover BOTH memory tools and Watch. A widget's ReadOnly option is an additional restriction, not permission to bypass the global lock. Byte editing cannot update part of a value protected by a typed value rule; write the complete matching type instead.

Reading peripheral registers may have side effects even in READ ONLY. Only inspect documented safe addresses. Do not use memory tools as a flash programmer or assume arbitrary MCU addresses belong to RAM.)guide"},
        {"11. Demo dashboard and firmware", R"guide(Open examples/demo.xml to explore Overview, Gauges, Controls, Plots, Layout, Memory, Media and Watch. Layout editing works without J-Link; meaningful live values require matching firmware.

examples/demo_firmware.c supplies a Cortex-M demo module. Call demo_init() once, demo_tick_1khz() from a 1 kHz timer interrupt and demo_poll() from the main loop. Reserve 0x2D0 bytes at 0x20001000 in your linker configuration, or relocate the structure and change the XML addresses together. Do not overwrite existing target RAM blindly.

The XML header documents the structure's offset map. The firmware generates waves, indicators, LCD content and buffered captures with DoneAddr handshakes. Copy demo_logo.png and demo_spinner.gif beside J-Link Utils.exe for the Media window. Review write protection before using controls or capture acknowledgements.)guide"},
        {"12. Troubleshooting", R"guide(Application will not start / missing DLL: install SEGGER J-Link and use the DLL matching x64 or x86. The app reports a missing DLL and exits; it does not bundle it.

Cannot connect: check USB, driver, target power/reference voltage, SWD wiring and the exact selected MCU. Review Settings > Last error. After five failed attempts, fix the problem and press Connect. Disconnect or Cancel connection stops retries, subject to a running DLL call finishing.

No windows loaded: open the intended file; check XML syntax and top-level Window / Items structure. Do not wrap Window elements in an extra root. Unknown/misspelled widget tags are skipped. Reopen hidden windows from Windows. Save editor changes to keep them on disk.

Zeros, wrong or stale values: check Connected, base mapping, physical address, all bytes fitting in the mapping, stored type, endianness and firmware layout. Values immediately after a cache reset are not measurements. Hidden/non-drawn widgets stop requesting data after approximately 0.5 seconds.

Value edits have no effect: inspect READ ONLY, Settings protection error/count, active address and value limits, widget-specific ReadOnly and Design Mode. Firmware may also overwrite a value immediately. Do not repeatedly unlock or retry without finding the cause.

Capture stuck: check DoneAddr, readiness flag and array type/size. READ ONLY prevents clearing the flag. Flat/slow PlotLines: check Real Time Update, Freq, Time Window and achievable polling bandwidth.

Images missing: place Image/Gif assets in the executable folder. Layout too large/small: check Windows DPI and Windows > Fonts; XML dimensions already use logical units. Save a backup before resetting settings or layout files.)guide"},
        {"13. Keyboard and mouse reference", R"guide(Application: Ctrl+N New; Ctrl+O Open; Ctrl+Q Close dashboard (not Quit); Ctrl+E toggle Design Mode; Alt+F4 Quit. Design-specific save/undo/duplicate/delete/arrow shortcuts require Design Mode and dashboard/Properties focus; see section 7.

Ordinary floating windows: drag their title bars to move, double-click the title bar to collapse, drag the lower-right corner/edges to resize. The main application uses its own title-bar minimize/maximize controls instead of collapsing.

Tab / Shift+Tab cycles editable fields. Ctrl+click a standard slider/drag field to enter a value as text. Mouse wheel scrolls. When entering text: Shift+arrows or drag selects; Ctrl+Left/Right jumps words; Ctrl+A selects all; Ctrl+C/X/V copies/cuts/pastes; Ctrl+Z/Y undoes/redoes text; Esc cancels the current text edit. These text actions are not dashboard-wide undo.

In this guide, Search filters section titles and full text (case-insensitive). Clear restores all sections. Expand all / Collapse all affect the visible sections; matching sections open automatically when the search changes. Scroll the content area to read long sections.)guide"}
    };
    count = static_cast<int>(sizeof(sections) / sizeof(sections[0]));
    return sections;
}

inline bool Matches(const ImGuiTextFilter& filter, const Section& section)
{
    if (!filter.IsActive()) return true;
    // Match the combined title/body so comma-separated inclusions and -exclusions
    // have exactly the same semantics as an ordinary ImGui text filter.
    ImGuiTextBuffer searchable;
    searchable.append(section.title);
    searchable.append("\n");
    searchable.append(section.text);
    return filter.PassFilter(searchable.c_str());
}

inline void Show(bool* p_open)
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    // Default only: users can resize the guide. Fit first use on small displays.
    ImGui::SetNextWindowSize(ImVec2(display.x < 780.0f ? display.x * 0.9f : 780.0f,
                                  display.y < 680.0f ? display.y * 0.85f : 680.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("User Guide", p_open)) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("J-Link Utils - connection, safe target access and XML dashboards");
    static ImGuiTextFilter filter;
    ImGui::SetNextItemWidth(-1.0f);
    bool searchChanged = ImGui::InputTextWithHint("##guide_search", "Search guide (e.g. DoneAddr, Watch, Ctrl+S)",
                                                filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf));
    if (ImGui::Button("Clear")) {
        filter.Clear();
        searchChanged = true;
    }
    if (searchChanged) filter.Build();
    ImGui::SameLine();
    int openAction = -1;
    if (ImGui::Button("Expand all")) openAction = 1;
    ImGui::SameLine();
    if (ImGui::Button("Collapse all")) openAction = 0;
    ImGui::Separator();

    // Keep the search/controls visible while the document scrolls.
    if (ImGui::BeginChild("Guide contents", ImVec2(0, 0))) {
        int count = 0;
        const Section* sections = Sections(count);
        int matches = 0;
        for (int i = 0; i < count; ++i) {
            if (!Matches(filter, sections[i])) continue;
            ++matches;
            if (openAction != -1)
                ImGui::SetNextItemOpen(openAction != 0, ImGuiCond_Always);
            else if (searchChanged && filter.IsActive())
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            if (ImGui::CollapsingHeader(sections[i].title, i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
                ImGui::PushTextWrapPos(0.0f);
                // Never treat documentation/printf examples as a format string.
                ImGui::TextUnformatted(sections[i].text);
                ImGui::PopTextWrapPos();
                ImGui::Spacing();
            }
        }
        if (!matches) ImGui::TextWrapped("No matching sections. Try another term or Clear the search.");
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace user_guide
