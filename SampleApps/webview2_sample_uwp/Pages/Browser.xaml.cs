// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma warning disable CS8305 //'Microsoft.UI.Xaml.Controls.WebView2' is for evaluation purposes only and is subject to change or removal in future updates.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;
using System;
using System.Diagnostics;
using System.IO;
using webview2_sample_uwp;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
namespace WebView2_UWP.Pages
{
    public sealed partial class Browser : BasePage
    {
        private string _homeUrl = "https://developer.microsoft.com/en-us/microsoft-edge/webview2/";

        public Browser()
        {
            InitializeComponent();
            AddressBar.Text = _homeUrl;

            WebView2.CoreWebView2Initialized += WebView2_CoreWebView2Initialized;
            WebView2.NavigationStarting += WebView2_NavigationStarting;
            WebView2.NavigationCompleted += WebView2_NavigationCompleted;

            StatusUpdate("Ready");
            WebView2.Source = new Uri(AddressBar.Text);
        }

        private void WebView2_CoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            if (args.Exception != null)
            {
                StatusUpdate($"Error initializing WebView2: {args.Exception.Message}");
            }
            else
            {
                App.Instance.UpdateAppTitle(sender.GetExtendedVersionString(false));

                WebView2.CoreWebView2.DownloadStarting += CoreWebView2_DownloadStarting;
                WebView2.CoreWebView2.HistoryChanged += CoreWebView2_HistoryChanged;
            }
        }

        // <DownloadStarting>
        private void CoreWebView2_DownloadStarting(CoreWebView2 sender, CoreWebView2DownloadStartingEventArgs args)
        {
            // Developer can obtain a deferral for the event so that the CoreWebView2
            // doesn't examine the properties we set on the event args until
            // after the deferral completes asynchronously.
            var deferral = args.GetDeferral();

            // We avoid potential reentrancy from running a message loop in the download
            // starting event handler by showing our download dialog later when we
            // complete the deferral asynchronously.
            System.Threading.SynchronizationContext.Current.Post(async(_) =>
            {
                using (deferral)
                {
                    // Hide the default download dialog.
                    args.Handled = true;
                    var panel = new StackPanel();
                    panel.Children.Add(new TextBlock
                    {
                        Text = "Enter new result file path or select OK to keep default path. " +
                        "Select cancel to cancel the download. Please enter a path that the " +
                        "app has access to write to, otherwise the download will fail.",
                        TextWrapping = TextWrapping.Wrap,
                    });
                    TextBox inputTextBox = new TextBox();
                    inputTextBox.Text = args.ResultFilePath;
                    inputTextBox.AcceptsReturn = false;
                    panel.Children.Add(inputTextBox);
                    ContentDialog dialog = new ContentDialog();
                    dialog.Content = panel;
                    dialog.Title = "Download Starting";
                    dialog.IsSecondaryButtonEnabled = true;
                    dialog.PrimaryButtonText = "Ok";
                    dialog.SecondaryButtonText = "Cancel";
                    if (await dialog.ShowAsync() == ContentDialogResult.Primary)
                    {
                        args.ResultFilePath = inputTextBox.Text;
                        UpdateProgress(args.DownloadOperation);
                    }
                    else
                    {
                        args.Cancel = true;
                    }
                }
            }, null);
        }
        // </DownloadStarting>

        // Update download progress
        void UpdateProgress(CoreWebView2DownloadOperation download)
        {
            // <BytesReceivedChanged>
            download.BytesReceivedChanged += delegate (CoreWebView2DownloadOperation sender, object e)
            {
                // Here developer can update download dialog to show progress of a
                // download using `download.BytesReceived` and `download.TotalBytesToReceive`
            };
            // </BytesReceivedChanged>

            // <StateChanged>
            download.StateChanged += delegate (CoreWebView2DownloadOperation sender, object e)
            {
                switch (download.State)
                {
                    case CoreWebView2DownloadState.InProgress:
                        break;
                    case CoreWebView2DownloadState.Interrupted:
                        // Here developer can take different actions based on `download.InterruptReason`.
                        // For example, show an error message to the end user.
                        break;
                    case CoreWebView2DownloadState.Completed:
                        break;
                }
            };
            // </StateChanged>
        }

        private void StatusUpdate(string message)
        {
            StatusBar.Text = message;
            Debug.WriteLine(message);
        }

        private void WebView2_NavigationStarting(WebView2 sender, CoreWebView2NavigationStartingEventArgs args)
        {
            RefreshButton.IsEnabled = false;
            CancelButton.IsEnabled = true;
        }

        private void WebView2_NavigationCompleted(WebView2 sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            StatusUpdate("Navigation complete");

            RefreshButton.IsEnabled = true;
            CancelButton.IsEnabled = false;

            // Update the address bar with the full URL that was navigated to.
            AddressBar.Text = sender.Source.ToString();
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

        private void AddressBar_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                StatusUpdate("AddressBar_KeyDown [Enter]: " + AddressBar.Text);

                e.Handled = true;
                TryNavigate();
            }
        }

        private void CoreWebView2_HistoryChanged(CoreWebView2 sender, object args)
        {
            BackButton.IsEnabled = sender.CanGoBack;
            ForwardButton.IsEnabled = sender.CanGoForward;
        }

        private void OnBackButtonClick(object sender, RoutedEventArgs e)
        {
            WebView2.GoBack();
        }

        private void OnForwardButtonClick(object sender, RoutedEventArgs e)
        {
            WebView2.GoForward();
        }

        private void OnReloadButtonClick(object sender, RoutedEventArgs e)
        {
            WebView2.Reload();
        }

        private void OnCancelButtonClick(object sender, RoutedEventArgs e)
        {
            WebView2.CoreWebView2.Stop();
        }

        private void OnGoButtonClick(object sender, RoutedEventArgs e)
        {
            StatusUpdate("OnGoButtonClick: " + AddressBar.Text);

            TryNavigate();
        }

        private void OnHomeButtonClick(object sender, RoutedEventArgs e)
        {
            WebView2.Source = new Uri(_homeUrl);
        }

        private void OnDevToolsButtonClick(object sender, RoutedEventArgs e)
        {
            WebView2.CoreWebView2.OpenDevToolsWindow();
        }
    }
}
