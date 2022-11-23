// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace WebView2_UWP.Pages
{
    public sealed partial class PopupsAndDialogs : Page
    {
        public PopupsAndDialogs()
        {
            this.InitializeComponent();

            WebView2.CoreWebView2Initialized += WebView2_CoreWebView2Initialized;
            WebView2.Source = new Uri("http://appassets.html.example/popups_and_dialogs.html");
        }

        private async void WebView2_CoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            WebView2.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.html.example", "html", CoreWebView2HostResourceAccessKind.Allow);
        }

        private async Task<Rect> GetElementBoundingClientRect(string elementId)
        {
            elementId = JsonValue.CreateStringValue(elementId).Stringify();
            string snippet = $"getElementBoundingClientRect({elementId})";
            string scriptResult = await WebView2.CoreWebView2.ExecuteScriptAsync(snippet);
            var rootObject = JsonObject.Parse(scriptResult);
            var x = rootObject.GetNamedNumber("x");
            var y = rootObject.GetNamedNumber("y");
            var width = rootObject.GetNamedNumber("width");
            var height = rootObject.GetNamedNumber("height");
            return new Rect(x, y, width, height);
        }

        private void ShowDownloadsDialogButton_Click(object sender, RoutedEventArgs e)
        {
            WebView2.CoreWebView2.OpenDefaultDownloadDialog();
        }

        private async void ShowContextMenuButton_Click(object sender, RoutedEventArgs e)
        {
            string args = @"{""type"": ""mousePressed"", ""button"": ""right"", ""x"": 1, ""y"": 1}";
            await WebView2.CoreWebView2.CallDevToolsProtocolMethodAsync("Input.dispatchMouseEvent", args);

            args = @"{""type"": ""mouseReleased"", ""button"": ""right"", ""x"": 1, ""y"": 1}";
            await WebView2.CoreWebView2.CallDevToolsProtocolMethodAsync("Input.dispatchMouseEvent", args);
        }

        private async void ShowStatusBarButton_Click(object sender, RoutedEventArgs e)
        {
            var rect = await GetElementBoundingClientRect("StatusBarLink");

            string args = $"{{\"type\": \"mouseMoved\", \"button\": \"none\", \"x\": {rect.X}, \"y\": {rect.Y}}}";
            await WebView2.CoreWebView2.CallDevToolsProtocolMethodAsync("Input.dispatchMouseEvent", args);
        }

        private async void ShowUploadFileDialogButton_Click(object sender, RoutedEventArgs e)
        {
            string snippet = "showFileUploadDialog();";
            await WebView2.CoreWebView2.ExecuteScriptAsync(snippet);
        }

        // To create a custom alert dialog and not use the default provided by WebView2,
        // see https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2.scriptdialogopening
        private async void ShowAlertDialogButton_Click(object sender, RoutedEventArgs e)
        {
            string snippet = "window.alert(\"alert\");";
            await WebView2.CoreWebView2.ExecuteScriptAsync(snippet);
        }

        // To create a custom confirm dialog and not use the default provided by WebView2,
        // see https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2.scriptdialogopening
        private async void ShowConfirmDialogButton_Click(object sender, RoutedEventArgs e)
        {
            string snippet = "window.confirm(\"confirm\");";
            await WebView2.CoreWebView2.ExecuteScriptAsync(snippet);
        }

        // To create a custom prompt dialog and not use the default provided by WebView2,
        // see https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2.scriptdialogopening
        private async void ShowPromptDialogButton_Click(object sender, RoutedEventArgs e)
        {
            string snippet = "window.prompt(\"prompt\");";
            await WebView2.CoreWebView2.ExecuteScriptAsync(snippet);
        }
    }
}
