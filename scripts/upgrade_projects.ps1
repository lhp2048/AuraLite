# Regenerates AuraLite VS2022 projects: Win32+x64, v143, C++11, /MD, Win7+
# Static libs for modules; AuraLite.dll aggregates them.
$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Write-Host "AuraLite root: $Root"

function Get-ClCompileIncludes([string]$vcxproj) {
  [xml]$xml = Get-Content $vcxproj -Encoding UTF8
  $ns = @{msb = "http://schemas.microsoft.com/developer/msbuild/2003"}
  $nodes = Select-Xml -Xml $xml -XPath "//msb:ClCompile[@Include]" -Namespace $ns
  $list = @()
  foreach ($n in $nodes) {
    $inc = $n.Node.GetAttribute("Include")
    $list += $inc
  }
  return $list
}

function Get-ClIncludeIncludes([string]$vcxproj) {
  [xml]$xml = Get-Content $vcxproj -Encoding UTF8
  $ns = @{msb = "http://schemas.microsoft.com/developer/msbuild/2003"}
  $nodes = Select-Xml -Xml $xml -XPath "//msb:ClInclude[@Include]" -Namespace $ns
  $list = @()
  foreach ($n in $nodes) { $list += $n.Node.GetAttribute("Include") }
  return $list
}

function Write-StaticLibProject {
  param(
    [string]$Name,
    [string]$Guid,
    [string]$Dir,
    [string[]]$Sources,
    [string[]]$Headers,
    [string]$ExtraPreprocessor = ""
  )
  $projPath = Join-Path $Dir "$Name.vcxproj"
  $outLib = "`$(AuraLiteRoot)lib\`$(Platform)\`$(Configuration)\$Name.lib"
  $sb = New-Object System.Text.StringBuilder
  [void]$sb.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
  [void]$sb.AppendLine('<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')
  [void]$sb.AppendLine('  <ItemGroup Label="ProjectConfigurations">')
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      [void]$sb.AppendLine("    <ProjectConfiguration Include=`"$cfg|$plat`">")
      [void]$sb.AppendLine("      <Configuration>$cfg</Configuration>")
      [void]$sb.AppendLine("      <Platform>$plat</Platform>")
      [void]$sb.AppendLine("    </ProjectConfiguration>")
    }
  }
  [void]$sb.AppendLine('  </ItemGroup>')
  [void]$sb.AppendLine('  <PropertyGroup Label="Globals">')
  [void]$sb.AppendLine("    <VCProjectVersion>17.0</VCProjectVersion>")
  [void]$sb.AppendLine("    <ProjectGuid>{$Guid}</ProjectGuid>")
  [void]$sb.AppendLine("    <Keyword>Win32Proj</Keyword>")
  [void]$sb.AppendLine("    <RootNamespace>$Name</RootNamespace>")
  [void]$sb.AppendLine("    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>")
  [void]$sb.AppendLine('  </PropertyGroup>')
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />')
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      $useDbg = if ($cfg -eq "Debug") { "true" } else { "false" }
      [void]$sb.AppendLine("  <PropertyGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`" Label=`"Configuration`">")
      [void]$sb.AppendLine("    <ConfigurationType>StaticLibrary</ConfigurationType>")
      [void]$sb.AppendLine("    <UseDebugLibraries>$useDbg</UseDebugLibraries>")
      [void]$sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>")
      [void]$sb.AppendLine("    <CharacterSet>Unicode</CharacterSet>")
      if ($cfg -eq "Release") {
        [void]$sb.AppendLine("    <WholeProgramOptimization>true</WholeProgramOptimization>")
      }
      [void]$sb.AppendLine("  </PropertyGroup>")
    }
  }
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />')
  [void]$sb.AppendLine('  <Import Project="$(ProjectDir)..\AuraLite.Common.props" />')
  [void]$sb.AppendLine('  <PropertyGroup>')
  [void]$sb.AppendLine('    <OutDir>$(AuraLiteRoot)lib\$(Platform)\$(Configuration)\</OutDir>')
  [void]$sb.AppendLine('    <IntDir>$(AuraLiteRoot)obj\$(Platform)\$(Configuration)\$(ProjectName)\</IntDir>')
  [void]$sb.AppendLine('    <TargetName>$(ProjectName)</TargetName>')
  [void]$sb.AppendLine('  </PropertyGroup>')
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      $defs = "WIN32;_LIB;AURALITE_IMPLEMENTATION"
      if ($cfg -eq "Debug") { $defs = "_DEBUG;$defs" } else { $defs = "NDEBUG;$defs" }
      if ($ExtraPreprocessor) { $defs = "$defs;$ExtraPreprocessor" }
      [void]$sb.AppendLine("  <ItemDefinitionGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`">")
      [void]$sb.AppendLine("    <ClCompile>")
      [void]$sb.AppendLine("      <PreprocessorDefinitions>$defs;%(PreprocessorDefinitions)</PreprocessorDefinitions>")
      [void]$sb.AppendLine("    </ClCompile>")
      [void]$sb.AppendLine("  </ItemDefinitionGroup>")
    }
  }
  if ($Headers -and $Headers.Count -gt 0) {
    [void]$sb.AppendLine('  <ItemGroup>')
    foreach ($h in $Headers) {
      [void]$sb.AppendLine("    <ClInclude Include=`"$h`" />")
    }
    [void]$sb.AppendLine('  </ItemGroup>')
  }
  [void]$sb.AppendLine('  <ItemGroup>')
  foreach ($s in $Sources) {
    [void]$sb.AppendLine("    <ClCompile Include=`"$s`" />")
  }
  [void]$sb.AppendLine('  </ItemGroup>')
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />')
  [void]$sb.AppendLine('</Project>')
  [System.IO.File]::WriteAllText($projPath, $sb.ToString(), [System.Text.UTF8Encoding]::new($false))
  Write-Host "Wrote $projPath"
}

# --- Collect existing sources ---
$modules = @(
  @{ Name="base"; Guid="B762AF39-B217-41F2-9D7B-E55EDB99F9CC"; Dir="base" },
  @{ Name="rfc_algorithm"; Guid="05A295B9-8A6F-4645-A87A-95E460EE46C8"; Dir="rfc_algorithm" },
  @{ Name="message_framework"; Guid="FD7F12A0-B830-4225-8249-5A41D28277B5"; Dir="message_framework" },
  @{ Name="gfx"; Guid="57285F70-013F-41B8-AA7F-4FBFA1005372"; Dir="gfx" },
  @{ Name="animation"; Guid="80817CD3-82CB-4118-9D70-B70F1648B90F"; Dir="animation" },
  @{ Name="view_framework"; Guid="3F19C1C0-7F28-4DF0-84CC-C69FFB55F359"; Dir="view_framework" }
)

foreach ($m in $modules) {
  $dir = Join-Path $Root $m.Dir
  $old = Join-Path $dir "$($m.Name).vcxproj"
  $sources = Get-ClCompileIncludes $old
  $headers = Get-ClIncludeIncludes $old
  Write-StaticLibProject -Name $m.Name -Guid $m.Guid -Dir $dir -Sources $sources -Headers $headers
}

# --- AuraLite.dll ---
$dllGuid = "A11A11A1-4111-4111-E000-A11A11A11E01"
$dllDir = Join-Path $Root "AuraLite"
New-Item -ItemType Directory -Path $dllDir -Force | Out-Null
$dllMain = Join-Path $dllDir "dll_main.cpp"
@"
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
  (void)module;
  (void)reserved;
  switch (reason) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
"@ | Set-Content -Path $dllMain -Encoding UTF8

$dllProj = Join-Path $dllDir "AuraLite.vcxproj"
$libNames = @("base","rfc_algorithm","message_framework","gfx","animation","view_framework")
$libGuids = @{
  base="B762AF39-B217-41F2-9D7B-E55EDB99F9CC"
  rfc_algorithm="05A295B9-8A6F-4645-A87A-95E460EE46C8"
  message_framework="FD7F12A0-B830-4225-8249-5A41D28277B5"
  gfx="57285F70-013F-41B8-AA7F-4FBFA1005372"
  animation="80817CD3-82CB-4118-9D70-B70F1648B90F"
  view_framework="3F19C1C0-7F28-4DF0-84CC-C69FFB55F359"
}

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
[void]$sb.AppendLine('<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')
[void]$sb.AppendLine('  <ItemGroup Label="ProjectConfigurations">')
foreach ($cfg in @("Debug","Release")) {
  foreach ($plat in @("Win32","x64")) {
    [void]$sb.AppendLine("    <ProjectConfiguration Include=`"$cfg|$plat`">")
    [void]$sb.AppendLine("      <Configuration>$cfg</Configuration>")
    [void]$sb.AppendLine("      <Platform>$plat</Platform>")
    [void]$sb.AppendLine("    </ProjectConfiguration>")
  }
}
[void]$sb.AppendLine('  </ItemGroup>')
[void]$sb.AppendLine('  <PropertyGroup Label="Globals">')
[void]$sb.AppendLine('    <VCProjectVersion>17.0</VCProjectVersion>')
[void]$sb.AppendLine("    <ProjectGuid>{$dllGuid}</ProjectGuid>")
[void]$sb.AppendLine('    <Keyword>Win32Proj</Keyword>')
[void]$sb.AppendLine('    <RootNamespace>AuraLite</RootNamespace>')
[void]$sb.AppendLine('    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>')
[void]$sb.AppendLine('  </PropertyGroup>')
[void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />')
foreach ($cfg in @("Debug","Release")) {
  foreach ($plat in @("Win32","x64")) {
    $useDbg = if ($cfg -eq "Debug") { "true" } else { "false" }
    [void]$sb.AppendLine("  <PropertyGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`" Label=`"Configuration`">")
    [void]$sb.AppendLine("    <ConfigurationType>DynamicLibrary</ConfigurationType>")
    [void]$sb.AppendLine("    <UseDebugLibraries>$useDbg</UseDebugLibraries>")
    [void]$sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>")
    [void]$sb.AppendLine("    <CharacterSet>Unicode</CharacterSet>")
    if ($cfg -eq "Release") {
      [void]$sb.AppendLine("    <WholeProgramOptimization>true</WholeProgramOptimization>")
    }
    [void]$sb.AppendLine("  </PropertyGroup>")
  }
}
[void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />')
[void]$sb.AppendLine('  <Import Project="$(ProjectDir)..\AuraLite.Common.props" />')
[void]$sb.AppendLine('  <PropertyGroup>')
[void]$sb.AppendLine('    <OutDir>$(AuraLiteRoot)bin\$(Platform)\$(Configuration)\</OutDir>')
[void]$sb.AppendLine('    <IntDir>$(AuraLiteRoot)obj\$(Platform)\$(Configuration)\AuraLite\</IntDir>')
[void]$sb.AppendLine('    <TargetName>AuraLite</TargetName>')
[void]$sb.AppendLine('  </PropertyGroup>')

$whole = ($libNames | ForEach-Object { "/WHOLEARCHIVE:`$(AuraLiteRoot)lib\`$(Platform)\`$(Configuration)\$_.lib" }) -join " "
$deps = "msimg32.lib;comctl32.lib;ole32.lib;oleaut32.lib;uuid.lib;shell32.lib;shlwapi.lib;imm32.lib;dwmapi.lib;uxtheme.lib;%(AdditionalDependencies)"

foreach ($cfg in @("Debug","Release")) {
  foreach ($plat in @("Win32","x64")) {
    $defs = "WIN32;_WINDOWS;AURALITE_IMPLEMENTATION;AURALITE_DLL"
    if ($cfg -eq "Debug") { $defs = "_DEBUG;$defs" } else { $defs = "NDEBUG;$defs" }
    [void]$sb.AppendLine("  <ItemDefinitionGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`">")
    [void]$sb.AppendLine("    <ClCompile>")
    [void]$sb.AppendLine("      <PreprocessorDefinitions>$defs;%(PreprocessorDefinitions)</PreprocessorDefinitions>")
    [void]$sb.AppendLine("    </ClCompile>")
    [void]$sb.AppendLine("    <Link>")
    [void]$sb.AppendLine("      <SubSystem>Windows</SubSystem>")
    [void]$sb.AppendLine("      <AdditionalDependencies>$deps</AdditionalDependencies>")
    [void]$sb.AppendLine("      <AdditionalOptions>$whole %(AdditionalOptions)</AdditionalOptions>")
    [void]$sb.AppendLine("      <ModuleDefinitionFile>")
    [void]$sb.AppendLine("      </ModuleDefinitionFile>")
    [void]$sb.AppendLine("    </Link>")
    [void]$sb.AppendLine("  </ItemDefinitionGroup>")
  }
}

[void]$sb.AppendLine('  <ItemGroup>')
[void]$sb.AppendLine('    <ClCompile Include="dll_main.cpp" />')
[void]$sb.AppendLine('  </ItemGroup>')
[void]$sb.AppendLine('  <ItemGroup>')
foreach ($n in $libNames) {
  $rel = "..\$n\$n.vcxproj"
  [void]$sb.AppendLine("    <ProjectReference Include=`"$rel`">")
  [void]$sb.AppendLine("      <Project>{$($libGuids[$n])}</Project>")
  [void]$sb.AppendLine("    </ProjectReference>")
}
[void]$sb.AppendLine('  </ItemGroup>')
[void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />')
[void]$sb.AppendLine('</Project>')
[System.IO.File]::WriteAllText($dllProj, $sb.ToString(), [System.Text.UTF8Encoding]::new($false))
Write-Host "Wrote $dllProj"

# --- test_view / test_base apps linking AuraLite.dll ---
function Write-AppProject {
  param(
    [string]$Name,
    [string]$Guid,
    [string]$Dir,
    [string]$SubSystem, # Windows or Console
    [bool]$HasRc = $false
  )
  $old = Join-Path $Dir "$Name.vcxproj"
  $sources = Get-ClCompileIncludes $old
  $headers = @()
  try { $headers = Get-ClIncludeIncludes $old } catch {}

  $projPath = Join-Path $Dir "$Name.vcxproj"
  $sb = New-Object System.Text.StringBuilder
  [void]$sb.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
  [void]$sb.AppendLine('<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')
  [void]$sb.AppendLine('  <ItemGroup Label="ProjectConfigurations">')
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      [void]$sb.AppendLine("    <ProjectConfiguration Include=`"$cfg|$plat`">")
      [void]$sb.AppendLine("      <Configuration>$cfg</Configuration>")
      [void]$sb.AppendLine("      <Platform>$plat</Platform>")
      [void]$sb.AppendLine("    </ProjectConfiguration>")
    }
  }
  [void]$sb.AppendLine('  </ItemGroup>')
  [void]$sb.AppendLine('  <PropertyGroup Label="Globals">')
  [void]$sb.AppendLine('    <VCProjectVersion>17.0</VCProjectVersion>')
  [void]$sb.AppendLine("    <ProjectGuid>{$Guid}</ProjectGuid>")
  [void]$sb.AppendLine('    <Keyword>Win32Proj</Keyword>')
  [void]$sb.AppendLine("    <RootNamespace>$Name</RootNamespace>")
  [void]$sb.AppendLine('    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>')
  [void]$sb.AppendLine('  </PropertyGroup>')
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />')
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      $useDbg = if ($cfg -eq "Debug") { "true" } else { "false" }
      [void]$sb.AppendLine("  <PropertyGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`" Label=`"Configuration`">")
      [void]$sb.AppendLine("    <ConfigurationType>Application</ConfigurationType>")
      [void]$sb.AppendLine("    <UseDebugLibraries>$useDbg</UseDebugLibraries>")
      [void]$sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>")
      [void]$sb.AppendLine("    <CharacterSet>Unicode</CharacterSet>")
      [void]$sb.AppendLine("  </PropertyGroup>")
    }
  }
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />')
  [void]$sb.AppendLine('  <Import Project="$(ProjectDir)..\AuraLite.Common.props" />')
  [void]$sb.AppendLine('  <PropertyGroup>')
  [void]$sb.AppendLine('    <OutDir>$(AuraLiteRoot)bin\$(Platform)\$(Configuration)\</OutDir>')
  [void]$sb.AppendLine('    <IntDir>$(AuraLiteRoot)obj\$(Platform)\$(Configuration)\$(ProjectName)\</IntDir>')
  [void]$sb.AppendLine('  </PropertyGroup>')
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      $defs = "WIN32;_WINDOWS;AURALITE_DLL"
      if ($SubSystem -eq "Console") { $defs = "WIN32;_CONSOLE;AURALITE_DLL" }
      if ($cfg -eq "Debug") { $defs = "_DEBUG;$defs" } else { $defs = "NDEBUG;$defs" }
      [void]$sb.AppendLine("  <ItemDefinitionGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`">")
      [void]$sb.AppendLine("    <ClCompile>")
      [void]$sb.AppendLine("      <PreprocessorDefinitions>$defs;%(PreprocessorDefinitions)</PreprocessorDefinitions>")
      [void]$sb.AppendLine("    </ClCompile>")
      [void]$sb.AppendLine("    <Link>")
      [void]$sb.AppendLine("      <SubSystem>$SubSystem</SubSystem>")
      [void]$sb.AppendLine("      <AdditionalDependencies>AuraLite.lib;%(AdditionalDependencies)</AdditionalDependencies>")
      [void]$sb.AppendLine("      <AdditionalLibraryDirectories>`$(AuraLiteRoot)bin\`$(Platform)\`$(Configuration);%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>")
      [void]$sb.AppendLine("    </Link>")
      [void]$sb.AppendLine("  </ItemDefinitionGroup>")
    }
  }
  if ($headers.Count -gt 0) {
    [void]$sb.AppendLine('  <ItemGroup>')
    foreach ($h in $headers) { [void]$sb.AppendLine("    <ClInclude Include=`"$h`" />") }
    [void]$sb.AppendLine('  </ItemGroup>')
  }
  [void]$sb.AppendLine('  <ItemGroup>')
  foreach ($s in $sources) { [void]$sb.AppendLine("    <ClCompile Include=`"$s`" />") }
  [void]$sb.AppendLine('  </ItemGroup>')
  if ($HasRc) {
    [void]$sb.AppendLine('  <ItemGroup>')
    [void]$sb.AppendLine("    <ResourceCompile Include=`"$Name.rc`" />")
    [void]$sb.AppendLine('  </ItemGroup>')
  }
  [void]$sb.AppendLine('  <ItemGroup>')
  [void]$sb.AppendLine('    <ProjectReference Include="..\AuraLite\AuraLite.vcxproj">')
  [void]$sb.AppendLine("      <Project>{$dllGuid}</Project>")
  [void]$sb.AppendLine('    </ProjectReference>')
  [void]$sb.AppendLine('  </ItemGroup>')
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />')
  [void]$sb.AppendLine('</Project>')
  [System.IO.File]::WriteAllText($projPath, $sb.ToString(), [System.Text.UTF8Encoding]::new($false))
  Write-Host "Wrote $projPath"
}

Write-AppProject -Name "test_base" -Guid "16CD023E-334A-4D66-A243-76A5B30F18B9" -Dir (Join-Path $Root "test_base") -SubSystem "Console"
Write-AppProject -Name "test_view" -Guid "F19EF4AA-5DD6-4E44-9B8E-2455568AE16E" -Dir (Join-Path $Root "test_view") -SubSystem "Windows" -HasRc $true

# --- library.sln ---
$sln = @"
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.0.31903.59
MinimumVisualStudioVersion = 10.0.40219.1
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "base", "base\base.vcxproj", "{B762AF39-B217-41F2-9D7B-E55EDB99F9CC}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "rfc_algorithm", "rfc_algorithm\rfc_algorithm.vcxproj", "{05A295B9-8A6F-4645-A87A-95E460EE46C8}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "message_framework", "message_framework\message_framework.vcxproj", "{FD7F12A0-B830-4225-8249-5A41D28277B5}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "gfx", "gfx\gfx.vcxproj", "{57285F70-013F-41B8-AA7F-4FBFA1005372}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "animation", "animation\animation.vcxproj", "{80817CD3-82CB-4118-9D70-B70F1648B90F}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "view_framework", "view_framework\view_framework.vcxproj", "{3F19C1C0-7F28-4DF0-84CC-C69FFB55F359}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "AuraLite", "AuraLite\AuraLite.vcxproj", "{$dllGuid}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "test_base", "test_base\test_base.vcxproj", "{16CD023E-334A-4D66-A243-76A5B30F18B9}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "test_view", "test_view\test_view.vcxproj", "{F19EF4AA-5DD6-4E44-9B8E-2455568AE16E}"
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|Win32 = Debug|Win32
		Debug|x64 = Debug|x64
		Release|Win32 = Release|Win32
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution

"@

$allGuids = @(
  "B762AF39-B217-41F2-9D7B-E55EDB99F9CC",
  "05A295B9-8A6F-4645-A87A-95E460EE46C8",
  "FD7F12A0-B830-4225-8249-5A41D28277B5",
  "57285F70-013F-41B8-AA7F-4FBFA1005372",
  "80817CD3-82CB-4118-9D70-B70F1648B90F",
  "3F19C1C0-7F28-4DF0-84CC-C69FFB55F359",
  $dllGuid,
  "16CD023E-334A-4D66-A243-76A5B30F18B9",
  "F19EF4AA-5DD6-4E44-9B8E-2455568AE16E"
)
foreach ($g in $allGuids) {
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      $sln += "`t`t{$g}.$cfg|$plat.ActiveCfg = $cfg|$plat`r`n"
      $sln += "`t`t{$g}.$cfg|$plat.Build.0 = $cfg|$plat`r`n"
    }
  }
}
$sln += @"
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
EndGlobal
"@
[System.IO.File]::WriteAllText((Join-Path $Root "library.sln"), $sln, [System.Text.UTF8Encoding]::new($false))
Write-Host "Wrote library.sln"
Write-Host "Done."
