#Requires -Version 7.2
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$RepositoryRoot,
    [Parameter(Mandatory)][ValidateSet('x64', 'x86')][string]$Architecture
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[xml]$source = Get-Content -LiteralPath $Project -Raw
$ns = [Xml.XmlNamespaceManager]::new($source.NameTable)
$ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
$platform = if ($Architecture -eq 'x86') { 'Win32' } else { 'x64' }
$root = [IO.Path]::GetFullPath($RepositoryRoot).TrimEnd('\', '/').Replace('/', '\')
$destination = Join-Path $root ".dependencies/msbuild/$Architecture.props"
New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
$output = [Xml.XmlDocument]::new()
$uri = 'http://schemas.microsoft.com/developer/msbuild/2003'
$projectNode = $output.CreateElement('Project', $uri)
$output.AppendChild($projectNode) | Out-Null
function Add-Element($Parent, [string]$Name, [string]$Text) {
    $node = $output.CreateElement($Name, $uri)
    $node.InnerText = $Text
    $Parent.AppendChild($node) | Out-Null
    return $node
}
$properties = Add-Element $projectNode 'PropertyGroup' ''
Add-Element $properties 'JLinkDependencyRepoRoot' '$(MSBuildThisFileDirectory)..\..\' | Out-Null
foreach ($configuration in 'Debug', 'Release') {
    $groups = @($source.SelectNodes('/m:Project/m:ItemDefinitionGroup', $ns) | Where-Object {
        $_.GetAttribute('Condition') -eq "'`$(Configuration)|`$(Platform)'=='$configuration|$platform'"
    })
    if ($groups.Count -ne 1) { throw "Expected exactly one CMake settings group for $configuration|$platform" }
    $group = $groups[0]
    $result = Add-Element $projectNode 'ItemDefinitionGroup' ''
    $result.SetAttribute('Condition', "'`$(Configuration)|`$(Platform)'=='$configuration|$platform'")
    foreach ($section in 'ClCompile', 'Link') {
        $node = Add-Element $result $section ''
        $fields = if ($section -eq 'ClCompile') { @('AdditionalIncludeDirectories', 'PreprocessorDefinitions') }
                  else { @('AdditionalDependencies', 'AdditionalLibraryDirectories') }
        foreach ($field in $fields) {
            $valueNode = $group.SelectSingleNode("m:$section/m:$field", $ns)
            if ($null -eq $valueNode) {
                if ($field -in @('AdditionalDependencies', 'AdditionalIncludeDirectories')) { throw "CMake omitted $field" }
                continue
            }
            $values = @($valueNode.InnerText.Split(';') | Where-Object {
                $_ -and $_ -notmatch '^%\(' -and $_ -notmatch '^(WIN32|_WINDOWS|_DEBUG|NDEBUG|_MBCS)$' -and $_ -notmatch '^CMAKE_INTDIR='
            })
            # All third-party paths must come from this exact installation.
            if ($field -eq 'AdditionalDependencies') {
                foreach ($lib in $values) {
                    $normalized = $lib.Replace('/', '\')
                    if ([IO.Path]::IsPathRooted($normalized) -and -not $normalized.StartsWith("$root\.dependencies\installed\", [StringComparison]::OrdinalIgnoreCase)) {
                        throw "Unexpected non-installed link dependency: $lib"
                    }
                }
            }
            $text = ($values -join ';').Replace($root + '\', '$(JLinkDependencyRepoRoot)')
            # CMake may write forward slashes on some releases.
            $text = $text.Replace($root.Replace('\', '/') + '/', '$(JLinkDependencyRepoRoot)')
            if ($text -match '\$<') { throw "Unresolved CMake generator expression in $field" }
            Add-Element $node $field "$text;%($field)" | Out-Null
        }
    }
}
$settings = [Xml.XmlWriterSettings]::new()
$settings.Indent = $true
$settings.Encoding = [Text.UTF8Encoding]::new($false)
$writer = [Xml.XmlWriter]::Create($destination + '.tmp', $settings)
try { $output.Save($writer) } finally { $writer.Dispose() }
Move-Item -Force ($destination + '.tmp') $destination
Write-Host "Generated relocatable dependency settings: $destination"
