// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Controls;

namespace WebView2_UWP
{
    public sealed partial class ExecuteJavascript : Page
    {
        Dictionary<string, string> codeSnippets = new Dictionary<string, string>()
        {
            {"Update header", "document.getElementById(\"header\").innerHTML = \"New Header\";" },
            {"Remove the body background", "list = document.getElementsByTagName(\"body\")[0].classList;\nlist.remove(\"webviewhero_bg\");" },
            {"Add the body background", "list = document.getElementsByTagName(\"body\")[0].classList;\nlist.add(\"webviewhero_bg\");" },
            {"Show an alert", "window.alert(\"Alert message\");" },
            {"Retrieve text from the content area", "document.getElementById(\"content\").innerHTML;" },
            {"Call a function and get the return value", "Add(9, 3);" },
        };

        public ExecuteJavascript()
        {
            this.InitializeComponent();
            WebView2.CoreWebView2Initialized += WebView2_CoreWebView2Initialized;
            WebView2.Source = new Uri("http://appassets.html.example/execute_javascript.html");
        }

        private async void WebView2_CoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            sender.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.html.example", "html", CoreWebView2HostResourceAccessKind.Allow);
        }

        private void OnCodeSnippetChanged(object sender, SelectionChangedEventArgs e)
        {
            if (CodeSnippetsCombo.SelectedItem != null)
            {
                var snippet = ((KeyValuePair<string, string>)CodeSnippetsCombo.SelectedItem).Value;
                CodeSnippetTextBox.Text = snippet;
            }
        }

        private async void OnExecuteJavascriptButtonClicked(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            string snippet = CodeSnippetTextBox.Text;
            string scriptResult = await WebView2.CoreWebView2.ExecuteScriptAsync(snippet);

            if (scriptResult != null)
            {
                ReturnedValueTextBox.Text = scriptResult;
            }
        }
    }
}

