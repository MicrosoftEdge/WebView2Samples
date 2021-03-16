---
description: "Demonstrate the usage patterns of WebView2 CDP Extension in WPF."
extendedZipContent:
  -
    path: SharedContent
    target: SharedContent
  -
    path: LICENSE
    target: LICENSE
languages:
  - csharp
page_type: sample
products:
  - microsoft-edge
urlFragment: WV2CDPExtensionSample
---
# WebView2 CDP Extension

This application built with the [WebView2 CDP Extension](https://aka.ms/webviewcdp) that defines all CDP methods, events, and types.

The WV2CDPExtensionSample showcases of Utilizing Chrome DevTools Protocol functions using a DevToolsProtocolHelper object in WebView2. It is built as a WPF [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) project and makes use of C# in the WebView2 environment.

If this is your first time using WebView2, we recommend first following the [Getting Started](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/wpf) guide, which goes over how to create a WebView2 and walks through some basic WebView2 functionality.

## Prerequisites

- [Microsoft Edge (Chromium)](https://www.microsoftedgeinsider.com/download/) installed on a supported OS. Currently we recommend the latest version of the Edge Canary channel.
- [Visual Studio](https://visualstudio.microsoft.com/vs/) with .NET support installed.
- Latest version of our [WebView2 SDK](https://aka.ms/webviewnuget), which is included in this project.
- Latest release version of our [WebView2 CDP Extension](https://aka.ms/webviewcdpnuget), which is included in this project.

## Build the WebView2 WPF Browser

Clone the repository and open the solution in Visual Studio. WebView2 and WebView2 DevToolsProtocolExtension is already included as a NuGet package* in this project.

- Open the solution in Visual Studio 2019**
- Set the target you want to build (Debug/Release, AnyCPU)
- Build the project file: _WV2CDPExtensionSample.csproj_

That's it! Everything should be ready to just launch the app.

*You can get the WebView2 and WebView2 DevToolsProtocolExtension NugetPackage through the Visual Studio NuGet Package Manager.

**You can also use Visual Studio 2017 by changing the project's Platform Toolset in Project Properties/Configuration properties/General/Platform Toolset. You might also need to change the Windows SDK to the latest version available to you.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact opencode@microsoft.com with any additional questions or comments.
