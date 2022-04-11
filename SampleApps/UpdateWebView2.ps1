param (
  [Parameter(Mandatory=$true)]
  [string] $NugetPath
)

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