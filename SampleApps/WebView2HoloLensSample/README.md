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

The WebView2HoloLensSample is a Unity application designed for HoloLens 2, featuring an embedded WebView2 control. This sample demonstrates a simple web browser that allows users to navigate to URLs via a text input control and navigate between previous pages.

> Note: Input within the web page itself is not functional in this demo. However, with some additional effort, developers can modify the sample to support it. You can find an overview of input handling for WebView2 inside Unity in our [Getting Started](https://learn.microsoft.com/microsoft-edge/webview2/gettingstarted/hololens2) documentation.

If you're new to using WebView2 on HoloLens, we recommend starting with the [Getting Started](https://learn.microsoft.com/microsoft-edge/webview2/gettingstarted/hololens2) guide.

## Prerequisites

- [Visual Studio](https://visualstudio.microsoft.com/vs/) 2019 or later, with .NET support installed.
- Latest version of the [Mixed Reality Feature Tool](https://learn.microsoft.com/en-us/windows/mixed-reality/develop/unity/welcome-to-mr-feature-tool)
- [Unity](https://unity.com/download) version 2020.3.35f\*

\*If you have a different 2020.3 version of the Unity Editor, other than 2020.3.35f, Unity Hub will offer you the ability to switch the project to a different editor version. This can cause unexpected issues. We recommend downloading the matching version of the editor.

## Build the WebView2 Windows Forms Browser

Follow these steps to get started with this sample code. WebView2 is already included as a Unity package, but you will need to load it via the Mixed Reality Feature Tool.

1. Clone this repository
1. Launch the Microsoft Mixed Reality Feature Tool
    - Click **Start**
    - Click the **...** button, navigate to the `WebView2HoloLensSample` folder, and select **Open**
    - With the project path selected, click **Restore Features** to load the required packages for the Mixed Reality Toolkit and the WebView2 plugin package.
1. Close the Mixed Reality Feature Tool
1. Launch Unity Hub
1. In Unity Hub, click **Open**, navigate to the `WebView2HoloLensSample` folder, and select **Open**. This should open the project in the Unity Editor.
1. In the Unity Editor, find the SampleScene in the `Assets/Scenes` folder and double-click to load it.
1. Click the play button in the Unity Editor to quickly test the app.

## Build and Deploy the app to a HoloLens device

Continuing from the above instructions, proceed with the following steps to deploy to a HoloLens device:

1. In the Unity Editor, select **File > Build Settings...**
1. Ensure *Universal App Platform* is selected as the platform. If not, select it and click **Switch Platform**.
1. Select HoloLens as the **Target Device**, then click **Build**.
1. Open the generated Visual Studio solution in Visual Studio.
1. Select **Release** or **Master** for the Solution Configuration. (Debug is available, but not recommended for performance reasons.)
1. Select **ARM64** as the Solution Platform.
1. Build the solution.
1. To deploy, choose either **Remote Machine** (over WiFi) or **Device** (over USB). [Read here](https://learn.microsoft.com/en-us/windows/mixed-reality/develop/advanced-concepts/using-visual-studio?tabs=hl2) for more detailed information on deploying to a HoloLens device.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact opencode@microsoft.com with any additional questions or comments.
