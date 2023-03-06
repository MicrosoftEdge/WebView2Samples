// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using Windows.UI.Xaml.Controls;

namespace WebView2_UWP.Pages
{
    public sealed partial class ScriptDebugging : Page
    {
        public ScriptDebugging()
        {
            this.InitializeComponent();
            WebView2.CoreWebView2Initialized += WebView2_CoreWebView2Initialized;
        }

        private async void WebView2_CoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            sender.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.html.example", "html", CoreWebView2HostResourceAccessKind.Allow);
        }

        private async void OnJavaScriptLocalFileButtonClicked(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            WebView2.Source = new Uri("http://appassets.html.example/ScenarioJavaScriptDebugIndex.html");
        }
        private async void OnTypeScriptLocalFileButtonClicked(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            WebView2.Source = new Uri("http://appassets.html.example/ScenarioTypeScriptDebugIndex.html");
        }
    }
}

