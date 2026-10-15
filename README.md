# J-Link-Utils

A C++17 Windows application for watching and changing microcontroller memory through a SEGGER J-Link over SWD. The user interface is described in XML: numeric fields, plots, indicators, a memory editor and other widgets. A built-in XML editor changes the configuration.

## Connecting

1. Install the SEGGER J-Link software and driver; connect the probe and the target microcontroller.
2. In **Settings**, choose the **J-Link device** and set the **MCU Memory address** (default `0x20000000`).
3. Click **Connect**, then open an XML configuration with **File → Open**. The XML path can also be passed as the first command-line argument.

Widget addresses are **physical addresses**, not offsets. `#20000010`, `0x20000010` and decimal values are accepted. A leading `0` without `x` means octal, so use `#` or `0x` for hexadecimal addresses. The visible range is limited to 1 MiB from the selected base and to the end of the 32-bit address space.

Changing the base address or the target device stops the exchange; click **Connect** again afterwards. A failed or short read or write also stops it. Stopping clears the cache and the pending requests: writes from an old connection are never replayed after reconnecting.

How the exchange works: every visible widget subscribes to its addresses, and a separate thread re-reads all subscribed addresses continuously (at most once per millisecond). Neighbouring addresses in memory regions (code/SRAM `0x00000000–0x3FFFFFFF` and external RAM `0x60000000–0x9FFFFFFF`) are merged into blocks: gaps of up to 64 bytes are read along with them, and a block of up to 4 KiB takes one J-Link call. In peripheral and system space only the requested bytes are read, so registers with read side effects are not touched. Addresses no widget asked for during 0.5 s (closed window, minimized application) are no longer polled. A written value shows up in the widget immediately; several writes to the same address while the J-Link is busy are sent as one, with the last value.

Byte order: every widget that reads or writes numbers takes `<LittleEndian>`. It defaults to `true`, the byte order of Cortex-M targets, so the tag can be omitted. Set it to `false` for big-endian data (Modbus registers, network buffers): the bytes are then swapped for every type, floats included. All widgets decode and encode values through the same code (`Widget/TypedValue.h`); the `FLOAT_INV` type uses the byte order opposite to `<LittleEndian>`.

### Plots

**PlotLines** gets timestamped values from the communication thread, one per polling cycle rather than one per frame, and thins them out to `Freq` (up to 1000 Hz). The X axis is real time, and the history holds the whole `Time Window`. The reachable rate depends on the probe: a J-Link BASE V8 (USB full speed) needs about 1–2 ms per call, so expect roughly 300–700 points per second when all plotted and displayed variables fit into one or two blocks; addresses scattered over memory share that rate.

For faster signals use **DataLog**/**Histogram** with `DoneAddr`: the microcontroller fills an array at the rate it needs and sets the flag; the program reads the whole array and clears the flag only after the read has completed, so the array is never overwritten while it is being read. In this mode the sample rate is limited only by the microcontroller.

## XML editor

**Widget Editor** edits the open configuration as a tree. Select a window, widget or list item to see its properties:

- Each property gets a control for its type: number fields, check boxes, color pickers, address fields (which accept `0x`, `#` and decimal and show the result), and drop-down lists for data types, modes and orientations. Hover a property name for a short description.
- **Add property** lists the properties the element can have but does not have yet, with their default values. Elements the loader ignores for that widget are shown in orange.
- **Add Widget** inserts a new widget into the selected container, after the selected widget, or at the end of the first window. **Add child widget**, **Add Line**, **Add Combo** and similar buttons fill containers and lists.
- **Save** writes the file and reloads the windows.

A missing property always falls back to a safe default, and out-of-range values (zero or negative sizes, reversed or empty ranges, `nan`, `inf`, unknown data types or modes) are corrected when the file is loaded, so a hand-written or partly edited file cannot crash the program.

**Caution:** writing memory changes the state of real hardware. Check addresses and configurations; do not use untested XML files with hardware connected.

## Design mode

**Windows → Design Mode** (or **Ctrl+E**) edits the widgets of the open configuration with the mouse. The old tree editor (**XML Editor**) stays available and works on the same document; both can be open at once.

- **Click** a widget to select it, or the empty part of a window to select the window. The innermost widget wins, so a click inside a group or child window selects the widget there; the parent is one click away in the Properties window or the right-click menu.
- **Drag** a widget to move it. A moved widget gets `<PosX>`/`<PosY>`: it is drawn at that point of its window (window coordinates, 0,0 is the top-left corner of the title bar) instead of after the previous widget, and the widgets around it close the gap. **Back to flow layout** in the right-click menu removes the position again. Positions snap to the grid (8 px by default); hold **Alt** to place freely. The tab above the top-left corner moves widgets whose area opens on a click, such as tree nodes and headers. Widgets inside a Grid stay in the grid.
- **Drag the orange handles** (right edge, bottom edge, corner) to resize. The new size goes into the widget's own properties: `Width`/`Height`, `Radius` (gauges, knobs, LEDs), `PixelSize` (LCD), `Scale` (8-bit control), `BandWidth` (equalizer) or `<ItemWidth>`, the new width of sliders, input fields, combo boxes and label texts.
- **Move and resize windows** as usual; in design mode their position and size are stored in `<Window>` as `PosX`, `PosY`, `Width` and `Height` and applied when the file is loaded.
- The **Properties** window shows every property of the selection with a control for its type: set properties in normal text, missing ones greyed out with their default value (editing creates them, **x** removes them). Plot lines, combo entries, zones and similar lists are edited in place; containers and windows list their widgets, which can be selected, reordered, deleted or added. Its toolbar adds widgets and windows, duplicates and deletes the selection, undoes and redoes, and saves.
- **Right-click** a widget for **Add widget here** (placed at the mouse), duplicate, reorder (later widgets are drawn on top), back to flow layout, select the parent and delete.
- Keys: arrows move the selection by a pixel (Shift: a grid step), **Del** deletes, **Ctrl+D** duplicates, **Ctrl+Z**/**Ctrl+Y** undo and redo (also changes made in the XML editor), **Ctrl+S** saves, **Esc** clears the selection or cancels a drag.

