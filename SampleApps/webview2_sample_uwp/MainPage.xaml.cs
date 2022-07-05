// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

 #pragma warning disable CS8305 //'Microsoft.UI.Xaml.Controls.WebView2' is for evaluation purposes only and is subject to change or removal in future updates.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Diagnostics;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace webview2_sample_uwp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            InitializeWebView2Async();

            StatusUpdate("Ready");
        }

        private async void InitializeWebView2Async()
        {
            await WebView2.EnsureCoreWebView2Async();

            var dispatchAdapter = new WinRTAdapter.DispatchAdapter();
            WebView2.CoreWebView2.AddHostObjectToScript("Windows", dispatchAdapter.WrapNamedObject("Windows", dispatchAdapter));
            WebView2.CoreWebView2.AddHostObjectToScript("CsRuntimeComponent", dispatchAdapter.WrapNamedObject("CsRuntimeComponent", dispatchAdapter));

            await WebView2.CoreWebView2.AddScriptToExecuteOnDocumentCreatedAsync(
                "(() => {" +
                        "if (chrome && chrome.webview) {" +
                            "console.log('Setting up WinRT projection options');" +
                            "chrome.webview.hostObjects.options.defaultSyncProxy = true;" +
                            "chrome.webview.hostObjects.options.forceAsyncMethodMatches = [/Async$/];" +
                            "chrome.webview.hostObjects.options.ignoreMemberNotFoundError = true;" +
                            "window.Windows = chrome.webview.hostObjects.sync.Windows;" +
                            "window.CsRuntimeComponent = chrome.webview.hostObjects.sync.CsRuntimeComponent;" +
                        "}" +
                    "})();");

            WebView2.CoreWebView2.NavigateToString(
                "<h1>Example WinRT Event</h1><button id='button'>Raise Event</button><br/>Result: <span id='result'></span>" +
                "<script>" +
                "document.getElementById('button').addEventListener('click', () => {" +
                "   let csRuntimeClass = new CsRuntimeComponent.CsRuntimeClass();" +
                "   csRuntimeClass.stringProperty = 'stringProperty value';" +
                "   csRuntimeClass.addEventListener('exampleEvent', eventArg => {" +
                "       console.log('event raised');" +
                "       document.getElementById('result').textContent = eventArg;" +
                "   });" +
                "   csRuntimeClass.async().invokeExampleEvent();" +
                "});" +
                "</script>");
        }

        private void StatusUpdate(string message)
        {
            StatusBar.Text = message;
            Debug.WriteLine(message);
        }

        private bool TryCreateUri(String potentialUri, out Uri result)
        {
            StatusUpdate("TryCreateUri");

            Uri uri;
            if ((Uri.TryCreate(potentialUri, UriKind.Absolute, out uri) || Uri.TryCreate("http://" + potentialUri, UriKind.Absolute, out uri)) &&
                (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps))
            {
                result = uri;
                return true;
            }
            else
            {
                StatusUpdate("Unable to configure URI");
                result = null;
                return false;
            }
        }

        private void TryNavigate()
        {
            StatusUpdate("TryNavigate");

            Uri destinationUri;
            if (TryCreateUri(AddressBar.Text, out destinationUri))
            {
                WebView2.Source = destinationUri;
            }
            else
            {
                StatusUpdate("URI couldn't be figured out use it as a bing search term");

                String bingString = "https://www.bing.com/search?q=" + Uri.EscapeUriString(AddressBar.Text);
                if (TryCreateUri(bingString, out destinationUri))
                {
                    AddressBar.Text = destinationUri.AbsoluteUri;
                    WebView2.Source = destinationUri;
                }
                else
                {
                    StatusUpdate("URI couldn't be configured as bing search term, giving up");
                }
            }
        }

        private void Go_OnClick(object sender, RoutedEventArgs e)
        {
            StatusUpdate("Go_OnClick: " + AddressBar.Text);

            TryNavigate();
        }

        private void AddressBar_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                StatusUpdate("AddressBar_KeyDown [Enter]: " + AddressBar.Text);

                e.Handled = true;
                TryNavigate();
            }
        }
    }
}
