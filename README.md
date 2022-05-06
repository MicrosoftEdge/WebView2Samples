# WebView2 Samples

This repository contains getting started apps as well as sample apps that demonstrate the features and usage patterns of [WebView2](https://docs.microsoft.com/microsoft-edge/webview2/). As we add more features to WebView2, we will regularly update our samples.

In the ``GettingStarted`` folder you will find the starter code for its respective guide listed below:
- [Win32 Getting Started](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/win32)
- [WPF Getting Started](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/wpf)
- [WinForms Getting Started](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/winforms)
- [WinUI Getting Started](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/winui)

In the ``Sample Apps`` folder you will find:
- [WebView2Samples.sln](SampleApps/WebView2Samples.sln) - a collective solution that includes [WebView2APISample.vcxproj](SampleApps/WebView2APISample/WebView2APISample.vcxproj), [WebView2SampleWinComp.vcxproj](SampleApps/WebView2SampleWinComp/WebView2SampleWinComp.vcxproj), [WebView2WpfBrowser.csproj](SampleApps/WebView2WpfBrowser/WebView2WpfBrowser.csproj), [WebView2WindowsFormsBrowser.csproj](SampleApps/WebView2WindowsFormsBrowser/WebView2WindowsFormsBrowser.csproj), [WV2DeploymentWiXCustomActionSample](/SampleApps/WV2DeploymentWiXCustomActionSample/README.md), and [WV2DeploymentWiXBurnBundleSample](/SampleApps/WV2DeploymentWiXBurnBundleSample/README.md).

Please leave feedback in our [our feedback repo](https://aka.ms/webviewfeedback)!

## Win32 C/C++

#### Getting Started

Start with the [Win32 WebView2 getting-started guide](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/win32) to learn how to setup a WebView within a Win32 application.

This will require downloading the [Getting Started Guide](https://github.com/MicrosoftEdge/WebView2Samples/tree/main/GettingStartedGuide) directory.

#### Comprehensive API Sample

The **Win32 C++ Sample** can be found in the [WebView2APISample](./SampleApps/WebView2APISample) directory and [WebView2SampleWinComp](./SampleApps/WebView2SampleWinComp) directory.

#### Multiple WebViews Sample

The [WebView2Browser](https://github.com/MicrosoftEdge/WebView2Browser) is an additional Win32 WebView2 sample project that uses multiple WebViews in a single application. Clone this project by running `git clone https://github.com/MicrosoftEdge/WebView2Browser.git`.

Follow the [WebView2Browser guide](https://github.com/MicrosoftEdge/WebView2Browser) to learn how to build an application that utilizes multiple WebViews.

## .NET (WPF and Windows Forms)

#### Getting Started

* Checkout the [WPF getting started guide](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/wpf) for WebView2 in WPF applications
* Checkout the [Windows Forms getting started guide](https://docs.microsoft.com/microsoft-edge/webview2/gettingstarted/winforms) for WebView2 in WinForms applications

#### Comprehensive API Sample

* The **WPF Sample** can be found in the [WebView2WpfBrowser](./SampleApps/WebView2WpfBrowser) directory.
* The **Windows Forms Sample** can be found in the [WebView2WindowsFormsBrowser](./SampleApps/WebView2WindowsFormsBrowser) directory.

## UWP/WinUI

#### Comprehensive API Sample

The **UWP WinUI Sample** can be found in the [WinUI Controls Gallery](https://github.com/microsoft/Xaml-Controls-Gallery/tree/winui3preview).

## WebView2 Deployment

To help developers understand how to [deploy Evergreen WebView2 Runtime](https://docs.microsoft.com/microsoft-edge/webview2/concepts/distribution#deploying-the-evergreen-webview2-runtime) with your applications, we have the following samples:

* [WV2DeploymentWiXCustomActionSample](/SampleApps/WV2DeploymentWiXCustomActionSample/README.md) creates a [WiX](https://wixtoolset.org/) installer for [WebView2APISample](./SampleApps/WebView2APISample/README.md) and uses [WiX Custom Action](https://wixtoolset.org/documentation/manual/v3/wixdev/extensions/authoring_custom_actions.html) to chain-install the Evergreen WebView2 Runtime.
* [WV2DeploymentWiXBurnBundleSample](/SampleApps/WV2DeploymentWiXBurnBundleSample/README.md) creates a [WiX](https://wixtoolset.org/) installer for [WebView2APISample](./SampleApps/WebView2APISample/README.md) and uses [WiX Burn Bundle](https://wixtoolset.org/documentation/manual/v3/bundle/) to chain-install the Evergreen WebView2 Runtime.
* [WV2DeploymentVSInstallerSample](/SampleApps/WV2DeploymentVSInstallerSample/README.md) uses the [Microsoft Visual Studio Installer Projects](https://marketplace.visualstudio.com/items?itemName=visualstudioclient.MicrosoftVisualStudio2017InstallerProjects) to create an installer for [WebView2APISample](./SampleApps/WebView2APISample/README.md) and chain-install the Evergreen WebView2 Runtime.

## Next Steps

* Checkout [Introduction to Microsoft Edge WebView2](https://aka.ms/webview) to learn more about WebView2
* Please leave us feedback specifically about the samples in [this repo](https://github.com/MicrosoftEdge/WebView2Samples/issues).
* Please leave us feedback about the WebView2 control in our [feedback repo](https://aka.ms/webviewfeedback).
