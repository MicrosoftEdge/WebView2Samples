---
description: "Demonstrate the features and usage patterns of WebView2 in Win32."
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
urlFragment: WebView2APISample
---
# WebView2 API Sample

<!-- only enough info to differentiate this sample vs. the others; what is different about this sample compared to the sibling samples? -->
This sample, **WebView2APISample**, embeds a Microsoft Edge WebView2 control within a Win32 native application.

This sample is built as a Win32 Visual Studio 2019 project.  It uses C++ in the native environment together with HTML/CSS/JavaScript in the WebView2 environment.  This sample showcases many of WebView2's event handlers and API methods that allow a native Win32 application to directly interact with a WebView, and vice versa.

To use this sample, see [Win32 sample app](https://learn.microsoft.com/microsoft-edge/webview2/samples/webview2apissample).

![The WebView2APISample sample app running](./documentation/screenshots/sample-app-screenshot.png)
