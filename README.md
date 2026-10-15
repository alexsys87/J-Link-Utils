# J-Link-Utils

A C++17 Windows application for watching and changing microcontroller memory through a SEGGER J-Link over SWD. The user interface is described in XML: numeric fields, plots, indicators, a memory editor and other widgets. A built-in XML editor changes the configuration.

## Connecting

1. Install the SEGGER J-Link software and driver; connect the probe and the target microcontroller.
2. In **Settings**, choose the **J-Link device** and set the **MCU Memory address** (default `0x20000000`).
3. Click **Connect**, then open an XML configuration with **File → Open**. The XML path can also be passed as the first command-line argument.

Widget addresses are **physical addresses**, not offsets. `#20000010`, `0x20000010` and decimal values are accepted. A leading `0` without `x` means octal, so use `#` or `0x` for hexadecimal addresses. The visible range is limited to 1 MiB from the selected base and to the end of the 32-bit address space.

Changing the base address or the target device stops the exchange; click **Connect** again afterwards. A failed or short read or write also stops it. Stopping clears the cache and the pending requests: writes from an old connection are never replayed after reconnecting.

How the exchange works: every visible widget subscribes to its addresses, and a separate thread re-reads all subscribed addresses continuously (at most once per millisecond). Neighbouring addresses in memory regions (code/SRAM `0x00000000–0x3FFFFFFF` and external RAM `0x60000000–0x9FFFFFFF`) are merged into blocks: gaps of up to 64 bytes are read along with them, and a block of up to 4 KiB takes one J-Link call. In peripheral and system space only the requested bytes are read, so registers with read side effects are not touched. Addresses no widget asked for during 0.5 s (closed window, minimized application) are no longer polled. A written value shows up in the widget immediately; several writes to the same address while the J-Link is busy are sent as one, with the last value.

### Plots

**PlotLines** gets timestamped values from the communication thread, one per polling cycle rather than one per frame, and thins them out to `Freq` (up to 1000 Hz). The X axis is real time, and the history holds the whole `Time Window`. The reachable rate depends on the probe: a J-Link BASE V8 (USB full speed) needs about 1–2 ms per call, so expect roughly 300–700 points per second when all plotted and displayed variables fit into one or two blocks; addresses scattered over memory share that rate.

For faster signals use **DataLog**/**Histogram** with `DoneAddr`: the microcontroller fills an array at the rate it needs and sets the flag; the program reads the whole array and clears the flag only after the read has completed, so the array is never overwritten while it is being read. In this mode the sample rate is limited only by the microcontroller.

**Caution:** writing memory changes the state of real hardware. Check addresses and configurations; do not use untested XML files with hardware connected.

## Demo

`examples/demo.xml` uses every widget the loader supports (65 types) in eight windows: Overview, Gauges, Controls, Plots, Layout, Memory, Media and Watch. Its variables are one struct at `0x20001000`; the offset map is at the top of the XML.

`examples/demo_firmware.c` is a matching firmware module for any Cortex-M project: call `demo_init()` once, `demo_tick_1khz()` from a 1 kHz timer interrupt and `demo_poll()` from the main loop. It animates the values (sine waves, gauges, a running light, the LCD) and feeds the DataLog and Histogram through the `DoneAddr` handshake. Reserve `0x2D0` bytes at `0x20001000` in the linker script, or move the struct and change the addresses in the XML.

For the **Media** window, copy `demo_logo.png` and `demo_spinner.gif` next to `J-Link Utils.exe`: Image and Gif load files from the program folder.

## Building on Windows

Open `J-Link Utils.sln` in Visual Studio with the C++ and Windows SDK components. The project uses C++17 and the `v145` toolset; that toolset must be installed, or the project must be retargeted.

**The repository is not yet self-contained for a full build.** Missing:

- the Dear ImGui sources in `include/imgui` (including the SDL2 backend and the FreeType renderer);
- the SDL2 and FreeType libraries and their dependencies listed in the `.vcxproj`, in `lib`;
- the J-Link import library: `JLinkARM_x64.lib` for x64 or `JLinkARM.lib` for Win32, and the matching DLL to run.

Third-party headers are in `include`: `SDL2`, `tinyxml2`, `ifqueue` (lfqueue), `JLinkSDK`, `implot` and single-file headers (`stb_image.h`, `gif_load.h`, `noc_file_dialog.h` and others). `include/freetype2` contains only the FreeType headers (`ft2build.h` and `freetype/`).

The libraries must match the architecture, the Debug/Release configuration and the static CRT (`/MTd` or `/MT`). Distribute the J-Link SDK according to the SEGGER license. A full Windows build and operation with a physical J-Link are **not verified**.

## Checks without hardware

On Linux with Python 3, GCC/G++ and the standard tools:

```sh
sh tests/run_transport_tests.sh
python3 tests/ui_regression/run.py
```

The tests compile the exchange code against simulated SDL and J-Link, use the real tinyxml2/lfqueue, and check individual widgets without a graphics window. They cover connection errors, memory bounds, block merging and polling, write-through, short reads and writes, plot sampling, the `DoneAddr` handshake, XML safety and formatting, with AddressSanitizer, UndefinedBehaviorSanitizer and leak checking enabled. These are regression checks, **not** a build of the full application and **not** a hardware test.

GitHub Actions runs both test suites on every push and pull request; **Regression tests** can also be started manually.
