# Introduction
This sample shows off using a WebView2 control in a WinUI 3 Windows SDK Packaged application.

This sample also shows how to optionally update the application to ship packaged together with a fixed version of the WebView2 Runtime, instead of using the version of the WebView2 Runtime that's installed on the user's computer.

# Relevant directories

| Directory | Contents |
--- | --- |
| WebView2_WinUI3_Sample | Project code |
| WebView2_WinUI3_Sample (Package) | Packaging and distribution project |
| WebView2_WinUI3_Sample (Package)\FixedRuntime | (Optional) Fixed WebView2 runtime |
| WebView2_WinUI3_Sample (Package)\FixedRuntime\95.0.1020.53 | (Optional) Fixed WebView2 runtime sample |


# Fixed version usage
If you want to ship a fixed version of the WebView2 Runtime packaged with your application, you will need to include the WebView2 Runtime in your project.

Instructions can be found at: https://docs.microsoft.com/microsoft-edge/webview2/concepts/distribution

The following instructions assume you are using WebView2 Runtime version 95.0.1020.53.  You'll change this number to whatever version you are using.

You will need to:
1. Include the fixed version of the WebView2 Runtime in the package project:

   `\WebView2_WinUI3_Sample\WebView2_WinUI3_Sample (Package)\FixedRuntime\95.0.1020.53\`
2 Update the package project wapproj file for the version your using
<pre> < Content Include="FixedRuntime\95.0.1020.53\\**\*.*" > </pre>
3 Uncomment the code in app.xaml.cs to enable the runtime override
<pre>
public App()
{
    this.InitializeComponent();
    // If you are shipping a fixed version WebView2 SDK with your application you will need
    // to use the following code (update the runtime version to what you are shipping.
    StorageFolder localFolder = Windows.ApplicationModel.Package.Current.InstalledLocation;
    String fixedPath = Path.Combine(localFolder.Path, "FixedRuntime\\95.0.1020.53");
    Debug.WriteLine($"Launch path [{localFolder.Path}]");
    Debug.WriteLine($"FixedRuntime path [{fixedPath}]");
    Environment.SetEnvironmentVariable("WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", fixedPath);
}
</pre>
4 Update the version information for the appropriate version
<pre> 
String fixedPath = Path.Combine(localFolder.Path, "FixedRuntime\\95.0.1020.53");
</pre>





