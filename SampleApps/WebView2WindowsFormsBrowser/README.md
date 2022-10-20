---
description: "Demonstrate the features and usage patterns of WebView2 in Windows Forms."
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
urlFragment: WebView2WindowsFormsBrowser
---
# WebView2 Windows Forms Browser

This is a hybrid application built with the [Microsoft Edge WebView2](https://aka.ms/webview2) control.

![Sample App Snapshot](https://raw.githubusercontent.com/MicrosoftEdge/WebView2Samples/master/SampleApps/WebView2WindowsFormsBrowser/screenshots/winforms-browser-screenshot.png)

The WebView2WindowsFormsBrowser is an example of an application that embeds a WebView within a Windows Forms application. It is built as a Windows Forms [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) project and makes use of both C# and HTML/CSS/JavaScript in the WebView2 environment.

The API Sample showcases a selection of WebView2's event handlers and API methods that allow a Windows Forms application to directly interact with a WebView and vice versa.

If this is your first time using WebView, we recommend first following the [Getting Started](https://learn.microsoft.com/microsoft-edge/webview2/gettingstarted/winforms) guide, which goes over how to create a WebView2 and walks through some basic WebView2 functionality.

To learn more specifics about events and API Handlers in WebView2, you can refer to the [WebView2 Reference Documentation](https://learn.microsoft.com/microsoft-edge/webview2/webview2-api-reference).

## Prerequisites

- [Microsoft Edge (Chromium)](https://www.microsoftedgeinsider.com/download/) installed on a supported OS. Currently we recommend the latest version of the Edge Canary channel.
- [Visual Studio](https://visualstudio.microsoft.com/vs/) with .NET support installed.
- Latest pre-release version of our [WebView2 SDK](https://aka.ms/webviewnuget), which is included in this project.

## Build the WebView2 Windows Forms Browser

Clone the repository and open the solution in Visual Studio. WebView2 is already included as a NuGet package* in this project.

- Clone this repository
- Open the solution in Visual Studio 2019**
- Set the target you want to build (Debug/Release, AnyCPU)
- Build the project file: _WebView2WindowsFormsBrowser.csproj_

That's it! Everything should be ready to just launch the app.

*You can get the WebView2 NugetPackage through the Visual Studio NuGet Package Manager.

**You can also use Visual Studio 2017 by changing the project's Platform Toolset in Project Properties/Configuration properties/General/Platform Toolset. You might also need to change the Windows SDK to the latest version available to you.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact opencode@microsoft.com with any additional questions or comments.
