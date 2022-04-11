if (-Not (Test-Path "nuget.exe")) {
    Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile nuget.exe
}

.\nuget.exe restore -PackagesDirectory c:\WebView2Samples\SampleApps\packages -Source "C:\packages\"

$vcxproj = "$(split-path -parent $MyInvocation.MyCommand.Path)\WebView2APISample.vcxproj"

pushd (Get-ChildItem -Path "C:\Program Files *\Microsoft Visual Studio\*\*\MSBuild\Current\Bin")[0].FullName

.\msbuild.exe $vcxproj

popd
