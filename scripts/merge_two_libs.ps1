# Merge AuraLite into two static libs: AuraLite.Base + AuraLite.UILegacy
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
    # Skip conditioned ATL-only or dll_main
    if ($inc -match 'dll_main') { continue }
    $list += $inc
  }
  return $list
}

function Resolve-Sources([string]$moduleDir, [string[]]$relIncludes) {
  $out = @()
  foreach ($inc in $relIncludes) {
    # Paths in old vcxproj are relative to module dir
    $full = Join-Path $moduleDir $inc
    $relFromRoot = Resolve-Path $full -ErrorAction SilentlyContinue
    if (-not $relFromRoot) {
      Write-Warning "Missing: $full"
      continue
    }
    $rel = $relFromRoot.Path.Substring($Root.Length).TrimStart('\','/')
    $out += $rel.Replace('/','\')
  }
  return $out
}

function Write-LibProject {
  param(
    [string]$Name,
    [string]$Guid,
    [string]$ProjDir,
    [string[]]$Sources,
    [string]$ExtraDefs = ""
  )
  New-Item -ItemType Directory -Path $ProjDir -Force | Out-Null
  $projPath = Join-Path $ProjDir "$Name.vcxproj"
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
      $defs = "WIN32;_LIB;AURALITE_STATIC"
      if ($cfg -eq "Debug") { $defs = "_DEBUG;$defs" } else { $defs = "NDEBUG;$defs" }
      if ($ExtraDefs) { $defs = "$defs;$ExtraDefs" }
      [void]$sb.AppendLine("  <ItemDefinitionGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`">")
      [void]$sb.AppendLine("    <ClCompile>")
      [void]$sb.AppendLine("      <PreprocessorDefinitions>$defs;%(PreprocessorDefinitions)</PreprocessorDefinitions>")
      [void]$sb.AppendLine("    </ClCompile>")
      [void]$sb.AppendLine("  </ItemDefinitionGroup>")
    }
  }
  [void]$sb.AppendLine('  <ItemGroup>')
  foreach ($s in ($Sources | Sort-Object -Unique)) {
    # Sources are relative to AuraLite root; project is in subdir → prefix ..\
    [void]$sb.AppendLine("    <ClCompile Include=`"..\$s`" />")
  }
  [void]$sb.AppendLine('  </ItemGroup>')
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />')
  [void]$sb.AppendLine('</Project>')
  [System.IO.File]::WriteAllText($projPath, $sb.ToString(), [System.Text.UTF8Encoding]::new($false))
  Write-Host "Wrote $projPath ($($Sources.Count) sources)"
}

# --- Base: base + rfc_algorithm + message_framework ---
$baseSources = @()
$baseSources += Resolve-Sources (Join-Path $Root "base") (Get-ClCompileIncludes (Join-Path $Root "base\base.vcxproj"))
$baseSources += Resolve-Sources (Join-Path $Root "rfc_algorithm") (Get-ClCompileIncludes (Join-Path $Root "rfc_algorithm\rfc_algorithm.vcxproj"))
$baseSources += Resolve-Sources (Join-Path $Root "message_framework") (Get-ClCompileIncludes (Join-Path $Root "message_framework\message_framework.vcxproj"))

# --- UI: gfx + animation + view_framework (msaa stub, no ATL cpp) ---
$uiSources = @()
$uiSources += Resolve-Sources (Join-Path $Root "gfx") (Get-ClCompileIncludes (Join-Path $Root "gfx\gfx.vcxproj"))
$uiSources += Resolve-Sources (Join-Path $Root "animation") (Get-ClCompileIncludes (Join-Path $Root "animation\animation.vcxproj"))
$viewInc = Get-ClCompileIncludes (Join-Path $Root "view_framework\view_framework.vcxproj")
# Normalize accessibility sources
$viewInc2 = @()
foreach ($i in $viewInc) {
  if ($i -match 'view_accessibility\.cpp$') { continue }
  if ($i -match 'view_accessibility_atl\.cpp$') { continue }
  $viewInc2 += $i
}
if (-not ($viewInc2 | Where-Object { $_ -match 'view_accessibility_msaa\.cpp' })) {
  $viewInc2 = @('accessibility\view_accessibility_msaa.cpp') + $viewInc2
}
$uiSources += Resolve-Sources (Join-Path $Root "view_framework") $viewInc2

$guidBase = "B762AF39-B217-41F2-9D7B-E55EDB99F9CC"
$guidUI   = "3F19C1C0-7F28-4DF0-84CC-C69FFB55F359"
$guidTestBase = "16CD023E-334A-4D66-A243-76A5B30F18B9"
$guidTestView = "F19EF4AA-5DD6-4E44-9B8E-2455568AE16E"

Write-LibProject -Name "AuraLite.Base" -Guid $guidBase -ProjDir (Join-Path $Root "AuraLite.Base") -Sources $baseSources
Write-LibProject -Name "AuraLite.UILegacy" -Guid $guidUI -ProjDir (Join-Path $Root "AuraLite.UILegacy") -Sources $uiSources

# --- test apps ---
function Write-AppProject {
  param(
    [string]$Name,
    [string]$Guid,
    [string]$Dir,
    [string]$SubSystem,
    [bool]$HasRc = $false,
    [string[]]$LibNames
  )
  $old = Join-Path $Dir "$Name.vcxproj"
  $sources = @("main.cpp")
  if (Test-Path $old) {
    try { $sources = Get-ClCompileIncludes $old } catch {}
  }
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
  $linkLibs = (($LibNames | ForEach-Object { "$_.lib" }) -join ";") + ";msimg32.lib;comctl32.lib;ole32.lib;oleaut32.lib;uuid.lib;shell32.lib;shlwapi.lib;imm32.lib;dwmapi.lib;uxtheme.lib;oleacc.lib;%(AdditionalDependencies)"
  foreach ($cfg in @("Debug","Release")) {
    foreach ($plat in @("Win32","x64")) {
      $defs = "WIN32;AURALITE_STATIC"
      if ($SubSystem -eq "Console") { $defs = "WIN32;_CONSOLE;AURALITE_STATIC" } else { $defs = "WIN32;_WINDOWS;AURALITE_STATIC" }
      if ($cfg -eq "Debug") { $defs = "_DEBUG;$defs" } else { $defs = "NDEBUG;$defs" }
      [void]$sb.AppendLine("  <ItemDefinitionGroup Condition=`"'`$(Configuration)|`$(Platform)'=='$cfg|$plat'`">")
      [void]$sb.AppendLine("    <ClCompile>")
      [void]$sb.AppendLine("      <PreprocessorDefinitions>$defs;%(PreprocessorDefinitions)</PreprocessorDefinitions>")
      [void]$sb.AppendLine("    </ClCompile>")
      [void]$sb.AppendLine("    <Link>")
      [void]$sb.AppendLine("      <SubSystem>$SubSystem</SubSystem>")
      [void]$sb.AppendLine("      <AdditionalDependencies>$linkLibs</AdditionalDependencies>")
      [void]$sb.AppendLine("      <AdditionalLibraryDirectories>`$(AuraLiteRoot)lib\`$(Platform)\`$(Configuration);%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>")
      [void]$sb.AppendLine("    </Link>")
      [void]$sb.AppendLine("  </ItemDefinitionGroup>")
    }
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
  [void]$sb.AppendLine('    <ProjectReference Include="..\AuraLite.Base\AuraLite.Base.vcxproj">')
  [void]$sb.AppendLine("      <Project>{$guidBase}</Project>")
  [void]$sb.AppendLine('    </ProjectReference>')
  if ($LibNames -contains "AuraLite.UILegacy") {
    [void]$sb.AppendLine('    <ProjectReference Include="..\AuraLite.UILegacy\AuraLite.UILegacy.vcxproj">')
    [void]$sb.AppendLine("      <Project>{$guidUI}</Project>")
    [void]$sb.AppendLine('    </ProjectReference>')
  }
  [void]$sb.AppendLine('  </ItemGroup>')
  [void]$sb.AppendLine('  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />')
  [void]$sb.AppendLine('</Project>')
  [System.IO.File]::WriteAllText($projPath, $sb.ToString(), [System.Text.UTF8Encoding]::new($false))
  Write-Host "Wrote $projPath"
}

Write-AppProject -Name "test_base" -Guid $guidTestBase -Dir (Join-Path $Root "test_base") -SubSystem "Console" -LibNames @("AuraLite.Base")
Write-AppProject -Name "test_view" -Guid $guidTestView -Dir (Join-Path $Root "test_view") -SubSystem "Windows" -HasRc $true -LibNames @("AuraLite.Base","AuraLite.UILegacy")

# --- library.sln ---
$sln = @"
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.0.31903.59
MinimumVisualStudioVersion = 10.0.40219.1
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "AuraLite.Base", "AuraLite.Base\AuraLite.Base.vcxproj", "{$guidBase}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "AuraLite.UILegacy", "AuraLite.UILegacy\AuraLite.UILegacy.vcxproj", "{$guidUI}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "test_base", "test_base\test_base.vcxproj", "{$guidTestBase}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "test_view", "test_view\test_view.vcxproj", "{$guidTestView}"
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
foreach ($g in @($guidBase, $guidUI, $guidTestBase, $guidTestView)) {
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
Write-Host "Done. Base=$($baseSources.Count) UI=$($uiSources.Count)"
