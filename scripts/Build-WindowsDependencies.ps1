#Requires -Version 7.2
[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86')][string]$Architecture = 'x64',
    [string]$Generator = 'Visual Studio 17 2022'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'This script requires Windows and Visual Studio C++ build tools.' }

function Invoke-Native([string]$File, [string[]]$Arguments) {
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}
function Assert-Hash([string]$Path, [string]$Expected) {
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA512).Hash -ne $Expected) {
        throw "SHA512 mismatch: $Path. Remove it and retry; never execute an unverified download."
    }
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$state = Join-Path $repo '.dependencies'
$vcpkg = Join-Path $state 'vcpkg'
$installed = Join-Path $state 'installed'
$lock = Get-Content (Join-Path $repo 'build/dependencies/vcpkg-lock.json') -Raw | ConvertFrom-Json
$manifest = Get-Content (Join-Path $repo 'vcpkg.json') -Raw | ConvertFrom-Json
if ($manifest.'builtin-baseline' -ne $lock.commit) { throw 'Manifest baseline and bootstrap commit must match.' }
foreach ($tool in 'git', 'cmake', 'ctest', 'tar') { Get-Command $tool -ErrorAction Stop | Out-Null }
New-Item -ItemType Directory -Force $state | Out-Null

if (-not (Test-Path "$vcpkg/.git")) {
    New-Item -ItemType Directory -Force $vcpkg | Out-Null
    Invoke-Native git @('init', $vcpkg)
    Invoke-Native git @('-C', $vcpkg, 'remote', 'add', 'origin', $lock.repository)
    Invoke-Native git @('-C', $vcpkg, 'fetch', '--depth=1', 'origin', $lock.commit)
    Invoke-Native git @('-C', $vcpkg, 'checkout', '--detach', $lock.commit)
}
$head = & git -C $vcpkg rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $head -ne $lock.commit) { throw 'Unexpected vcpkg checkout. Remove .dependencies/vcpkg and retry.' }
Invoke-Native git @('-C', $vcpkg, 'diff', '--exit-code', 'HEAD', '--', 'scripts', 'ports', 'versions', 'triplets')

# The upstream Windows bootstrap downloads an unhashed executable. Instead build
# the tool from its upstream SHA512-pinned source archive. Its fmt/cmrc downloads
# are also hash-pinned by that source. No winget, global integration, or mutable exe.
# Upstream VCPKG_TOOL_SOURCE_SHA describes the tag ZIP, not the tar.gz.
$archive = Join-Path $state "vcpkg-tool-$($lock.toolVersion).zip"
if (-not (Test-Path $archive)) { Invoke-WebRequest -Uri $lock.toolSourceUrl -OutFile $archive }
Assert-Hash $archive $lock.toolSourceSha512
$toolSource = Join-Path $state "tool-source-$($lock.toolVersion)"
# An older bootstrap cache can still point at the January tool sources.
# Discard only that build directory when the source version changes; retain
# downloads, installed libraries and the vcpkg binary cache.
$previousToolBuild = Join-Path $state 'tool-build'
$previousToolCache = Join-Path $previousToolBuild 'CMakeCache.txt'
if (Test-Path -LiteralPath $previousToolCache) {
    $sourceEntry = @(Get-Content -LiteralPath $previousToolCache | Where-Object { $_ -match '^CMAKE_HOME_DIRECTORY:INTERNAL=' })
    if ($sourceEntry.Count -ne 1 -or
        [IO.Path]::GetFullPath(($sourceEntry[0] -split '=', 2)[1]) -ine [IO.Path]::GetFullPath($toolSource)) {
        Write-Host 'Removing outdated vcpkg-tool build cache (tool source version changed).'
        Remove-Item -LiteralPath $previousToolBuild -Recurse -Force
    }
}
if (-not (Test-Path $toolSource)) {
    New-Item -ItemType Directory $toolSource | Out-Null
    try { Invoke-Native tar @('-xf', $archive, '--strip-components=1', '-C', $toolSource) }
    catch { Remove-Item -Recurse -Force $toolSource; throw }
}
$toolBuild = Join-Path $state 'tool-build'
$toolConfigureArguments = @('-S', $toolSource, '-B', $toolBuild, '-G', $Generator, '-A', 'x64',
    '-DBUILD_TESTING=OFF', '-DVCPKG_DEVELOPMENT_WARNINGS=OFF', '-DVCPKG_MSVC_ANALYZE=OFF',
    "-DVCPKG_BASE_VERSION=$($lock.toolVersion)", "-DVCPKG_VERSION=$($lock.toolCommit)")
# The pinned vcpkg-tool embeds CMakeRC with a pre-3.5 policy baseline.
# CMake 4 rejects that baseline. Apply the supported compatibility override only
# to this bootstrap configure (including its FetchContent subdirectories), not
# globally to vcpkg ports or the application. 3.10 also avoids CMake 4's warning
# about policy compatibility older than 3.10. Keep the hash-pinned sources intact.
$cmakeCapabilitiesJson = & cmake -E capabilities
if ($LASTEXITCODE -ne 0) { throw "cmake -E capabilities failed with exit code $LASTEXITCODE" }
$cmakeCapabilities = ($cmakeCapabilitiesJson -join "`n") | ConvertFrom-Json
if ([int]$cmakeCapabilities.version.major -ge 4) {
    $toolConfigureArguments += '-DCMAKE_POLICY_VERSION_MINIMUM=3.10'
}
Invoke-Native cmake $toolConfigureArguments
Invoke-Native cmake @('--build', $toolBuild, '--config', 'Release', '--target', 'vcpkg', '--parallel')
Copy-Item -Force "$toolBuild/Release/vcpkg.exe" "$vcpkg/vcpkg.exe"
New-Item -ItemType File -Force "$vcpkg/vcpkg.disable_metrics" | Out-Null

