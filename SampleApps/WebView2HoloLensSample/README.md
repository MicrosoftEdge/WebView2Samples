---
description: "Demonstrate the features and usage patterns of WebView2 on HoloLens 2 in Unity."
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
urlFragment: WebView2HoloLensSample
---
# WebView2 HoloLens 2 Sample

![Sample App Snapshot](screenshots/webview2_browser_hololens2.png)

The WebView2HololensSample is an example of a Unity application built for running on HoloLens 2 with an embedded WebView2 control. The sample showcases a simple web browser which can navigate to URLs via a text input control, as well as nevigate to previous pages.

> Note: Input within the web page itself does not function with this demo. Though, with some additional effort, a developer could modify the sample to support it. An overview of input handling for WebView2 inside Unity can be found in our [Getting Started](https://learn.microsoft.com/microsoft-edge/webview2/gettingstarted/hololens2) documentation.

If this is your first time using WebView2 on HoloLens, we recommend first following the [Getting Started](https://learn.microsoft.com/microsoft-edge/webview2/gettingstarted/hololens2) guide.

## Prerequisites

- [Visual Studio](https://visualstudio.microsoft.com/vs/) 2019 or greater, with .NET support installed.
- Latest version of our [Mixed Reality Feature Tool](https://learn.microsoft.com/en-us/windows/mixed-reality/develop/unity/welcome-to-mr-feature-tool)
- [Unity](https://unity.com/download) version 2020.3.35f*

## Build the WebView2 Windows Forms Browser

Follow the following basic steps to get started with this sample code. WebView2 is already included as a Unity package, but you will need to load it via the Mixed Reality Feature Tool.

- Clone this repository
- Launch the Microsoft Mixed Realty Feature Tool
  - Click **Start**
  - Click the **...** button, navigate to the WebView2HoloLensSample folder, and select **Open**
  - With the project path selected, click **Restore Features** to load the requisite packages for the Mixed Reality Toolkit and the WebView2 plugin package.
- Close the Mixed Reality Feature Tool
- Launch Unity Hub
- In Unity Hub, click **Open**, navigate to the WebView2HoloLensSample folder, and select **Open**. This should open the project in the Unity Editor.
- In the Unity Editor, find the SampleScene the under Assets/Scenes folder and double-click to load it.
- Click the play button in Unity Editor to quick test the app running.
- Build & Deploy app to a HoloLens device
  - In Unity Editor, select **File > Build Settings...**
  - Ensure Universal App Platform  is selected as the platform. If not, select it and click **Switch Platform**.
  - Select HoloLens as the **Target Devive**, then click **Build**.
  - Open the generated Visual Studio solution in Visual Studio.
  - Select the **Release** or **Master** for the Solution Configuration. (Debug is available, but not recommended for performance reasons.)
  - Select **ARM64** as the Solution Platform.
  - Build the solution.
  - To deploy, you can either choose **Remote Machine** (over WiFi) or **Device** (over USB). [Read here](https://learn.microsoft.com/en-us/windows/mixed-reality/develop/advanced-concepts/using-visual-studio?tabs=hl2) for more detailed information on deploying to a HoloLens device. 

*If you have a different 2020.3 version of the Unity Editor, other than 2020.3.35f, Unity Hub will offer you the ability to switch the project to a different editor version. This can cause unexpected issues. It's recommend you download the matching version of the editor.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact opencode@microsoft.com with any additional questions or comments.
