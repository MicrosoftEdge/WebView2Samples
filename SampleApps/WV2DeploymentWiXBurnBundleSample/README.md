---
description: "Demonstrate the deployment workflow of WebView2 with WiX Burn Bundle."
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
urlFragment: WV2DeploymentWiXBurnBundleSample
---
# WebView2 Deployment WiX Burn Bundle Sample

To help developers understand how to [deploy the Evergreen WebView2 Runtime](https://docs.microsoft.com/microsoft-edge/webview2/concepts/distribution#deploying-the-evergreen-webview2-runtime) with your application, this sample creates a [WiX](https://wixtoolset.org/) installer for [WebView2APISample](../WebView2APISample/README.md) and uses [WiX Burn Bundle](https://wixtoolset.org/documentation/manual/v3/bundle/) to chain-install the Evergreen WebView2 Runtime.

This sample showcases [deployment workflows](https://docs.microsoft.com/microsoft-edge/webview2/concepts/distribution#deploying-the-evergreen-webview2-runtime) for,

* Download the Evergreen WebView2 Runtime Bootstrapper through link.
* Package the Evergreen WebView2 Runtime Bootstrapper.

Packaging the Evergreen WebView2 Runtime Standalone Installer is very similar to packaging the Evergreen WebView2 Runtime Bootstrapper.

## Prerequisites

* [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) with C++ support installed.
* [WiX Toolset](https://wixtoolset.org/).
* [WiX Toolset Visual Studio 2019 Extension](https://marketplace.visualstudio.com/items?itemName=WixToolset.WixToolsetVisualStudio2019Extension).

## Build steps

To create a WiX installer that chain-installs the Evergreen WebView2 Runtime through Burn Bundle,

1. Clone the repo.
1. Open `../WebView2Samples.sln` with Visual Studio. 
1. This sample is an extension to the [WV2DeploymentWiXCustomActionSample](../WV2DeploymentWiXCustomActionSample/README.md) sample. Let's open `Product.wxs` under the `WV2DeploymentWiXCustomActionSample` project, and comment out all the `<Binary>`, `<CustomAction>`, and `<Custom>` elements under `<!-- Step 4: Config Custom Action to download/install Bootstrapper -->` and `<!-- Step 5: Config execute sequence of custom action -->` so that Custom Action is not used.
1. Open `Bundle.wxs` under the `WV2DeploymentWiXBurnBundleSample` project. Edit `Bundle.wxs` depending on the workflow you wish to use.
    * For "Package the Evergreen WebView2 Runtime Bootstrapper", uncomment the `<ExePackage Id="InvokeBootstrapper" ...>` element below `<!-- [Package Bootstrapper] ... -->`, and comment out other `<ExePackage>` elements.
    * For "Download the Evergreen WebView2 Runtime Bootstrapper through link", uncomment the `<ExePackage Id="DownloadAndInvokeBootstrapper" ...>` element below `<!-- [Download Bootstrapper] ... -->`, and comment out other `<ExePackage>` elements.
1. If you plan to package the Evergreen WebView2 Runtime Bootstrapper, [download](https://developer.microsoft.com/microsoft-edge/webview2/) the Bootstrapper and place it under the enclosing `SampleApps` folder.
1. Build the `WV2DeploymentWiXBurnBundleSample` project.