Every change is shown at once: the windows are rebuilt from the edited document, unsaved. Buttons, sliders, inputs and other controls do not react in design mode, so editing never writes to the target; values keep updating. Tree nodes, headers and tabs still open and close, to reach the widgets inside them.

## Icon

The program icon is drawn by `res/make_icon.py` (Python 3 with Pillow), which writes `res/app.ico` (16–256 px, linked into the executable through `app.rc`), a 256 px preview `res/app_icon.png` and `res/app_icon.h`, the pixels the program sets as its window icon and shows in **Help → About**.

## Demo

`examples/demo.xml` uses every widget the loader supports (65 types) in eight windows: Overview, Gauges, Controls, Plots, Layout, Memory, Media and Watch. Its variables are one struct at `0x20001000`; the offset map is at the top of the XML.

`examples/demo_firmware.c` is a matching firmware module for any Cortex-M project: call `demo_init()` once, `demo_tick_1khz()` from a 1 kHz timer interrupt and `demo_poll()` from the main loop. It animates the values (sine waves, gauges, a running light, the LCD) and feeds the DataLog and Histogram through the `DoneAddr` handshake. Reserve `0x2D0` bytes at `0x20001000` in the linker script, or move the struct and change the addresses in the XML.

For the **Media** window, copy `demo_logo.png` and `demo_spinner.gif` next to `J-Link Utils.exe`: Image and Gif load files from the program folder.

## Building on Windows

For the **full Release executable**, install Visual Studio 2022 C++ build tools,
a Windows SDK, Git, CMake 3.25+ and PowerShell 7.2+, then run:

```powershell
pwsh -File scripts/Build-WindowsApp.ps1 -Architecture x64
pwsh -File scripts/Build-WindowsApp.ps1 -Architecture x86
```

These commands build the pinned open-source dependencies, run their independent
link probes, then compile/link the full app with static CRT `/MT`. The package is
in `out/package/J-Link-Utils-<arch>-Release/`. The **Windows application** Actions
workflow produces the same x64/x86 packages. See [full build and runtime
instructions](docs/windows-app.md).

The exact supplied `vendor/jlink/JLink_x64.lib` and `JLink.lib` are static DLL-loader
wrappers, deliberately linked under their original names. `JLinkARM.lib` is a
different x86 import library, retained but not linked. All remain subject to
SEGGER licensing. **No proprietary runtime DLL is bundled or downloaded.** Supply
compatible `JLink_x64.dll` (x64) or `JLinkARM.dll` (x86) beside the exe, or install
the matching SEGGER software. Missing runtime produces an error and exit 2 rather
than attempting SDK operations. Hardware operation remains **not verified**.

The script selects `v143` on the command line; the IDE project's default `v145`
and C++17 settings are preserved. The supplied wrappers use Release `/MT`;
full `/MTd` Debug builds are not supported by this automation.

The separate [dependency build](docs/windows-dependencies.md) still builds/tests
both Debug and Release without J-Link or the full application:

```powershell
pwsh -File scripts/Build-WindowsDependencies.ps1 -Architecture x64
pwsh -File scripts/Build-WindowsDependencies.ps1 -Architecture x86
```

Generated dependency headers take precedence in the Visual Studio project.
Vendored SDL2 and FreeType headers in `include` remain untouched for the Linux
regression tests; Dear ImGui, ImPlot, tinyxml2, lfqueue and single-file code
are still compiled from the repository, not replaced with vcpkg copies.

## Checks without hardware

On Linux with Python 3, GCC/G++ and the standard tools:

```sh
sh tests/run_transport_tests.sh
python3 tests/ui_regression/run.py
python3 tests/widget_smoke/run.py
```

The tests compile the exchange code against simulated SDL and J-Link, use the real tinyxml2/lfqueue, and check individual widgets without a graphics window. They cover connection errors, memory bounds, block merging and polling, write-through, short reads and writes, plot sampling, the `DoneAddr` handshake, XML safety and formatting, with AddressSanitizer, UndefinedBehaviorSanitizer and leak checking enabled. The widget smoke test builds the real Dear ImGui and ImPlot and draws every widget loaded with no properties, with the editor's defaults and with hostile values in every property, inside every container, plus `examples/demo.xml`; it also draws the editor panel for every element, clicks, drags, right-clicks and uses the shortcuts of the design mode in every window (checking that nothing is written to the target meanwhile), and checks that the editor offers exactly the properties the loader reads. Any ImGui assertion fails it. These are regression checks, **not** a build of the full application and **not** a hardware test.

GitHub Actions runs all three test suites on every push and pull request; **Regression tests** can also be started manually.
