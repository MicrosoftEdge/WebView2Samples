// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Web.WebView2.Core;

namespace WebView2WpfBrowser
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		public static RoutedCommand AddRemoteObjectCommand = new RoutedCommand();

		private bool _isNavigating = false;

		public MainWindow()
		{
			InitializeComponent();
		}

		void BackCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
		{
			e.CanExecute = webView != null && webView.CoreWebView2 != null && webView.CanGoBack;
		}

		void BackCmdExecuted(object target, ExecutedRoutedEventArgs e)
		{
			webView.CoreWebView2.GoBack();
		}

		void ForwardCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
		{
			e.CanExecute = webView != null && webView.CoreWebView2 != null && webView.CanGoForward;
		}

		void ForwardCmdExecuted(object target, ExecutedRoutedEventArgs e)
		{
			webView.CoreWebView2.GoForward();
		}

		void RefreshCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
		{
			e.CanExecute = webView != null && webView.CoreWebView2 != null && !_isNavigating;
		}

		void RefreshCmdExecuted(object target, ExecutedRoutedEventArgs e)
		{
			webView.CoreWebView2.Reload();
		}

		void BrowseStopCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
		{
			e.CanExecute = webView != null && webView.CoreWebView2 != null && _isNavigating;
		}

		void BrowseStopCmdExecuted(object target, ExecutedRoutedEventArgs e)
		{
			webView.CoreWebView2.Stop();
		}
		void GoToPageCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
		{
			e.CanExecute = webView != null && webView.CoreWebView2 != null && !_isNavigating;
		}

		void GoToPageCmdExecuted(object target, ExecutedRoutedEventArgs e)
		{
			webView.CoreWebView2.Navigate((string)e.Parameter);
		}
		private void webView_NavigationStarting(object sender, CoreWebView2NavigationStartingEventArgs e)
		{
			_isNavigating = true;
			RequeryCommands();
		}

		private void webView_NavigationCompleted(object sender, CoreWebView2NavigationCompletedEventArgs e)
		{
			_isNavigating = false;
			RequeryCommands();
		}

		private void RequeryCommands()
		{
			// Seems like there should be a way to bind CanExecute directly to a bool property
			// so that the binding can take care keeping CanExecute up-to-date when the property's
			// value changes, but apparently there isn't.  Instead we listen for the WebView events
			// which signal that one of the underlying bool properties might have changed and
			// bluntly tell all commands to re-check their CanExecute status.
			//
			// Another way to trigger this re-check would be to create our own bool dependency 
			// properties on this class, bind them to the underlying properties, and implement a
			// PropertyChangedCallback on them.  That arguably more directly binds the status of
			// the commands to the WebView's state, but at the cost of having an extraneous
			// dependency property sitting around for each underlying property, which doesn't seem
			// worth it, especially given that the WebView API explicitly documents which events
			// signal the property value changes.
			CommandManager.InvalidateRequerySuggested();
		}
	}
}
