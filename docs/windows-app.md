# Full Windows executable build

Prerequisites: Windows, PowerShell 7.2+, Git, CMake, and Visual Studio (2022 or 2026)
Desktop development with C++ and Windows SDK. From repository root:

```powershell
pwsh -File scripts/Build-WindowsApp.ps1 -Architecture x64
pwsh -File scripts/Build-WindowsApp.ps1 -Architecture x86
```

By default, the script automatically detects the installed Visual Studio version
(VS 2022 or VS 2026), selects the appropriate generator and platform toolset (`v143`
or `v145`), cleans outdated CMake build caches if the Visual Studio version changed,
automatically builds/installs missing dependencies, and compresses the final executable
using UPX (`upx --best`) if UPX is available or installed via winget.

Each command builds and tests the pinned open-source dependencies, then compiles
and links the **complete application**, using Release `/MT` and the CMake-selected
MSBuild. It packages `out/package/J-Link-Utils-<arch>-Release/`. The executable and
MSBuild binary log also remain in `out/app/<arch>/Release/`.
Other VS generators/toolsets can be explicitly selected using
`-Generator` and `-PlatformToolset`; the script rejects a compiler-toolset mismatch
with its dependencies.

The separate **Windows application** workflow builds x64 and x86 Release on PRs,
pushes to `main`, and manual dispatch. Download `J-Link-Utils-x64-Release` or
`J-Link-Utils-x86-Release` from that run's artifacts. It shares only ABI-addressed
vcpkg binary archives with the dependency-only workflow, not installed trees or
generated props. Later failures still preserve completed package archives.

## Supplied J-Link libraries: deliberate wrapper linkage

`vendor/jlink/` contains the three original user-supplied binaries, byte-for-byte;
`binaries.json` records their size, SHA256 and inspected role. They are proprietary
SEGGER inputs, **not open-source dependencies**. Their presence is not a grant of
redistribution rights; applicable SEGGER terms still govern their use/distribution.
The build script verifies the supplied bytes before linking.

* x64 links **JLink_x64.lib**, a static loader wrapper, under its original name.
* x86 links **JLink.lib**, the corresponding static loader wrapper.
* **JLinkARM.lib** is a true x86 import library, preserved but not linked. It is
  not renamed to simulate an x64 library.

The vendored `JLinkARMDLL.h` intentionally declares its API **without dllimport**
and documents loader-wrapper linkage. Both selected archives contain one COFF
object, direct `JLINKARM_*` entry points and `JLINKARMDLL_LIB_LoadLibrary`; their
linker directives select `LIBCMT` (Release `/MT`). Startup explicitly calls that
cdecl loader (0 success, -1 failure) before any SDK call/thread. The wrappers then
resolve DLL entry points dynamically. No fabricated import library, downloaded
proprietary DLL, or suppressed CRT conflict is involved.

Full app automation intentionally supports **Release only**, matching the supplied
wrappers' CRT. The dependency-only workflow continues to validate `/MTd` Debug and
`/MT` Release. Matching Debug vendor wrappers would be needed before claiming a
supported full `/MTd` application build.

## DLL supplied separately at runtime

No SEGGER DLL is bundled or downloaded by these builds. Before normal use, place
the licensed, compatible **JLink_x64.dll** (x64) or **JLinkARM.dll** (x86) beside
`J-Link Utils.exe`, or use the matching SEGGER installation recognized by the
wrapper's registry search. Do not rename a DLL of the wrong architecture. The
names and executable-relative/registry lookup come from the supplied wrappers.

Missing/unloadable DLL: the app presents a clear error and exits **2**, before
accessing SDK APIs. `"J-Link Utils.exe" --check-jlink-runtime` performs only this
loader check, without a GUI dialog: exit 0 loaded, exit 2 missing/unloadable.
CI checks exit 2 with no proprietary DLL installed. This is a startup smoke test,
**not** a GUI, SDK-version-compatibility, device, or hardware test.

The package includes the complete `examples/` tree and `res/`, plus the demo's
PNG/GIF next to the executable, where the widgets resolve these filenames.
The executable icon is embedded by `app.rc`. Fonts are loaded
from Windows Fonts with the application's existing fallback. `settings.ini`,
`window.ini` and optional `custom.style.xml` are user state, not required repository
inputs; run from a writable extracted folder. For the demo, launch from that folder
with `"J-Link Utils.exe" examples/demo.xml` after installing the runtime DLL.
