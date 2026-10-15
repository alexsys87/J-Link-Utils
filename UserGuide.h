#pragma once

#include <imgui.h>
#include "Localization.h"

// Kept independent of the transport and Windows headers so the complete guide
// can be exercised with real ImGui without a J-Link or a native window.
namespace user_guide {

struct Section {
    const char* title;
    const char* text;
};

inline const Section* EnglishSections(int& count)
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

inline const Section* RussianSections(int& count)
{
    static const Section sections[] = {
        {"1. Быстрый старт", R"guide(J-Link Utils позволяет наблюдать и изменять память микроконтроллера через SEGGER J-Link по интерфейсу SWD. Окна и виджеты описываются в XML-файлах: панель можно создать без изменения кода C++.

1. Установите ПО и драйвер SEGGER J-Link. Используйте DLL, соответствующую разрядности приложения: JLink_x64.dll для x64, JLinkARM.dll для x86. Эта проприетарная DLL не входит в комплект приложения.
2. Подключите целевое устройство и подайте питание; проверьте соединения SWD и опорное напряжение. В меню «Настройки» отфильтруйте список устройств J-Link и выберите точную модель микроконтроллера.
3. Задайте «Адрес памяти МК» (MCU Memory address) — начало области, которую хотите просматривать (по умолчанию 0x20000000). Все адреса виджетов должны находиться внутри отображаемой области.
4. Нажмите «Подключить» (Connect) и дождитесь состояния «Подключено» (Connected). Откройте XML-панель через «Файл > Открыть» или создайте её через «Файл > Создать». Редактировать компоновку можно и без подключения.
5. Начинайте в режиме «ТОЛЬКО ЧТЕНИЕ» (READ ONLY). Проверьте адреса, типы хранимых данных и безопасность целевого устройства, прежде чем явно разрешать запись в настройках.

Текущий транспорт использует SWD на частоте 8000 кГц; выбрать другую частоту в интерфейсе нельзя. Используйте только устройства и соединения, рассчитанные на эту скорость.)guide"},
        {"2. Файлы, окна и внешний вид", R"guide(«Файл > Создать» (Ctrl+N) предлагает выбрать имя файла и открывает редактор с пустым документом. Добавьте Window и виджеты, затем нажмите «Сохранить», чтобы записать файл. «Файл > Открыть» (Ctrl+O) заменяет текущую панель; «Файл > Закрыть» (Ctrl+Q) закрывает её. «Файл > Выход» или Alt+F4 завершает работу. Сохраняйте изменения перед открытием, созданием или закрытием другого документа, а также перед выходом: подтверждения при наличии несохранённых изменений нет.

Первым аргументом командной строки можно передать путь к XML, например:
"J-Link Utils.exe" "C:\dashboards\motor.xml"

Меню «Окна» (Windows) содержит список окон загруженной панели; включайте и выключайте пункты, чтобы показывать или скрывать окна. В этом же меню находятся «Поверх всех окон» (Top Most), выбор шрифта, «Стиль», «Редактор стиля», «Редактор XML» и «Режим дизайна».

В редакторе стиля доступны команды «Сохранить в XML», «Загрузить из XML», «Сохранить как пользовательский» и «Сбросить по умолчанию». «Сохранить как пользовательский» записывает custom.style.xml; для его использования выберите «Пользовательский» (Custom) в списке стилей. Файлы стилей не являются файлами панелей.

Параметры подключения, путь к панели, обычные размеры и положение главного окна/состояние развёртывания, видимость редактора, параметры режима дизайна и сетки, параметры отображения Watch, видимость окон панели и параметры PlotLines хранятся в settings.ini. Компоновка ImGui хранится в window.ini. Эти файлы и custom.style.xml размещаются в каталоге исполняемого файла независимо от рабочей папки ярлыка. Перед запуском перенесите старые файлы из другой рабочей папки рядом с EXE; сохраняйте резервные копии. Режим «Поверх всех окон» применяется сразу. Разрешение записи в целевое устройство никогда не сохраняется.

Изменённые настройки сохраняются примерно раз в секунду и при штатном выходе. При ошибке сохранения прежний INI остаётся нетронутым, а приложение показывает сообщение об ошибке с кнопкой повтора. Каталог исполняемого файла должен быть доступен для записи. Некорректные числовые настройки заменяются безопасными значениями по умолчанию; если сохранённый шрифт недоступен, используется установленный шрифт. XML-компоновки, стили и размеры сетки задаются в логических единицах при 96 DPI; масштабирование экрана Windows применяется автоматически. Не умножайте размеры в XML для каждого монитора.

В заголовке главного окна есть кнопки сворачивания, развёртывания/восстановления и закрытия. Перетаскивайте заголовок, чтобы переместить приложение, а края и углы — чтобы изменить размер. Сворачивание приостанавливает отрисовку, но не поток обмена с J-Link.)guide"},
        {"3. Подключение, адресация и восстановление связи", R"guide(Параметр «Настройки > Адрес памяти МК» (MCU Memory address) отображает до 1 МиБ начиная с базового адреса, но не за пределами 32-битного адресного пространства. Адреса виджетов — это физические адреса МК, а НЕ смещения. При базе 0x20000000 смещение 0x10 записывается как Address 0x20000010. Значение или массив должны целиком помещаться в отображаемую область.

Поля адресов в XML принимают 0x20000010, #20000010 или десятичную запись. Начальный ноль без x означает восьмеричную запись. Для шестнадцатеричных чисел используйте # или 0x. Поля адресов в настройках показывают шестнадцатеричные цифры.

Изменение базового адреса или выбранного МК останавливает обмен и удаляет ожидающие запросы; снова нажмите «Подключить». «Автоподключение» (Auto Connect) лишь запрашивает подключение при запуске приложения и не разрешает запись.

В главном меню отображаются состояния «Подключение» (Connecting), «Подключено» (Connected), «Переподключение» (Reconnecting) или «Не удалось подключиться» (Connection failed). В настройках дополнительно видны последняя ошибка, число попыток и задержка повтора. Выполняется не более пяти попыток с задержками 0,5, 1, 2 и 4 секунды. После состояния «Не удалось подключиться» устраните причину и нажмите «Подключить». «Отменить подключение» / «Отключить» останавливает повторные попытки.

После ошибки ввода-вывода очищаются кэшированные значения, отсчёты графиков и ожидающие операции записи. После восстановления связи подписки возобновляются, но запись остаётся заблокированной. Неудачная запись могла выполниться частично: проверьте целевое устройство перед новой записью. Неудачные или поставленные в очередь записи НЕ воспроизводятся повторно. Нули в очищенном кэше не являются измеренными значениями; всегда проверяйте состояние подключения.

Команда «Отключить» может немедленно прервать ожидание повтора, но должна дождаться завершения уже выполняющегося синхронного вызова DLL J-Link. Для зависшего вызова DLL гарантированного срока отмены нет.)guide"},
        {"4. Защита от записи — прочтите перед записью", R"guide(«ТОЛЬКО ЧТЕНИЕ» (READ ONLY) — режим по умолчанию, в том числе после автоподключения. Он блокирует все записи в целевое устройство: из элементов управления, Watch, редакторов памяти и автоматических подтверждений DoneAddr. Чтение продолжает работать. Эта программная блокировка не заменяет аппаратных мер безопасности.

При заблокированной записи откройте «Настройки > Защита записи в целевое устройство» (Target write protection):
- При необходимости включите «Ограничить адреса записи (черновик)» (Limit writable addresses (draft)), задайте «Первый адрес» и «Последний адрес» включительно, затем нажмите «Применить ограничение адресов». До нажатия «Применить» это лишь черновик. Каждый байт записи должен находиться в действующем диапазоне.
- Необязательные ограничения значений по адресам позволяют задать до 32 неперекрывающихся значений. Введите физический адрес, фактический тип хранимых данных и десятичные «Минимум»/«Максимум»; нажмите «Добавить ограничение значения». Поддерживаются U8/U16/U32/U64, I8/I16/I32/I64 и Float32/Float64. «Удалить» удаляет правило. Значения вне разрешённых границ отклоняются, а не ограничиваются ближайшей границей.
- Правила требуют точного совпадения адреса и полной ширины хранимого значения. Частичные или перекрывающиеся побайтовые изменения отклоняются. Учитывайте фактическое представление в МК: для битов float используйте Float32, даже если виджет передаёт их как U32. Защищённые значения с плавающей точкой не могут быть NaN или бесконечностью.

Подключитесь, нажмите «Разрешить запись...» (Enable writes...), проверьте устройство, отображаемую область и действующие ограничения, подтвердите принятие риска, затем нажмите «Разрешить для этого подключения». Это разрешает ВСЕ допустимые записи в рамках данного подключения, а не только одно нажатие. Адреса без правил значений по-прежнему подчиняются общей блокировке, диапазону адресов и отображаемой области памяти МК.

«Заблокировать запись» в главном меню или «Заблокировать запись в устройство сейчас» в настройках отзывает разрешение и удаляет ещё не отправленные записи. Запись, уже переданную в SDK, отозвать нельзя. Отключение, ошибка ввода-вывода, переподключение или изменение защиты снова блокируют запись. Отклонённые записи не сохраняются для последующего выполнения.

Разрешение и правила защиты действуют только до завершения текущего запуска программы; при переподключении правила сохраняются, а разрешение — нет. В настройках показаны последняя ошибка защиты и счётчик заблокированных запросов. Проверьте их, если элемент управления кажется неработающим.)guide"},
        {"5. Структура XML и общие свойства", R"guide(Загрузчик ожидает элементы Window верхнего уровня без дополнительного охватывающего корневого элемента. Поддерживается несколько соседних элементов Window. Каждый Window содержит Text (имя окна) и Items (виджеты). Виджеты-контейнеры используют собственные Items. Имена XML-тегов чувствительны к регистру; неизвестные виджеты пропускаются.

Минимальная панель только для чтения (адрес приведён для примера и не является проверенным адресом переменной на вашем устройстве):
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

Выбирайте допустимые типы и режимы с помощью элементов управления свойствами в редакторе. Address задаёт адрес хранения; Text/Label — название виджета; Width/Height или ItemWidth — поддерживаемые размеры. Min/Max, Format, Color и другие свойства зависят от виджета и не являются универсальными. Наведите указатель на название свойства, чтобы увидеть описание. Цвета задаются как #RRGGBB или #AARRGGBB.

Для общих свойств DataType используются числовые коды: 0 — Boolean, 1 — знаковое 8-битное, 2 — беззнаковое 8-битное, 3 — знаковое 16-битное, 4 — беззнаковое 16-битное, 5 — знаковое 32-битное, 6 — беззнаковое 32-битное, 7 — Float32, 8 — Float32 inverse, 9 — знаковое 64-битное, 10 — беззнаковое 64-битное. Не каждый виджет принимает все типы; некоторые используют другой набор (например, Equalizer использует float или int16). Выбирайте значение из выпадающего списка свойства, а не считайте эти коды универсальными.

LittleEndian по умолчанию равно true там, где поддерживается (обычный порядок байтов Cortex-M). Для данных с порядком big-endian задайте false. FLOAT_INV меняет выбранный порядок байтов на противоположный. Если значения выглядят неверно, проверяйте и разрядность, и знаковость.

Свойства Window PosX/PosY/Width/Height могут задавать начальные положение и размеры окна. PosX/PosY виджетов используют координаты окна, где 0,0 — левый верхний угол строки заголовка. Без явной позиции виджеты размещаются последовательно, в потоке. Для необязательных свойств загрузчик использует значения по умолчанию; некорректные диапазоны, размеры и типы приводятся к допустимым значениям, но это НЕ проверяет безопасность физического адреса.)guide"},
        {"6. Редактор XML (редактор виджетов)", R"guide(После создания или открытия XML-файла выберите «Окна > Редактор XML». Дерево редактора виджетов (Widget Editor) и режим дизайна работают с одним и тем же документом.

Выберите Window, виджет или элемент списка в дереве, чтобы изменить его свойства. Для адресов, чисел, флажков, цветов и перечислений предусмотрены специальные элементы управления. Наводите указатель на названия для справки. «Добавить свойство» показывает поддерживаемые свойства, которых ещё нет; оранжевые пункты игнорируются загрузчиком для этого виджета.

«Добавить виджет» вставляет виджет в выбранный контейнер, после выбранного виджета или в конец первого окна. «Добавить дочерний виджет» и кнопки списков, например «Добавить линию» / «Добавить элемент Combo» (Add Line / Add Combo), добавляют вложенные элементы. «Удалить этот узел» удаляет выбранный элемент; будьте осторожны при удалении контейнера с дочерними элементами.

«Сохранить» записывает текущий XML-файл и перезагружает его окна. Благодаря общему документу изменения можно просматривать в реальном времени до сохранения, но постоянными они становятся только после записи в файл. Перед значительными изменениями компоновки сделайте резервную копию. Редактирование XML не заменяет блокировку записи в целевое устройство.)guide"},
        {"7. Режим дизайна и редактирование компоновки", R"guide(Откройте или создайте панель, затем выберите «Окна > Режим дизайна» или нажмите Ctrl+E. Панель инструментов и окно «Свойства» позволяют добавлять окна/виджеты, дублировать, удалять, отменять и повторять действия, а также сохранять. Изменения сразу перестраивают панель; «Сохранить» (Ctrl+S) записывает их на диск.

- Щёлкните виджет, чтобы выбрать самый глубоко вложенный элемент; щёлкните пустое место окна, чтобы выбрать окно. Для выбора родителя используйте окно «Свойства» или контекстное меню.
- Перетаскивайте виджет для перемещения. Шаг сетки по умолчанию — 8 логических единиц; удерживайте Alt, чтобы отключить привязку. Небольшая метка над деревом/заголовком позволяет перетаскивать его, не раскрывая и не сворачивая содержимое. Виджеты внутри Grid остаются в сетке.
- Для изменения размера перетаскивайте оранжевые маркеры справа, снизу или в углу. Редактор обновляет соответствующее свойство Width/Height, Radius, PixelSize, Scale, BandWidth или ItemWidth.
- Перемещайте окна панели и меняйте их размеры обычным способом; геометрия записывается в свойства Window.
- Щелчок правой кнопкой открывает команды «Добавить виджет здесь», дублирования, изменения порядка, «Вернуть в потоковую компоновку», выбора родителя и удаления. Более поздние виджеты рисуются поверх более ранних. Возврат в потоковую компоновку удаляет явно заданную позицию.
- Окно «Свойства» показывает существующие значения обычным цветом, а отсутствующие значения по умолчанию — серым. Изменение значения по умолчанию создаёт свойство; x удаляет его. Списки, зоны и дочерние виджеты можно редактировать на месте.

Когда фокус находится в окне панели или в окне «Свойства» (и вы не вводите текст): стрелки перемещают на 1 единицу; Shift+arrows (Shift+стрелки) — на шаг сетки; Del удаляет; Ctrl+D дублирует; Ctrl+Z отменяет; Ctrl+Y или Ctrl+Shift+Z повторяет; Ctrl+S сохраняет; Esc снимает выделение или отменяет перетаскивание. Отмена/повтор охватывают также изменения в XML-редакторе.

В режиме дизайна элементы управления не реагируют на обычные щелчки для изменения значений, но чтение текущих данных продолжается. Деревья, заголовки и вкладки по-прежнему открываются для доступа к вложенным виджетам. Для наиболее безопасного редактирования оставляйте режим «ТОЛЬКО ЧТЕНИЕ» включённым.)guide"},
        {"8. Каталог виджетов", R"guide(Используйте «Добавить виджет» для выбора шаблона и панель свойств для точных параметров каждого виджета. examples/demo.xml демонстрирует все 65 поддерживаемых XML-тегов виджетов.

Текст и справка: Text, TextColored, TextFormatted (целое число), TextFormattedF (число с плавающей точкой), LabelText, SelectableText, Tooltip, HelpMarker, PlotTooltip.

Элементы управления: Button, GlossyButton, Checkbox, RadioButton (список Radio), ComboBox (список Combo), ImputInt, ImputFloat, SliderInt, SliderFloat, CustomVerticalSlider, NeumorphicKnob, StyledKnob, SegmentButton (список Segment), RotarySwitch, Keypad, BitControl, BitControl8Bit. XML-теги ввода действительно называются ImputInt и ImputFloat, а не InputInt/InputFloat. Внешний вид и режимы ползунков выбираются в редакторе.

Индикаторы: ProgressBar, Gauge, RPMIndicator (список Zone), VerticalGauge, VUMeter, Tank, Equalizer, LedIndicator, SevenSegment, MechanicalSwitch2, GlossyDisplay, LCD_HD44780.

Графики: PlotLines, Histogram, DataLog. Память и переменные: MemoryView, MemoryEditor, Watch. Медиа: Image, Gif.

Контейнеры: Group, TreeNode, Grid, FixedWidth, TabBar с дочерними Tab, Column, ChildWindow, Header, Popup, HintBox.

Вспомогательные элементы компоновки: SameLine, NewLine, Spacing, Separator, Dummy, Indent, AlignTextToFramePadding, SetCursorPos. Для вложенных виджетов используйте Items; для элементов Line, Radio, Combo, Segment и Zone используйте кнопки списков в редакторе, а не вставляйте в эти списки посторонние виджеты.

Пути к файлам Image и Gif отсчитываются от каталога исполняемого файла, а не от папки панели. Скопируйте используемые ресурсы туда.)guide"},
        {"9. PlotLines, DataLog и Histogram", R"guide(PlotLines предназначен для наблюдения сигналов в реальном времени. Отсчёты поступают из потока обмена с временными метками, а не по одному значению на каждый отрисованный кадр. Freq управляет частотой выборки при прореживании (до 1000 Гц); это не гарантирует, что отладчик обеспечит такую частоту. Группируйте близко расположенные переменные, чтобы сократить число вызовов J-Link.

Откройте «Параметры» (Options) графика, чтобы изменить «Обновление в реальном времени» (Real Time Update), «Частота (Гц)» (Freq (Hz)) и «Временное окно (с)» (Time Window (s)). Эти рабочие параметры сохраняются для каждой панели/графика в settings.ini; сохранённая настройка имеет приоритет над начальным значением из XML. Несколько элементов Items > Line могут иметь собственные Name, Address, DataType, LittleEndian, Scale, LineColor и LineWeight. У каждой линии своя история; устаревшее свойство BufferId не выбирает истории PlotLines. Задавайте Freq явно: новый шаблон редактора использует 100 Гц, а при отсутствии свойства загрузчик использует 1 Гц.

Для сигналов быстрее частоты опроса используйте DataLog или Histogram: прошивка заполняет массив с собственной частотой дискретизации. Задайте в Size число элементов и выберите соответствующие Address/DataType элемента Line и масштабирование. Поддерживается несколько элементов Line. ScaleX/ScaleY и Smooth влияют на отображение, а не на частоту дискретизации прошивки.

При использовании DoneAddr целевое устройство сообщает о готовности записи сигнала и должно сохранять буфер неизменным. Приложение считывает всю запись, затем подтверждает получение, сбрасывая флаг. Режим «ТОЛЬКО ЧТЕНИЕ» блокирует и это подтверждение: протокол захвата может выглядеть зависшим, пока вы явно не разрешите запись. Включите и буфер, и флаг в отображаемую область памяти МК и разрешите в политике записи фактическую ширину хранимого флага.

Не считайте разрыв при переподключении или очищенный кэш достоверными отсчётами. Прежде чем искать причину плоского или пустого графика, проверьте состояние подключения и протокол захвата в прошивке.)guide"},
        {"10. Watch и инструменты работы с памятью", R"guide(Watch содержит редактируемые строки с флажком активности, физическим адресом, именем, значением и описанием. Щёлкните правой кнопкой в пустой области, чтобы выбрать «Новая строка», «Экспорт...» или «Импорт...» (XML Watch). Щёлкните строку правой кнопкой для доступа к команде «Удалить строку», типу хранимых данных, «Битовому виду» (Bit View), «Шестнадцатеричному виду» (Hex View) и little Endian. Перед интерпретацией или изменением значения выберите фактический тип хранимых данных. Изменения конфигурации Watch автоматически сохраняются в заданный для него XML-файл; изменения значений в целевом устройстве не сохраняются как конфигурация. Это не связано с сохранением компоновки панели.

MemoryView использует BaseAddress, NumBytes и BytesPerLine. Измените шестнадцатеричный байт и нажмите Enter для подтверждения. MemoryEditor использует BaseAddress, Size и Cols, а также параметры ShowAscii, ShowHexII, ShowPreview и ReadOnly. Для доступа MemoryEditor к памяти целевого устройства UseExternal должно быть true; значение false по умолчанию позволяет редактировать только локальный буфер. Эти размеры задаются в байтах, в отличие от числа элементов в графике массива.

Общая блокировка записи в устройство и ограничения распространяются на ОБА инструмента памяти и Watch. Параметр ReadOnly виджета — дополнительное ограничение, а не разрешение обойти общую блокировку. Побайтовое редактирование не может изменить часть значения, защищённого правилом с типом данных; вместо этого записывайте полное значение соответствующего типа.

Чтение регистров периферии может иметь побочные эффекты даже в режиме «ТОЛЬКО ЧТЕНИЕ». Просматривайте только документированные безопасные адреса. Не используйте инструменты памяти как программатор flash-памяти и не предполагайте, что произвольный адрес МК относится к ОЗУ.)guide"},
        {"11. Демонстрационная панель и прошивка", R"guide(Откройте examples/demo.xml, чтобы изучить окна Overview, Gauges, Controls, Plots, Layout, Memory, Media и Watch. Редактирование компоновки работает без J-Link; для осмысленных текущих значений требуется соответствующая прошивка.

examples/demo_firmware.c содержит демонстрационный модуль для Cortex-M. Вызовите demo_init() один раз, demo_tick_1khz() — из прерывания таймера с частотой 1 кГц, а demo_poll() — из основного цикла. Зарезервируйте 0x2D0 байт по адресу 0x20001000 в настройках компоновщика либо одновременно перенесите структуру и измените адреса в XML. Не перезаписывайте существующее ОЗУ устройства вслепую.

В заголовке XML приведена карта смещений структуры. Прошивка генерирует сигналы, показания индикаторов, содержимое LCD и буферизованные записи с подтверждениями через DoneAddr. Для окна Media скопируйте demo_logo.png и demo_spinner.gif рядом с J-Link Utils.exe. Перед использованием элементов управления или подтверждений захвата проверьте защиту записи.)guide"},
        {"12. Устранение неполадок", R"guide(Приложение не запускается / отсутствует DLL: установите SEGGER J-Link и используйте DLL для нужной разрядности x64 или x86. Приложение сообщает об отсутствии DLL и завершает работу; DLL не входит в его комплект.

Не удаётся подключиться: проверьте USB, драйвер, питание и опорное напряжение целевого устройства, соединения SWD и точную модель выбранного МК. Посмотрите «Настройки > Последняя ошибка» (Last error). После пяти неудачных попыток устраните причину и нажмите «Подключить». «Отключить» или «Отменить подключение» останавливает повторы, но необходимо дождаться завершения уже выполняющегося вызова DLL.

Окна не загружены: откройте нужный файл; проверьте синтаксис XML и структуру Window / Items верхнего уровня. Не оборачивайте элементы Window дополнительным корневым элементом. Неизвестные теги виджетов и теги с опечатками пропускаются. Покажите скрытые окна через меню «Окна». Сохраните изменения редактора, чтобы они остались на диске.

Нули, неверные или устаревшие значения: проверьте состояние «Подключено», базовую отображаемую область, физический адрес, вхождение всех байтов в эту область, тип хранимых данных, порядок байтов и размещение данных в прошивке. Значения сразу после сброса кэша не являются измерениями. Скрытые или неотрисовываемые виджеты перестают запрашивать данные примерно через 0,5 секунды.

Изменения значений не действуют: проверьте режим «ТОЛЬКО ЧТЕНИЕ», ошибку/счётчик защиты в настройках, действующие ограничения адресов и значений, параметр ReadOnly виджета и режим дизайна. Прошивка также может сразу перезаписывать значение. Не снимайте блокировку и не повторяйте попытки многократно, не выяснив причину.

Захват завис: проверьте DoneAddr, флаг готовности, тип и размер массива. Режим «ТОЛЬКО ЧТЕНИЕ» не даёт сбросить флаг. PlotLines плоский или медленный: проверьте «Обновление в реальном времени», Freq, «Временное окно» и достижимую пропускную способность опроса.

Нет изображений: поместите ресурсы Image/Gif в каталог исполняемого файла. Компоновка слишком большая или маленькая: проверьте DPI Windows и «Окна > Шрифты»; размеры в XML уже заданы в логических единицах. Перед сбросом файлов настроек или компоновки сделайте резервную копию.)guide"},
        {"13. Справочник клавиатуры и мыши", R"guide(Приложение: Ctrl+N — создать; Ctrl+O — открыть; Ctrl+Q — закрыть панель (не выход из приложения); Ctrl+E — включить/выключить режим дизайна; Alt+F4 — выход. Сочетания для сохранения, отмены, дублирования, удаления и перемещения стрелками в режиме дизайна требуют активного режима дизайна и фокуса в окне панели/«Свойства»; см. раздел 7.

Обычные плавающие окна: перетаскивайте заголовок для перемещения, дважды щёлкните заголовок для сворачивания, перетаскивайте нижний правый угол/края для изменения размера. Главное окно приложения вместо сворачивания до заголовка использует собственные кнопки сворачивания и развёртывания.

Tab / Shift+Tab переключает редактируемые поля. Ctrl+click (Ctrl+щелчок) по стандартному ползунку или полю с перетаскиванием позволяет ввести значение текстом. Колесо мыши прокручивает содержимое. При вводе текста: Shift+arrows (Shift+стрелки) или перетаскивание выделяет; Ctrl+Left/Right перемещает по словам; Ctrl+A выделяет всё; Ctrl+C/X/V копирует/вырезает/вставляет; Ctrl+Z/Y отменяет/повторяет действия с текстом; Esc отменяет текущее редактирование текста. Эти действия с текстом не являются отменой изменений всей панели.

В этом руководстве «Поиск» фильтрует заголовки разделов и полный текст на русском и английском языках без учёта регистра. «Очистить» возвращает все разделы. «Развернуть всё» / «Свернуть всё» действует на видимые разделы; при изменении поиска подходящие разделы открываются автоматически. Для чтения длинных разделов прокручивайте область содержимого.)guide"}
    };
    count = static_cast<int>(sizeof(sections) / sizeof(sections[0]));
    return sections;
}

inline const Section* Sections(int& count)
{
    return app_language::IsRussian() ? RussianSections(count) : EnglishSections(count);
}

// ImGuiTextFilter folds ASCII only. Fold Russian UTF-8 too, retaining its
// comma-separated inclusion / -exclusion syntax and all other bytes unchanged.
inline void AppendSearchFolded(ImGuiTextBuffer& out, const char* text)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
    while (*p) {
        char bytes[3] = {};
        if (*p >= 'A' && *p <= 'Z') {
            bytes[0] = static_cast<char>(*p++ + ('a' - 'A'));
        } else if (p[0] == 0xD0 && p[1] >= 0x90 && p[1] <= 0x9F) {
            bytes[0] = static_cast<char>(0xD0);
            bytes[1] = static_cast<char>(p[1] + 0x20);
            p += 2;
        } else if (p[0] == 0xD0 && p[1] >= 0xA0 && p[1] <= 0xAF) {
            bytes[0] = static_cast<char>(0xD1);
            bytes[1] = static_cast<char>(p[1] - 0x20);
            p += 2;
        } else if (p[0] == 0xD0 && p[1] == 0x81) {
            bytes[0] = static_cast<char>(0xD1);
            bytes[1] = static_cast<char>(0x91);
            p += 2;
        } else {
            bytes[0] = static_cast<char>(*p++);
        }
        out.append(bytes);
    }
}

