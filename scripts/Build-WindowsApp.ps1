#Requires -Version 7.2
[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86')][string]$Architecture = 'x64',
    [ValidatePattern('^v[0-9]+$')][string]$PlatformToolset = 'v143',
    [string]$Generator = 'Visual Studio 17 2022'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'Windows and Visual Studio C++ build tools are required.' }
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$state = Join-Path $repo '.dependencies'

# Check the exact uploaded bytes before building or linking any proprietary code.
$inventory = Get-Content "$repo/vendor/jlink/binaries.json" -Raw | ConvertFrom-Json
foreach ($binary in $inventory) {
    $path = Join-Path "$repo/vendor/jlink" $binary.file
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing supplied library: $path" }
    if ((Get-Item -LiteralPath $path).Length -ne $binary.bytes -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $binary.sha256) {
        throw "Supplied library integrity check failed: $path"
    }
}

# Keep the independently validated dependency build/probes as the single source
# of truth for package versions, CRT flags, includes and transitive libraries.
& "$PSScriptRoot/Build-WindowsDependencies.ps1" -Architecture $Architecture -Generator $Generator
$probe = Join-Path $state "probe-$Architecture"
$msbuild = (Get-Content "$probe/selected-msbuild.txt" -Raw).Trim()
$selectedToolset = (Get-Content "$probe/selected-toolset.txt" -Raw).Trim()
if ($PlatformToolset -ne $selectedToolset) {
    throw "App toolset $PlatformToolset differs from dependency toolset $selectedToolset. Select a matching Generator/PlatformToolset."
}
if (-not (Test-Path -LiteralPath $msbuild -PathType Leaf)) { throw 'CMake-selected MSBuild is missing.' }
$platform = if ($Architecture -eq 'x86') { 'Win32' } else { 'x64' }
$output = Join-Path $repo "out/app/$Architecture/Release"
$objects = Join-Path $repo "out/obj/$Architecture/Release"
New-Item -ItemType Directory -Force $output, $objects | Out-Null
& $msbuild "$repo/J-Link Utils.vcxproj" /m /t:Rebuild /verbosity:minimal `
    /p:Configuration=Release "/p:Platform=$platform" "/p:PlatformToolset=$PlatformToolset" `
    "/p:OutDir=$output/" "/p:IntDir=$objects/" `
    "/bl:$output/build.binlog"
if ($LASTEXITCODE -ne 0) { throw "Full application build failed with exit code $LASTEXITCODE" }
$exe = Join-Path $output 'J-Link Utils.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'MSBuild did not produce the expected application executable.' }

$package = Join-Path $repo "out/package/J-Link-Utils-$Architecture-Release"
if (Test-Path $package) { Remove-Item -Recurse -Force $package }
New-Item -ItemType Directory -Force $package | Out-Null
Copy-Item -LiteralPath $exe -Destination $package
foreach ($assets in 'examples', 'res') {
    Copy-Item -Recurse -LiteralPath "$repo/$assets" -Destination $package
}
# Image/Gif widgets resolve these demo filenames relative to the executable,
# not relative to the XML file in examples/.
Copy-Item -LiteralPath "$repo/examples/demo_logo.png", "$repo/examples/demo_spinner.gif" -Destination $package
Copy-Item -LiteralPath "$repo/README.md" -Destination $package
Copy-Item -LiteralPath "$repo/docs/windows-app.md" -Destination "$package/BUILD-AND-RUNTIME.md"
Copy-Item -LiteralPath "$repo/vendor/jlink/binaries.json" -Destination "$package/jlink-build-inputs.json"
# Keep installed package license notices alongside the statically linked exe.
Copy-Item -Recurse -LiteralPath "$state/installed/$Architecture-windows-static/share" -Destination "$package/third-party-notices"
Copy-Item -LiteralPath "$repo/include/imgui/LICENSE.txt" -Destination "$package/third-party-notices/imgui-LICENSE.txt"
Copy-Item -LiteralPath "$repo/include/implot/LICENSE" -Destination "$package/third-party-notices/implot-LICENSE.txt"
# These vendored single-file libraries carry their notices in the source itself.
$sourceNotices = Join-Path $package 'third-party-notices/vendored-sources'
New-Item -ItemType Directory -Force $sourceNotices | Out-Null
foreach ($source in 'stb_image.h', 'gif_load.h', 'noc_file_dialog.h', 'tinyxml2/tinyxml2.h', 'ifqueue/lfqueue.h') {
    Copy-Item -LiteralPath "$repo/include/$source" -Destination $sourceNotices
}
Get-FileHash -LiteralPath $exe -Algorithm SHA256 | Format-List | Out-File "$package/executable-sha256.txt"
Write-Host "Full $Architecture Release application: $exe"
Write-Host "Package (no proprietary DLL): $package"
