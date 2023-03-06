using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System.ComponentModel;
using System.Diagnostics;
using System;

namespace WebView2_UWP
{
    internal static class WebView2Extensions
    {
        public static string GetExtendedVersionString(this WebView2 webView2, bool full)
        {
            var versionString = "";
            var runtimeVersion = webView2.CoreWebView2.Environment.BrowserVersionString;
            var sdkVersion = GetSdkBuildVersion();

            if (full)
            {
                versionString = $"Runtime:{runtimeVersion}; SDK:{sdkVersion}";
            }
            else
            {
                versionString = $"{runtimeVersion}; {sdkVersion}";
            }

            return versionString;
        }

        public static string GetSdkBuildVersion()
        {
            CoreWebView2EnvironmentOptions options = new CoreWebView2EnvironmentOptions();

            var targetVersionMajorAndRest = options.TargetCompatibleBrowserVersion;
            var versionList = targetVersionMajorAndRest.Split('.');
            if (versionList.Length != 4)
            {
                return "Invalid SDK build version";
            }
            return versionList[2] + "." + versionList[3];
        }

        public static string GetRuntimePath(this WebView2 webView2)
        {
            int processId = (int)webView2.CoreWebView2.BrowserProcessId;
            try
            {
                Process process = Process.GetProcessById(processId);
                var fileName = process.MainModule.FileName;
                return System.IO.Path.GetDirectoryName(fileName);
            }
            catch (ArgumentException e)
            {
                return e.Message;
            }
            catch (InvalidOperationException e)
            {
                return e.Message;
            }
            // Occurred when a 32-bit process wants to access the modules of a 64-bit process.
            catch (Win32Exception e)
            {
                return e.Message;
            }
        }
    }
}
