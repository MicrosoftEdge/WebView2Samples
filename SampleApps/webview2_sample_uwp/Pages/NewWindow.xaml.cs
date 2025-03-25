// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Controls;

namespace WebView2_UWP.Pages
{
    public sealed partial class NewWindow : BasePage
    {
        public enum OptionEnum
        {
            DoNothing,
            Cancel,
            OpenUriInDefaultBrowser,
            OpenInCustomWindow
        };

        private class Option
        {
            public OptionEnum OptionEnum { get; set; }
            public string Name { get; set; }
            public string Description { get; set; }
        };

        private List<Option> options = new List<Option>()
        {
            new Option()
            {
                OptionEnum = OptionEnum.DoNothing,
                Name = "Open In WebView2 Window",
                Description = "Take no action and allow the WebView to handle the creation of the new window. WebView2 will open its own mini-browser window. " +
                "On Hololens 2, this will forward navigate to the requested URI instead of opening a new window."
            },
            new Option()
            {
                OptionEnum = OptionEnum.Cancel,
                Name = "Cancel",
                Description = "Cancel opening of the new window."
            },
            new Option()
            {
                OptionEnum = OptionEnum.OpenUriInDefaultBrowser,
                Name = "Open URI in default browser",
                Description = "Open the URI in the users default browser."
            },
            new Option()
            {
                OptionEnum = OptionEnum.OpenInCustomWindow,
                Name = "Open in custom window",
                Description = "Open the page in a custom host provided webview. In this example, the new window is opened in the TabView below the main WebView."
            },
        };

        private Option _selectedOption;
        private int _tabNameIndex = 0;

        public NewWindow()
        {
            this.InitializeComponent();
            WebView2.CoreWebView2Initialized += WebView2_CoreWebView2Initialized;
            WebView2.Source = new Uri("http://appassets.html.example/new_window.html");
        }

        private void WebView2_CoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            sender.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.html.example", "html", CoreWebView2HostResourceAccessKind.Allow);
            sender.CoreWebView2.NewWindowRequested += CoreWebView2_NewWindowRequested;
        }

        private async void CoreWebView2_NewWindowRequested(CoreWebView2 sender, CoreWebView2NewWindowRequestedEventArgs args)
        {
            if (_selectedOption.OptionEnum == OptionEnum.Cancel)
            {
                args.Handled = true;
            }
            else if (_selectedOption.OptionEnum == OptionEnum.OpenUriInDefaultBrowser)
            {
                args.Handled = true;
                await Windows.System.Launcher.LaunchUriAsync(new Uri(args.Uri));
            }
            else if (_selectedOption.OptionEnum == OptionEnum.OpenInCustomWindow)
            {
                var deferral = args.GetDeferral();

                var webView = new WebView2();
                webView.CoreWebView2Initialized += (WebView2 wv2, CoreWebView2InitializedEventArgs initArgs) =>
                {
                    string name = String.IsNullOrEmpty(args.Name) ?
                                  ("Tab " + (++_tabNameIndex).ToString()) : args.Name;

                    var tabViewItem = new TabViewItem
                    {
                        Header = name,
                        Content = webView
                    };

                    WebViewTabView.TabItems.Add(tabViewItem);
                    WebViewTabView.SelectedItem = tabViewItem;

                    args.Handled = true;
                    args.NewWindow = webView.CoreWebView2;
                    deferral.Complete();
                };

                await webView.EnsureCoreWebView2Async();
            }

            // If you do nothing with the NewWindowRequested event,
            // WebView2 will open its own mini-browser window.
        }

        private void WebViewTabView_TabCloseRequested(TabView sender, TabViewTabCloseRequestedEventArgs args)
        {
            sender.TabItems.Remove(args.Tab);
        }

        private void WindowOpenOptions_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (sender is RadioButtons rb)
            {
                _selectedOption = rb.SelectedItem as Option;
            }
        }
    }
}

