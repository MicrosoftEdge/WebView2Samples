# Run this command from TShell when connected to Holo VM.
# It will install the webview2_sample_uwp and the runtime if pointed by $WebView2RuntimePath

param(
  [Parameter(HelpMessage="When dealing only with runtime changes")]
  [switch] $SkipApp,

  [Parameter(HelpMessage="When dealing only with App changes")]
  [switch] $SkipRuntime,

  [string]$WebView2RuntimePath = $null
)

# Connection to the Holo VM
if ((Get-Variable Device*).Count -eq 0) {
    Write-Error "You need to be conntected to a device. Use Open-Device command to connect to your Holo VM. See https://www.osgwiki.com/wiki/TShell_Commands#open-device:_connecting_tshell_to_target_device"
    Exit 1
}
Write-Host "Connected to a device with IP=$((Get-Variable DeviceAddress).Value)"

$dataTestBinPath = "U:\data\test\bin"
$cmdPrefix = "cmdd $dataTestBinPath\mindeployappx.exe"
# The mindeployappx.exe should be retrieved from
# \\winbuilds\release\ni_release_svc_sydney_dev\22621.1070.230305-1600\amd64fre\bin\appxtools\deployappx
# or a newer folder

$scriptPath = Split-path -parent $myInvocation.MyCommand.Definition

Write-Host "The script is in $scriptPath"

if (!$SkipApp) {
    $uwpSampleAppPackageName = "WebView2-WinUI2-UWP-Sample_1.0.0.0_x64__ph1m9x8skttmg"

    # Check if the solution file exists
    $sln = $scriptPath + "\webview2_sample_uwp.sln"
    if (-Not (Test-Path $sln -PathType Leaf)) {
        Write-Error "The solution file is expected to exist: $sln"
        Exit 1
    }

    # Build and create app package
    pushd (Get-ChildItem -Path "C:\Program Files*\Microsoft Visual Studio\*\*\MSBuild\Current\Bin")[0].FullName
    ./msbuild.exe $sln /property:Configuration=Debug /property:Platform=x64 /p:AppxBundle=Always /p:UapAppxPackageBuildMode=StoreUpload -verbosity:minimal
    popd

    Write-Host "Copying and installing app..." -ForegroundColor Cyan
    $command = $cmdPrefix
    $command += ' /Add /PackagePath:"' + $dataTestBinPath + '\webview2_sample_uwp_1.0.0.0_x64_Debug.msixbundle"'
    $command += ' /DependencyPackagePaths:"'

    # For each app dependency copy the appx file and update the command for deployment
    Get-ChildItem -Path $scriptPath\AppPackages\webview2_sample_uwp_1.0.0.0_Debug_Test\Dependencies\x64\ | ForEach-Object {
        Write-Host "dependencies += $_"
        putd $scriptPath\AppPackages\webview2_sample_uwp_1.0.0.0_Debug_Test\Dependencies\x64\$_ $dataTestBinPath
        $command += $dataTestBinPath + "\$_;"
    }

    putd $scriptPath\AppPackages\webview2_sample_uwp_1.0.0.0_Debug_Test\webview2_sample_uwp_1.0.0.0_x64_Debug.msixbundle $dataTestBinPath

    $command += '"'

    Write-Host $command -ForegroundColor Cyan
    Invoke-Expression $command
}

if ($WebView2RuntimePath) {
    Write-Host "Copying and installing runtime..." -ForegroundColor Cyan
    putd $WebView2RuntimePath $dataTestBinPath
    $fileName = Split-Path $WebView2RuntimePath -Leaf
    $command = $cmdPrefix + ' /Add /PackagePath:"' + $dataTestBinPath + '\' + $fileName  + '"'
    Write-Host $command -ForegroundColor Cyan
    Invoke-Expression $command
}
