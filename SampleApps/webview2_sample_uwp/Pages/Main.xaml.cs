// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections.Generic;
using System.Linq;
using webview2_sample_uwp;
using WebView2_UWP.Pages;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Animation;
using MUXC = Microsoft.UI.Xaml.Controls;

namespace WebView2_UWP
{
    public sealed partial class MainPage : Page
    {
        // List of ValueTuple holding the Navigation Tag
        // and its corresponding Page.
        private readonly List<(string Tag, Type Page)> _pages = new List<(string Tag, Type Page)>
        {
            ("browser", typeof(Browser)),
            ("execute_javascript", typeof(ExecuteJavascript)),
            ("add_host_object", typeof(AddHostObject)),
            ("new_window", typeof(NewWindow)),
            ("popups_and_dialogs", typeof(PopupsAndDialogs)),
        };

        public MainPage()
        {
            this.InitializeComponent();
        }

        private void NavView_Loaded(object sender, RoutedEventArgs e)
        {
            // NavView doesn't load any page by default, so load browser page.
            NavView.SelectedItem = NavView.MenuItems[0];
            NavView_Navigate("browser", new EntranceNavigationTransitionInfo());
        }

        private void NavView_ItemInvoked(MUXC.NavigationView sender, MUXC.NavigationViewItemInvokedEventArgs args)
        {
            if ((args.InvokedItemContainer != null) && (args.InvokedItemContainer.Tag != null))
            {
                var navItemTag = args.InvokedItemContainer.Tag.ToString();
                NavView_Navigate(navItemTag, args.RecommendedNavigationTransitionInfo);
            }
        }

        private void NavView_Navigate(
            string navItemTag,
            NavigationTransitionInfo transitionInfo)
        {
            var index = _pages.FindIndex(p => p.Tag.Equals(navItemTag));
            if (index >= 0)
            {
                var page = _pages[index].Page;

                // Get the page type before navigation to
                // prevent duplicate entries in the backstack.
                var preNavPageType = ContentFrame.CurrentSourcePageType;

                // Only navigate if the selected page isn't currently loaded.
                if (!Type.Equals(preNavPageType, page))
                {
                    ContentFrame.Navigate(page, null, transitionInfo);
                }
            }
        }

        private void ContentFrame_NavigationFailed(object sender, Windows.UI.Xaml.Navigation.NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page " + e.SourcePageType.FullName);
        }
    }
}
