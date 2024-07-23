// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections.Generic;
using System.Windows;
using Microsoft.Web.WebView2.Core;

namespace WebView2WpfBrowser {
    /// <summary>
    /// Interaction logic for SaveAsDialog.xaml
    /// </summary>
public partial class SaveAsDialog : Window {
    public SaveAsDialog(List<CoreWebView2SaveAsKind> kind = null) {
        InitializeComponent();
        SaveAsKind.ItemsSource = kind;
        SaveAsKind.SelectedIndex = 0;
    }
    void OK_Clicked(object sender, RoutedEventArgs args) { this.DialogResult = true; }
    void CANCEL_Clicked(object sender, RoutedEventArgs args) { this.DialogResult = false; }
    }
}