inline bool Matches(const ImGuiTextFilter& filter, const Section& section,
                    const Section* alternate = nullptr)
{
    if (!filter.IsActive()) return true;
    // Match the combined title/body so comma-separated inclusions and -exclusions
    // have exactly the same semantics as an ordinary ImGui text filter.
    ImGuiTextBuffer searchable;
    AppendSearchFolded(searchable, section.title);
    searchable.append("\n");
    AppendSearchFolded(searchable, section.text);
    if (alternate && alternate != &section) {
        searchable.append("\n");
        AppendSearchFolded(searchable, alternate->title);
        searchable.append("\n");
        AppendSearchFolded(searchable, alternate->text);
    }
    ImGuiTextBuffer pattern;
    AppendSearchFolded(pattern, filter.InputBuf);
    const ImGuiTextFilter foldedFilter(pattern.c_str());
    return foldedFilter.PassFilter(searchable.c_str());
}

inline void Show(bool* p_open)
{
    const bool russian = app_language::IsRussian();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    // Default only: users can resize the guide. Fit first use on small displays.
    ImGui::SetNextWindowSize(ImVec2(display.x < 780.0f ? display.x * 0.9f : 780.0f,
                                  display.y < 680.0f ? display.y * 0.85f : 680.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(russian ? "Руководство пользователя###User Guide" : "User Guide###User Guide", p_open)) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("%s", russian
        ? "J-Link Utils — подключение, безопасный доступ к устройству и XML-панели"
        : "J-Link Utils - connection, safe target access and XML dashboards");
    static ImGuiTextFilter filter;
    ImGui::SetNextItemWidth(-1.0f);
    bool searchChanged = ImGui::InputTextWithHint("###guide_search", russian
                                                ? "Поиск в руководстве (например: DoneAddr, Watch, Ctrl+S)"
                                                : "Search guide (e.g. DoneAddr, Watch, Ctrl+S)",
                                                filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf));
    static bool previousRussian = russian;
    if (previousRussian != russian) {
        searchChanged = true;
        previousRussian = russian;
    }
    if (ImGui::Button(russian ? "Очистить###Clear" : "Clear###Clear")) {
        filter.Clear();
        searchChanged = true;
    }
    if (searchChanged) filter.Build();
    ImGui::SameLine();
    int openAction = -1;
    if (ImGui::Button(russian ? "Развернуть всё###Expand all" : "Expand all###Expand all")) openAction = 1;
    ImGui::SameLine();
    if (ImGui::Button(russian ? "Свернуть всё###Collapse all" : "Collapse all###Collapse all")) openAction = 0;
    ImGui::Separator();

    // Keep the search/controls visible while the document scrolls.
    if (ImGui::BeginChild("###Guide contents", ImVec2(0, 0))) {
        int count = 0;
        const Section* sections = Sections(count);
        int englishCount = 0;
        const Section* english = EnglishSections(englishCount);
        int alternateCount = 0;
        const Section* alternate = russian ? english : RussianSections(alternateCount);
        if (russian) alternateCount = englishCount;
        int matches = 0;
        for (int i = 0; i < count; ++i) {
            if (!Matches(filter, sections[i], i < alternateCount ? &alternate[i] : nullptr)) continue;
            ++matches;
            if (openAction != -1)
                ImGui::SetNextItemOpen(openAction != 0, ImGuiCond_Always);
            else if (searchChanged && filter.IsActive())
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            ImGuiTextBuffer header;
            header.append(sections[i].title);
            header.append("###");
            header.append(i < englishCount ? english[i].title : sections[i].title);
            if (ImGui::CollapsingHeader(header.c_str(), i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
                ImGui::PushTextWrapPos(0.0f);
                // Never treat documentation/printf examples as a format string.
                ImGui::TextUnformatted(sections[i].text);
                ImGui::PopTextWrapPos();
                ImGui::Spacing();
            }
        }
        if (!matches) ImGui::TextWrapped("%s", russian
            ? "Подходящих разделов нет. Попробуйте другой запрос или нажмите «Очистить»."
            : "No matching sections. Try another term or Clear the search.");
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace user_guide
