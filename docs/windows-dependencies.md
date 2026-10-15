# Reproducible Windows open-source dependencies

This builds **only the missing open-source libraries**. It never downloads,
generates or links a SEGGER J-Link import library and does not build the full
application. Dear ImGui, ImPlot, tinyxml2, lfqueue and single-file libraries
already in the repository continue to be compiled as application sources.

## Prerequisites and commands

- Windows x64, PowerShell **7.2 or newer**, Git, CMake **3.25 or newer** (with
  `ctest`), Windows `tar` and internet access.
- Visual Studio 2022 / Build Tools: **Desktop development with C++**, x64/x86
  MSVC v143 tools and a Windows 10/11 SDK. CMake discovers the installed Visual
  Studio instance; no Developer Prompt, administrator rights, winget or global
  `vcpkg integrate install` is needed. Use the VS Installer to obtain missing
  prerequisites; the script does not change the machine's installed software.

From the repository root (paths containing spaces are supported):

```powershell
pwsh -File scripts/Build-WindowsDependencies.ps1 -Architecture x64
pwsh -File scripts/Build-WindowsDependencies.ps1 -Architecture x86
```

Each invocation installs **both Debug and Release** via the standard
`x64-windows-static` or `x86-windows-static` triplet, which selects static
libraries and `/MTd` / `/MT`. An alternate installed Visual Studio CMake
generator can be selected with `-Generator 'Visual Studio 18 2026'`; changing
generators requires removing `.dependencies/tool-build` and the corresponding
`.dependencies/probe-*` directory first. Only VS 2022 is selected in CI.

The application retains its original **v145** toolset and all unrelated
compiler/linker options. Install that toolset to build it, or deliberately
retarget it yourself. Dependencies produced by VS 2022/v143 are intended for
MSVC's binary-compatible newer toolsets with the same static CRT choice, not
MinGW or `/MD` consumers. For a strictly matching compiler, run the dependency
script using your application's installed VS generator. Neither an application
link nor hardware operation is established by the dependency-only job.

## Pins and bootstrap integrity

`vcpkg.json` fixes the builtin baseline to vcpkg **2025.01.13**, commit
`6f29f12e82a8293156836ad81cc9bf5af41fe836`. That tree fixes the versions,
port revisions, upstream download hashes, patches and host helper ports.
SDL2 has no default features enabled; FreeType explicitly enables Brotli,
bzip2, PNG and zlib. Brotli, bzip2, libpng and zlib are also explicit manifest
dependencies, so all requested open-source packages are installed.

`build/dependencies/vcpkg-lock.json` pins the bootstrap checkout and tool
**2025-01-11**. Rather than run the upstream Windows bootstrap's un-hashed
executable download, the script downloads the tool's source archive, verifies
its **SHA512 before extraction**, and builds it with CMake/MSVC. The hash is
copied from `scripts/vcpkg-tool-metadata.txt` in the pinned vcpkg tree. The
tool source itself hash-pins its fmt 11.0.2 and cmrc 2.0.1 bootstrap downloads.
No mutable latest executable is trusted. The checked-out vcpkg HEAD is
validated and tracked port/toolchain modifications are rejected on reuse.

The checkout, source build, packages and tool downloads live entirely under
`.dependencies/`. A subsequent invocation reuses build outputs and vcpkg ABI
caches but regenerates the integration settings. Set
`VCPKG_DEFAULT_BINARY_CACHE` to an existing directory to reuse package archives
between clean checkouts. Source pins do not freeze your compiler, SDK or OS;
CI records CMake's cache and tool versions for provenance.

To upgrade: choose a reviewed vcpkg release commit, update both baseline and
lock commit, read that tree's tool metadata and update the tool source version,
commit and SHA512 together. Remove `.dependencies/` (or use a clean checkout),
then run both architectures in CI. Never replace a failed hash with the hash
of an unexplained download.

## How headers and libraries stay in sync

1. The install root is `.dependencies/installed/<triplet>/`. Headers are under
   `include/`, Release libraries under `lib/`, Debug under `debug/lib/`, and
   licenses/package CMake metadata under `share/`. No files are copied into the
   old vendored `include/` or `lib/` directories.
