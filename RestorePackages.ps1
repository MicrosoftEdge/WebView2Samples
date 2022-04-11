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
.\nuget.exe restore

