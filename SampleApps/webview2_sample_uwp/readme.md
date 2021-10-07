---
description: "Demonstrate the features and usage patterns of WebView2 running in a UWP application using winui2."
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
urlFragment: WebView2_UWP
---
# WebView2 UWP WinUi2 browser

This is a hybrid application built with the [Microsoft Edge WebView2](https://aka.ms/webview2) control.

![Sample App Snapshot](https://raw.githubusercontent.com/MicrosoftEdge/WebView2Samples/master/SampleApps/WebView2WpfBrowser/screenshots/wpf-browser-screenshot.png)

The WebView2 UWP is an example of an application that embeds a WebView within a UWP application. It is built as a UWP [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) project and makes use of both cpp and HTML/CSS/JavaScript in the WebView2 environment.

The sample showcases simple application structure and handling of webview api.

If this is your first time using WebView, we recommend first following the [Getting Started](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/wpf) guide, which goes over how to create a WebView2 and walks through some basic WebView2 functionality.

To learn more specifics about events and API Handlers in WebView2, you can refer to the [WebView2 Reference Documentation](https://docs.microsoft.com/microsoft-edge/webview2/webview2-api-reference).

## Prerequisites

- [Microsoft Edge (Chromium)](https://www.microsoftedgeinsider.com/download/) installed on a supported OS. Currently we recommend the latest version of the Edge Canary channel.
- [Visual Studio](https://visualstudio.microsoft.com/vs/) with .NET support installed.
- Latest pre-release version of our [WebView2 SDK](https://aka.ms/webviewnuget), which is included in this project.
- Latest pre-release version of the [WinUI2 SDK](https://aka.ms/webviewnuget), which is included in this project

## Build the WebView2 UWP WinUi2 browser

Clone the repository and open the solution in Visual Studio. WebView2 & WebUi2 are already included as a NuGet package* in this project.

- Clone this repository
- Open the solution in Visual Studio 2019
- Set the target you want to build (Debug/Release, AnyCPU)
- Build the project file: _WebView2_UWP.csproj_

That's it! Everything should be ready to just launch the app.

*You can get the WebView2 & WinUI2 NugetPackage through the Visual Studio NuGet Package Manager.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact opencode@microsoft.com with any additional questions or comments.
