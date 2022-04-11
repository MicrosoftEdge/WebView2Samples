param (
  [Parameter(Mandatory=$true)]
  [string] $NugetPath
)

$packageVersion = $NugetPath.TrimEnd(".nupkg")
$packageVersion = $packageVersion.Substring($packageVersion.LastIndexOf("Microsoft.Web.WebView2.") + 23)
echo $packageVersion

$scriptPath = $MyInvocation.MyCommand.Path | Split-Path -Parent

# Update packages.config
[xml]$packages = Get-Content -Path (Join-Path $scriptPath "packages.config")
$nuget = $packages.CreateElement("package")
$nuget.SetAttribute("id", "Microsoft.Web.WebView2")
$nuget.SetAttribute("version", $fullVersion)
$nuget.SetAttribute("targetFramework", "native")

foreach ($child in $packages.packages.ChildNodes) {
    Write-Host "Child:"
    if ($child.id -eq "Microsoft.Web.WebView2") {
        $child.SetAttribute("version", $packageVersion)
        break;
    }
}
$packages.Save((Join-Path $scriptPath "packages.config"))

exit 0

if ($packages.packages.LastChild.id -eq "Microsoft.Web.WebView2") {
    Write-Host "The WebView2 nuget package was already added to packages.config. If you want to add this package revert $filename and packages.config and rerun the script." -ForegroundColor Red    
    exit 1
}
