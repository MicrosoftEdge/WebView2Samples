param(
  [switch] $Prerelease = $false
)

$source = "https://www.nuget.org/api/v2"

$matrix = Find-Package Microsoft.Web.WebView2 -AllVersions -Source $source

Write-Host "The following are Microsoft.Web.WebView2 versions being tested:"
$version = ($matrix | Where-Object { $_.Version.EndsWith("-prerelease") -eq $Prerelease} | Select-Object -First 1).Version
$version = $version.Replace("-prerelease", ".0")
Write-Host $version

$tag = "official-$version"
Write-Host "Tag: $tag"
#($matrix | Select-Object Version | Where $_.Version.EndsWith("-prerelease") |Select-Object -First 1).Version

return $tag