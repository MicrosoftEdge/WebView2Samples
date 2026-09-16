using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.Storage;
using Windows.UI.Popups;

namespace WebView2_WinUI3_Sample
{
    internal class Scenario_FileHandle
    {
        private WebView2 _webView;

        public void Execute(WebView2 webView)
        {
            _webView = webView;
            webView.CoreWebView2.Settings.IsWebMessageEnabled = true;
            webView.CoreWebView2.PermissionRequested += CoreWebView2_PermissionRequested;
            webView.CoreWebView2.WebMessageReceived += CoreWebView2_WebMessageReceived;
            webView.CoreWebView2.SetVirtualHostNameToFolderMapping("content.app",
                Path.Combine(Package.Current.InstalledLocation.Path, "assets"),
                CoreWebView2HostResourceAccessKind.Allow);
            webView.Source = new Uri("https://content.app/ScenarioFileHandle.html");
        }

        private void SendDirAndFileHandleFromNativeToJS()
        {
            var dirHandle = _webView.CoreWebView2.Environment.CreateWebFileSystemDirectoryHandle(
                Path.Combine(Package.Current.InstalledLocation.Path, "assets"),
                CoreWebView2FileSystemHandlePermission.ReadWrite);
            var fileHandle = _webView.CoreWebView2.Environment.CreateWebFileSystemFileHandle(
                Path.Combine(Package.Current.InstalledLocation.Path, "assets", "ScenarioFileHandle.html"),
                CoreWebView2FileSystemHandlePermission.ReadOnly);
            _webView.CoreWebView2.PostWebMessageAsJson("{}", new List<object> { dirHandle, fileHandle });
        }

        private void CoreWebView2_PermissionRequested(CoreWebView2 sender, CoreWebView2PermissionRequestedEventArgs args)
        {
            //allow all permission requests for demo purpose
            args.State = CoreWebView2PermissionState.Allow;
        }

        private async void CoreWebView2_WebMessageReceived(CoreWebView2 sender, CoreWebView2WebMessageReceivedEventArgs args)
        {
            if (args.WebMessageAsJson == "\"requestWorkingDir\"")
            {
                SendDirAndFileHandleFromNativeToJS();
            }

            foreach (var item in args.AdditionalObjects)
            {
                if (item == null)
                {
                    var messageDialog = new ContentDialog()
                    {
                        Title = "Error",
                        Content = "Received additional object is <null>, Expected: CoreWebView2FileSystemHandle",
                        CloseButtonText = "Ok"
                    };
                    messageDialog.XamlRoot = _webView.XamlRoot;
                    await messageDialog.ShowAsync();
                }
                Debug.Assert(item is CoreWebView2FileSystemHandle);
                Debug.Assert(item != null);
            }
        }
    }
}
