// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using AddHostObjectBridgeComponent;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Controls;

namespace WebView2_UWP.Pages
{
    public sealed partial class AddHostObject : BasePage
    {
        private Bridge _bridge;

        public AddHostObject()
        {
            _bridge = new Bridge();
            _bridge.ItemsChangedEvent += UpdateItemsList;

            this.InitializeComponent();

            InitializeWebView2Async();
        }

        private async void InitializeWebView2Async()
        {
            await WebView2.EnsureCoreWebView2Async();

            WebView2.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.html.example", "html", CoreWebView2HostResourceAccessKind.Allow);
            WebView2.Source = new Uri("http://appassets.html.example/add_host_object.html");

            var dispatchAdapter = new WinRTAdapter.DispatchAdapter();
            WebView2.CoreWebView2.AddHostObjectToScript("BridgeInstance", dispatchAdapter.WrapObject(_bridge, dispatchAdapter));
        }

        private void UpdateItemsList(object sender, IList<string> items)
        {
            ItemsListTextBox.Text = String.Join("\n", items);
        }

        private void ItemButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            var text = (sender as Button).Content as string;
            _bridge.AppendToItems(text);
        }

        private void ClearButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            _bridge.ClearItems();
        }
    }
}