$triplet = "$Architecture-windows-static"
$env:VCPKG_ROOT = $vcpkg
$env:VCPKG_DISABLE_METRICS = '1'
# Keep vcpkg on the same VS instance selected by CMake, even on a machine
# with several Visual Studio generations installed.
$instanceEntry = Get-Content "$toolBuild/CMakeCache.txt" | Where-Object { $_ -match '^CMAKE_GENERATOR_INSTANCE:INTERNAL=' }
if (@($instanceEntry).Count -ne 1) { throw 'Cannot locate the CMake-selected Visual Studio instance.' }
$env:VCPKG_VISUAL_STUDIO_PATH = ($instanceEntry -split '=', 2)[1]
if (-not $env:VCPKG_VISUAL_STUDIO_PATH) { throw 'CMake did not record a Visual Studio instance.' }
# The pinned MSYS bootstrap references packages removed from rolling mirrors.
# Build native pkgconf first instead: its Meson port uses NO_PKG_CONFIG, so this
# bootstrap does not itself need pkg-config or an MSYS runtime. Keep it separate
# from the target install to avoid mixing x64 host tools with x86 libraries.
$hostManifestRoot = Join-Path $repo 'build/dependencies/host-tools'
$hostManifest = Get-Content "$hostManifestRoot/vcpkg.json" -Raw | ConvertFrom-Json
if ($hostManifest.'builtin-baseline' -ne $lock.commit) { throw 'Host-tool baseline and bootstrap commit must match.' }
$hostInstalled = Join-Path $state 'host-installed'
Invoke-Native "$vcpkg/vcpkg.exe" @('install', '--triplet=x64-windows-static', '--host-triplet=x64-windows',
    "--x-manifest-root=$hostManifestRoot", "--x-install-root=$hostInstalled", '--clean-after-build')
$env:PKG_CONFIG = Join-Path $hostInstalled 'x64-windows-static/tools/pkgconf/pkgconf.exe'
if (-not (Test-Path $env:PKG_CONFIG)) { throw 'Native pkgconf bootstrap did not produce pkgconf.exe.' }
Invoke-Native $env:PKG_CONFIG @('--version')
# vcpkg sanitizes port environments. This pinned host utility only validates
# pkg-config metadata; preserve the explicit path rather than acquiring MSYS.
# KEEP_ENV_VARS is intentionally not ABI-tracked: the absolute checkout path
# must not invalidate caches; the utility source is pinned by the same baseline.
$env:VCPKG_KEEP_ENV_VARS = (@($env:VCPKG_KEEP_ENV_VARS -split ';' | Where-Object { $_ }) + 'PKG_CONFIG' | Select-Object -Unique) -join ';'
# Both Debug and Release are installed by the standard static triplet (/MTd,/MT).
Invoke-Native "$vcpkg/vcpkg.exe" @('install', "--triplet=$triplet", '--host-triplet=x64-windows',
    "--x-manifest-root=$repo", "--x-install-root=$installed", '--clean-after-build')

$platform = if ($Architecture -eq 'x86') { 'Win32' } else { 'x64' }
$build = Join-Path $state "probe-$Architecture"
Invoke-Native cmake @('-S', "$repo/build/dependencies", '-B', $build, '-G', $Generator, '-A', $platform,
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkg/scripts/buildsystems/vcpkg.cmake", "-DVCPKG_TARGET_TRIPLET=$triplet",
    "-DCMAKE_GENERATOR_INSTANCE=$env:VCPKG_VISUAL_STUDIO_PATH",
    "-DVCPKG_INSTALLED_DIR=$installed", '-DVCPKG_MANIFEST_INSTALL=OFF')
foreach ($configuration in 'Debug', 'Release') {
    Invoke-Native cmake @('--build', $build, '--config', $configuration, '--parallel')
    Invoke-Native ctest @('--test-dir', $build, '-C', $configuration, '--output-on-failure')
}

# Export the exact link closure resolved by CMake's VS generator. This includes
# SDL2maind's manual-link directory and static SDL2's Windows system libraries.
# Do not glob *.lib or maintain a second list of debug suffixes by hand.
& "$PSScriptRoot/Export-DependencyProps.ps1" -Project "$build/dependency_probe.vcxproj" `
    -RepositoryRoot $repo -Architecture $Architecture
# Independently verify that the exported MSBuild properties compile/link/run,
# not just that CMake's original project does. Still no J-Link or application.
$msbuild = (Get-Content "$build/selected-msbuild.txt" -Raw).Trim()
if (-not $msbuild -or -not (Test-Path -LiteralPath $msbuild -PathType Leaf)) {
    throw 'Cannot locate the CMake-selected MSBuild exported by the Visual Studio generator.'
}
# A standalone hand-written vcxproj otherwise falls back to v100. Match the
# dependency build's selected compiler without changing the application toolset.
$toolset = (Get-Content "$build/selected-toolset.txt" -Raw).Trim()
if ($toolset -notmatch '^v[0-9]+$') { throw "Unexpected CMake-selected platform toolset: $toolset" }
foreach ($configuration in 'Debug', 'Release') {
    Invoke-Native $msbuild @("$repo/build/dependencies/props-probe.vcxproj", '/m',
        "/p:Configuration=$configuration", "/p:Platform=$platform", "/p:PlatformToolset=$toolset")
    Invoke-Native "$state/props-probe/$platform/$configuration/dependency_props_probe.exe" @()
}
Write-Host "Dependencies ready for $platform Debug and Release. No J-Link library or application was built."