2. A small dependency-only CMake executable consumes `SDL2::SDL2main`,
   `SDL2::SDL2-static`, `Freetype::Freetype`, `PNG::PNG`, `BZip2::BZip2` and
   `ZLIB::ZLIB`, plus the port's `unofficial::brotli::brotlidec` and
   `unofficial::brotli::brotlienc` targets. Imported targets resolve the concrete library filenames,
   configuration suffixes, Brotli and other transitive dependencies, and SDL's
   Windows system libraries. The probe initializes SDL's timer and FreeType,
   checks the codec versions, and exits without opening a window or hardware.
3. `Export-DependencyProps.ps1` reads CMake's generated VS project and exports
   its resolved include paths, compile definitions, libraries and library
   search paths into `.dependencies/msbuild/x64.props` or `x86.props`. It
   rejects unexpected absolute library paths and unresolved generator
   expressions. Paths into this checkout become relocatable MSBuild paths.
   This avoids guessing names: in the pinned SDL2 port, for example, Release
   `SDL2main.lib` and Debug **`SDL2maind.lib`** live in `lib/manual-link` and
   `debug/lib/manual-link`, not the application's old library directories.
4. The application imports `build/dependencies/Dependencies.props` after its
   own settings. Dependency `/I` paths precede the old `IncludePath`, so the
   generated SDL2/FreeType headers match the installed libraries. Its stale
   open-source `.lib` list is removed, but J-Link and unrelated project settings
   are retained. Global vcpkg integration is disabled for this project to avoid
   mixing installations. Linux test commands are unchanged and still use their
   vendored headers/stubs.
5. Both the CMake probe and an independent plain MSBuild probe consuming the
   **exported** `.props` are built and run in Debug and Release. This checks the
   exported transitive static link closure as well as the installed packages.
   Neither probe uses the J-Link SDK or the main application.

If the generated `.props` are missing, the application reports the explicit
preparation command. The app does not silently download dependencies in a
pre-build event. Rerun the script after changing a manifest, pin or build script.

## CI, cache and artifacts

`.github/workflows/windows-dependencies.yml` uses `windows-2022` and separate
x64/x86 jobs, each building/testing Debug and Release. The existing Linux
regression workflow is unchanged. Dependency CI triggers on dependency-related
push/PR changes and manually through **Run workflow**.

Only vcpkg's content-addressed binary package archives are cached. Cache keys
include the manifest/lock/scripts; vcpkg's own ABI keys additionally account
for compiler, SDK and port inputs. No installed tree, generated project or
stale `.props` is restored as a build cache.

Each successful job uploads `windows-dependencies-<arch>-static-debug-release`
with the installation, generated `.props`, lock/manifest and provenance files,
including package license notices under `share/`. Treat artifacts as build
inputs, not a complete app distribution. To use an artifact with the same
checkout, extract its `installed/` and `msbuild/` folders **under
`.dependencies/`**. Regenerate locally if you change pins or compiler settings;
artifact `CMakeCache-*.txt` records the producing runner's absolute paths and
is informational, not meant for reuse as a local CMake cache.

## Add J-Link later, separately

After obtaining the import library under the SEGGER license, use the existing
project search directories:

| Project configuration | Import library path |
| --- | --- |
| x64 Release | `lib/x64/JLinkARM_x64.lib` |
| x64 Debug | `lib/x64/debug/JLinkARM_x64.lib` |
| Win32 Release | `lib/x86/JLinkARM.lib` |
| Win32 Debug | `lib/x86/debug/JLinkARM.lib` |

The same vendor import library can normally be copied to both configuration
directories for its architecture; it is a DLL import library, not one of the
static CRT libraries built here. Obtain and deploy the matching architecture's
J-Link DLL next to the application executable (or through an appropriate DLL
search path), and install the SEGGER drivers as required. Do not upload
proprietary libraries to this dependency workflow's artifacts.

**Validation boundary:** the dependency workflow is designed to compile, link
and run only the two dependency probes. It does not verify the full GUI app,
J-Link ABI/runtime compatibility or operation against physical hardware.
