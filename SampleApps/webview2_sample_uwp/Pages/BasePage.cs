// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using Microsoft.UI.Xaml.Controls;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace WebView2_UWP.Pages
{
    public class BasePage : Page
    {
        public BasePage()
        {
            Unloaded += BasePage_Unloaded;
        }

        private void BasePage_Unloaded(object sender, RoutedEventArgs e)
        {
            // The garbage collector can be slow to dispose of the
            // webviews. Manually free the resources being used by
            // the webviews since they are no longer needed after
            // the page has been unloaded.
            CloseAllWebViews(this);
        }

        private void CloseAllWebViews(DependencyObject root)
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
            {
                var childVisual = VisualTreeHelper.GetChild(root, i);
                CloseAllWebViews(childVisual);

                if (childVisual is WebView2 webView)
                {
                    webView.Close();
                }
            }
        }
    }
}
