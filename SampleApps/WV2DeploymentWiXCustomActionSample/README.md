---
description: "Demonstrate the deployment workflow of WebView2 with WiX Custom Action."
extendedZipContent:
  -
    path: SharedContent
    target: SharedContent
  -
    path: LICENSE
    target: LICENSE
languages:
  - cpp
page_type: sample
products:
  - microsoft-edge
urlFragment: WV2DeploymentWiXCustomActionSample
---
# WebView2 Deployment WiX Custom Action Sample

To help developers understand how to [deploy the Evergreen WebView2 Runtime](https://docs.microsoft.com/microsoft-edge/webview2/concepts/distribution#deploying-the-evergreen-webview2-runtime) with your application, this sample creates a [WiX](https://wixtoolset.org/) installer for [WebView2APISample](../WebView2APISample/README.md) and uses [WiX Custom Action](https://wixtoolset.org/documentation/manual/v3/wixdev/extensions/authoring_custom_actions.html) to chain-install the Evergreen WebView2 Runtime.

This sample showcases [deployment workflows](https://docs.microsoft.com/microsoft-edge/webview2/concepts/distribution#deploying-the-evergreen-webview2-runtime) for,

* Download the Evergreen WebView2 Runtime Bootstrapper through link.
* Package the Evergreen WebView2 Runtime Bootstrapper.
* Package the Evergreen WebView2 Runtime Standalone Installer.

## Prerequisites

* [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) with C++ support installed.
* [WiX Toolset](https://wixtoolset.org/).
* [WiX Toolset Visual Studio 2019 Extension](https://marketplace.visualstudio.com/items?itemName=WixToolset.WixToolsetVisualStudio2019Extension).

## Build steps

To create a WiX installer that chain-installs the Evergreen WebView2 Runtime through Custom Action,

1. Open `../WebView2Samples.sln` with Visual Studio, then open `Product.wxs` under the `WV2DeploymentWiXCustomActionSample` project. Edit `Product.wxs` depending on the workflow you wish to use.
    * For "Download the Evergreen WebView2 Runtime Bootstrapper through link",
        * Under `<!-- Step 4: Config Custom Action to download/install Bootstrapper -->`, uncomment the `<CustomAction Id='DownloadAndInvokeBootstrapper' ...>` element below `<!-- [Download Bootstrapper] ... -->`. Comment out other `<Binary>` and `<CustomAction>` elements.
        * Under `<!-- Step 5: Config execute sequence of custom action -->`, uncomment the `<Custom Action='DownloadAndInvokeBootstrapper' ...>` element below `<!-- [Download Bootstrapper] ...-->`. Comment out other `<Custom>` elements.
    * For "Package the Evergreen WebView2 Runtime Bootstrapper",
        * Under `<!-- Step 4: Config Custom Action to download/install Bootstrapper -->`, uncomment the `<Binary Id="MicrosoftEdgeWebview2Setup.exe" ...>` and `<CustomAction Id='InvokeBootstrapper' ...>` elements below `<!-- [Package Bootstrapper] ... -->`. Comment out other `<Binary>` and `<CustomAction>` elements.
        * Under `<!-- Step 5: Config execute sequence of custom action -->`, uncomment the `<Custom Action='InvokeBootstrapper' ...>` element below `<!-- [Package Bootstrapper] ...-->`. Comment out other `<Custom>` elements.
    * For "Package the Evergreen WebView2 Runtime Standalone Installer",
        * Under `<!-- Step 4: Config Custom Action to download/install Bootstrapper -->`, uncomment the `<Binary Id="MicrosoftEdgeWebView2RuntimeInstallerX64.exe" ...>` and `<CustomAction Id='InvokeStandalone' ...>` elements below `<!-- [Package Standalone Installer] ... -->`. Comment out other `<Binary>` and `<CustomAction>` elements. If you're targeting non-X64 devices, you may also want to edit the `MicrosoftEdgeWebView2RuntimeInstallerX64` filename to reflect the correct architecture.
        * Under `<!-- Step 5: Config execute sequence of custom action -->`, uncomment the `<Custom Action='InvokeStandalone' ...>` element below `<!-- [Package Standalone Installer] ...-->`. Comment out other `<Custom>` elements.
1. If you plan to package either the Bootstrapper or the Standalone Installer, [download](https://developer.microsoft.com/microsoft-edge/webview2/) the Bootstrapper or the Standalone Installer and place it under the enclosing `SampleApps` folder.
1. Build the `WV2DeploymentVSInstallerSample` project.
