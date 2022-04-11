<#
.SYNOPSIS
Switch WebView2APISample.vcxproj to use nuget package instead of local anaheim build output folder
like out\<flavor>_<arch>\

.DESCRIPTION
This script requires one parameter which is the path to a local nuget pacakge (.nupkg).

This script will do the following:
- Remove all the references in WebView2APISample.vcxproj to local anaheim build e.g. out\debug_x64
- Restore the two nuget packages (other than WebView2) by using nuget.exe
- Modify packages.config and WebView2APISample.vcxproj so they point to the nuget file provided to the script
- Copy and extract the nuget file to packages\Microsoft.Web.WebView2.[VERSION]\

.LINK
https://aka.ms/webview
#>

param (
  [Parameter(Mandatory=$true)]
  [string] $NugetPath
)

if (-Not ($NugetPath.EndsWith(".nupkg"))) {
    Write-Host "$NugetPath must be a path to a nupkg file"
    exit 1
}

if (-Not (Test-Path $NugetPath)) {
    Write-Host "The path $NugetPath doesn't exist"
    exit 1
}


$scriptPath = $MyInvocation.MyCommand.Path | Split-Path -Parent

# Trim off ".nupkg" and then trim left
$fullVersion = $NugetPath.Substring(0, $NugetPath.Length - 6)
$packageName = "Microsoft.Web.WebView2"
$index = $fullVersion.LastIndexOf($packageName)
$fullVersion = $fullVersion.Substring($index + $packageName.Length + 1)
Write-Host "The nuget version to install: $fullVersion"

$filename = 'WebView2APISample.vcxproj'
$vcxproj = Get-Content -Path (Join-Path $scriptPath $filename)

# Remove link references to the enlistment e.g. "..\..\..\..\out\debug_x64\WebView2Loader.dll.lib"
Foreach ($conf in "release", "debug") {
    Foreach ($arch in "x64", "x86", "arm64") {
        Foreach ($dll in "WebView2Loader.dll.lib", "WebView2Guid.lib", "WebView2LoaderStatic.lib") {
            $replace = '..\..\..\..\out\' + $conf + '_' + $arch + '\' + $dll
            $vcxproj = $vcxproj.Replace($replace, "")
        }
    }
}

$vcxproj | Set-Content -Path (Join-Path $scriptPath $filename)

# Restore existing packages
if (-Not (Test-Path "nuget.exe")) {
    Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile nuget.exe
}
Write-Host "NOTE: Please ignore any errors from nuget restore on WV2DeploymentWiXCustomActionSample.wixproj and WV2DeploymentWiXBurnBundleSample.wixproj." -ForegroundColor Yellow

Set-Content -Path 'NuGet.Config' -Value (
'<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
</configuration>')


# As noted in the message to the user the nuget restore fails for these two wixproj's which are irrelevant
.\nuget.exe restore -SolutionDirectory c:\webview2samples\SampleApps

#exit 0

[xml]$vcxprojXML = $vcxproj

# Add project reference to the nuget package
$importGroup = $vcxprojXML.Project.ImportGroup | Where-Object {$_.Label -match "ExtensionTargets"}
if ($importGroup.LastChild.Project.EndsWith("Microsoft.Web.WebView2.targets")) {
    $importGroup.LastChild.RemoveFromTree()
}
$import = $vcxprojXML.CreateElement("Import", $vcxprojXML.Project.xmlns)
$import.SetAttribute("Project", "packages\Microsoft.Web.WebView2.$fullVersion\build\native\Microsoft.Web.WebView2.targets")
$import.SetAttribute("Condition", $condition)
$importGroup.AppendChild($import) | Out-Null

# Add error message
$condition = "Exists('packages\Microsoft.Web.WebView2.$fullVersion\build\native\Microsoft.Web.WebView2.targets')"
$target = $vcxprojXML.Project.Target | Where-Object {$_.Name -match "EnsureNuGetPackageBuildImports"}
$errorNode = $vcxprojXML.CreateElement("Error", $vcxprojXML.Project.xmlns)
$errorNode.SetAttribute("Condition", "!$condition")
$target.AppendChild($errorNode) | Out-Null
$vcxprojXML.Save((Join-Path $scriptPath $filename))

# Update packages.config
[xml]$packages = Get-Content -Path (Join-Path $scriptPath "packages.config")
$nuget = $packages.CreateElement("package")
$nuget.SetAttribute("id", "Microsoft.Web.WebView2")
$nuget.SetAttribute("version", $fullVersion)
$nuget.SetAttribute("targetFramework", "native")
if ($packages.packages.LastChild.id -eq "Microsoft.Web.WebView2") {
    Write-Host "The WebView2 nuget package was already added to packages.config. If you want to add this package revert $filename and packages.config and rerun the script." -ForegroundColor Red    
    exit 1
}
$packages.packages.AppendChild($nuget) | Out-Null
$packages.Save((Join-Path $scriptPath "packages.config"))


# Install the WebView2 package by extracting the nuget package file
Copy-Item $NugetPath -Destination (Join-Path $scriptPath "\package.zip")
New-Item -Path (Join-Path $scriptPath .\packages\Microsoft.Web.WebView2.$fullVersion) -Type Directory -Force
Expand-Archive (Join-Path $scriptPath "\package.zip") -DestinationPath (Join-Path $scriptPath "\packages\Microsoft.Web.WebView2.$fullVersion") -Force
Remove-Item (Join-Path $scriptPath "\package.zip")
