// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
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
using Microsoft.Web.WebView2.Wpf;

namespace WebView2WpfBrowser
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static RoutedCommand InjectScriptCommand = new RoutedCommand();
        public static RoutedCommand NavigateWithWebResourceRequestCommand = new RoutedCommand();
        public static RoutedCommand DOMContentLoadedCommand = new RoutedCommand();
        public static RoutedCommand GetCookiesCommand = new RoutedCommand();
        public static RoutedCommand SuspendCommand = new RoutedCommand();
        public static RoutedCommand ResumeCommand = new RoutedCommand();
        public static RoutedCommand CheckUpdateCommand = new RoutedCommand();
        public static RoutedCommand NewBrowserVersionCommand = new RoutedCommand();
        public static RoutedCommand CustomClientCertificateSelectionCommand = new RoutedCommand();
        public static RoutedCommand DeferredCustomCertificateDialogCommand = new RoutedCommand();
        public static RoutedCommand BackgroundColorCommand = new RoutedCommand();
        public static RoutedCommand DownloadStartingCommand = new RoutedCommand();
        public static RoutedCommand AddOrUpdateCookieCommand = new RoutedCommand();
        public static RoutedCommand DeleteCookiesCommand = new RoutedCommand();
        public static RoutedCommand DeleteAllCookiesCommand = new RoutedCommand();
        public static RoutedCommand SetUserAgentCommand = new RoutedCommand();
        public static RoutedCommand PasswordAutosaveCommand = new RoutedCommand();
        public static RoutedCommand GeneralAutofillCommand = new RoutedCommand();
        public static RoutedCommand PinchZoomCommand = new RoutedCommand();
        public static RoutedCommand SwipeNavigationCommand = new RoutedCommand();
        bool _isNavigating = false;

        CoreWebView2Settings _webViewSettings;
        CoreWebView2Settings WebViewSettings
        {
            get
            {
                if (_webViewSettings == null && webView?.CoreWebView2 != null)
                {
                    _webViewSettings = webView.CoreWebView2.Settings;
                }
                return _webViewSettings;
            }
        }
        CoreWebView2Environment _webViewEnvironment;
        CoreWebView2Environment WebViewEnvironment
        {
            get
            {
                if (_webViewEnvironment == null && webView?.CoreWebView2 != null)
                {
                    _webViewEnvironment = webView.CoreWebView2.Environment;
                }
                return _webViewEnvironment;
            }
        }

        public MainWindow()
        {
            InitializeComponent();
            AttachControlEventHandlers(webView);
        }

        void AttachControlEventHandlers(WebView2 control)
        {
            control.NavigationStarting += WebView_NavigationStarting;
            control.NavigationCompleted += WebView_NavigationCompleted;
            control.CoreWebView2InitializationCompleted += WebView_CoreWebView2InitializationCompleted;
            control.KeyDown += WebView_KeyDown;
        }

        bool IsWebViewValid()
        {
            try
            {
                return webView != null && webView.CoreWebView2 != null;
            }
            catch (Exception ex) when (ex is ObjectDisposedException || ex is InvalidOperationException)
            {
                return false;
            }
        }

        void NewCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
            new MainWindow().Show();
        }

        void CloseCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }

        void BackCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = webView != null && webView.CanGoBack;
        }

        void BackCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.GoBack();
        }

        void ForwardCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = webView != null && webView.CanGoForward;
        }

        void ForwardCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.GoForward();
        }

        void RefreshCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = IsWebViewValid() && !_isNavigating;
        }

        void RefreshCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.Reload();
        }

        void BrowseStopCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = IsWebViewValid() && _isNavigating;
        }

        void BrowseStopCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.Stop();
        }

        void WebViewRequiringCmdsCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = webView != null;
        }

        void CoreWebView2RequiringCmdsCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = IsWebViewValid();
        }

        void CustomClientCertificateSelectionCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            EnableCustomClientCertificateSelection();
        }
        void DeferredCustomCertificateDialogCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            DeferredCustomClientCertificateSelectionDialog();
        }

        private bool _isControlInVisualTree = true;

        void RemoveControlFromVisualTree(WebView2 control)
        {
            Layout.Children.Remove(control);
            _isControlInVisualTree = false;
        }

        void AttachControlToVisualTree(WebView2 control)
        {
            Layout.Children.Add(control);
            _isControlInVisualTree = true;
        }

        WebView2 GetReplacementControl(bool useNewEnvironment)
        {
            WebView2 replacementControl = new WebView2();
            ((System.ComponentModel.ISupportInitialize)(replacementControl)).BeginInit();
            // Setup properties and bindings.
            if (useNewEnvironment)
            {
                // Create a new CoreWebView2CreationProperties instance so the environment
                // is made anew.
                replacementControl.CreationProperties = new CoreWebView2CreationProperties();
                replacementControl.CreationProperties.BrowserExecutableFolder = webView.CreationProperties.BrowserExecutableFolder;
                replacementControl.CreationProperties.Language = webView.CreationProperties.Language;
                replacementControl.CreationProperties.UserDataFolder = webView.CreationProperties.UserDataFolder;
                shouldAttachEnvironmentEventHandlers = true;
            }
            else
            {
                replacementControl.CreationProperties = webView.CreationProperties;
            }
            Binding urlBinding = new Binding()
            {
                Source = replacementControl,
                Path = new PropertyPath("Source"),
                Mode = BindingMode.OneWay
            };
            url.SetBinding(TextBox.TextProperty, urlBinding);

            AttachControlEventHandlers(replacementControl);
            replacementControl.Source = webView.Source ?? new Uri("https://www.bing.com");
            ((System.ComponentModel.ISupportInitialize)(replacementControl)).EndInit();

            return replacementControl;
        }

        void WebView_ProcessFailed(object sender, CoreWebView2ProcessFailedEventArgs e)
        {
            void ReinitIfSelectedByUser(CoreWebView2ProcessFailedKind kind)
            {
                string caption;
                string message;
                if (kind == CoreWebView2ProcessFailedKind.BrowserProcessExited)
                {
                    caption = "Browser process exited";
                    message = "WebView2 Runtime's browser process exited unexpectedly. Recreate WebView?";
                }
                else
                {
                    caption = "Web page unresponsive";
                    message = "WebView2 Runtime's render process stopped responding. Recreate WebView?";
                }

                var selection = MessageBox.Show(message, caption, MessageBoxButton.YesNo);
                if (selection == MessageBoxResult.Yes)
                {
                    // The control cannot be re-initialized so we setup a new instance to replace it.
                    // Note the previous instance of the control is disposed of and removed from the
                    // visual tree before attaching the new one.
                    if (_isControlInVisualTree)
                    {
                        RemoveControlFromVisualTree(webView);
                    }
                    webView.Dispose();
                    webView = GetReplacementControl(false);
                    AttachControlToVisualTree(webView);
                }
            }

            void ReloadIfSelectedByUser(CoreWebView2ProcessFailedKind kind)
            {
                string caption;
                string message;
                if (kind == CoreWebView2ProcessFailedKind.RenderProcessExited)
                {
                    caption = "Web page unresponsive";
                    message = "WebView2 Runtime's render process exited unexpectedly. Reload page?";
                }
                else
                {
                    caption = "App content frame unresponsive";
                    message = "WebView2 Runtime's render process for app frame exited unexpectedly. Reload page?";
                }

                var selection = MessageBox.Show(message, caption, MessageBoxButton.YesNo);
                if (selection == MessageBoxResult.Yes)
                {
                    webView.Reload();
                }
            }

            bool IsAppContentUri(Uri source)
            {
                // Sample virtual host name for the app's content.
                // See CoreWebView2.SetVirtualHostNameToFolderMapping: https://docs.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2.setvirtualhostnametofoldermapping
                return source.Host == "appassets.example";
            }

            switch (e.ProcessFailedKind)
            {
                case CoreWebView2ProcessFailedKind.BrowserProcessExited:
                    // Once the WebView2 Runtime's browser process has crashed,
                    // the control becomes virtually unusable as the process exit
                    // moves the CoreWebView2 to its Closed state. Most calls will
                    // become invalid as they require a backing browser process.
                    // Remove the control from the visual tree so the framework does
                    // not atempt to redraw it, which would call the invalid methods.
                    RemoveControlFromVisualTree(webView);
                    goto case CoreWebView2ProcessFailedKind.RenderProcessUnresponsive;
                case CoreWebView2ProcessFailedKind.RenderProcessUnresponsive:
                    System.Threading.SynchronizationContext.Current.Post((_) =>
                    {
                        ReinitIfSelectedByUser(e.ProcessFailedKind);
                    }, null);
                    break;
                case CoreWebView2ProcessFailedKind.RenderProcessExited:
                    System.Threading.SynchronizationContext.Current.Post((_) =>
                    {
                        ReloadIfSelectedByUser(e.ProcessFailedKind);
                    }, null);
                    break;
                case CoreWebView2ProcessFailedKind.FrameRenderProcessExited:
                    // A frame-only renderer has exited unexpectedly. Check if reload is needed.
                    // In this sample we only reload if the app's content has been impacted.
                    foreach (CoreWebView2FrameInfo frameInfo in e.FrameInfosForFailedProcess)
                    {
                        if (IsAppContentUri(new System.Uri(frameInfo.Source)))
                        {
                            goto case CoreWebView2ProcessFailedKind.RenderProcessExited;
                        }
                    }
                    break;
                default:
                    // Show the process failure details. Apps can collect info for their logging purposes.
                    StringBuilder messageBuilder = new StringBuilder();
                    messageBuilder.AppendLine($"Process kind: {e.ProcessFailedKind}");
                    messageBuilder.AppendLine($"Reason: {e.Reason}");
                    messageBuilder.AppendLine($"Exit code: {e.ExitCode}");
                    messageBuilder.AppendLine($"Process description: {e.ProcessDescription}");
                    System.Threading.SynchronizationContext.Current.Post((_) =>
                    {
                        MessageBox.Show(messageBuilder.ToString(), "Child process failed", MessageBoxButton.OK);
                    }, null);
                    break;
            }
        }

        double ZoomStep()
        {
            if (webView.ZoomFactor < 1)
            {
                return 0.25;
            }
            else if (webView.ZoomFactor < 2)
            {
                return 0.5;
            }
            else
            {
                return 1;
            }
        }

        void IncreaseZoomCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.ZoomFactor += ZoomStep();
        }

        void DecreaseZoomCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = (webView != null) && (webView.ZoomFactor - ZoomStep() > 0.0);
        }

        void DecreaseZoomCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.ZoomFactor -= ZoomStep();
        }

        void BackgroundColorCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            System.Drawing.Color backgroundColor = System.Drawing.Color.FromName(e.Parameter.ToString());
            webView.DefaultBackgroundColor = backgroundColor;
        }

        async void InjectScriptCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Inject Script",
                description: "Enter some JavaScript to be executed in the context of this page.",
                defaultInput: "window.getComputedStyle(document.body).backgroundColor");
            if (dialog.ShowDialog() == true)
            {
                string scriptResult = await webView.ExecuteScriptAsync(dialog.Input.Text);
                MessageBox.Show(this, scriptResult, "Script Result");
            }
        }

        async void GetCookiesCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            List<CoreWebView2Cookie> cookieList = await webView.CoreWebView2.CookieManager.GetCookiesAsync("https://www.bing.com");
            StringBuilder cookieResult = new StringBuilder(cookieList.Count + " cookie(s) received from https://www.bing.com\n");
            for (int i = 0; i < cookieList.Count; ++i)
            {
                CoreWebView2Cookie cookie = webView.CoreWebView2.CookieManager.CreateCookieWithSystemNetCookie(cookieList[i].ToSystemNetCookie());
                cookieResult.Append($"\n{cookie.Name} {cookie.Value} {(cookie.IsSession ? "[session cookie]" : cookie.Expires.ToString("G"))}");
            }
            MessageBox.Show(this, cookieResult.ToString(), "GetCookiesAsync");
        }

        void AddOrUpdateCookieCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            CoreWebView2Cookie cookie = webView.CoreWebView2.CookieManager.CreateCookie("CookieName", "CookieValue", ".bing.com", "/");
            webView.CoreWebView2.CookieManager.AddOrUpdateCookie(cookie);
        }

        void DeleteAllCookiesCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.CookieManager.DeleteAllCookies();
        }

        void DeleteCookiesCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.CookieManager.DeleteCookiesWithDomainAndPath("CookieName", ".bing.com", "/");
        }

        void SetUserAgentCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "SetUserAgent",
                description: "Enter UserAgent");
            if (dialog.ShowDialog() == true)
            {
                WebViewSettings.UserAgent = dialog.Input.Text;
            }
        }

        void DOMContentLoadedCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.DOMContentLoaded += (object sender, CoreWebView2DOMContentLoadedEventArgs arg) =>
            {
                _ = webView.ExecuteScriptAsync("let " +
                                          "content=document.createElement(\"h2\");content.style.color=" +
                                          "'blue';content.textContent= \"This text was added by the " +
                                          "host app\";document.body.appendChild(content);");
            };
            webView.NavigateToString(@"<!DOCTYPE html><h1>DOMContentLoaded sample page</h1><h2>The content below will be added after DOM content is loaded </h2>");
        }

        void PasswordAutosaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            WebViewSettings.IsPasswordAutosaveEnabled = !WebViewSettings.IsPasswordAutosaveEnabled;
        }
        void GeneralAutofillCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            WebViewSettings.IsGeneralAutofillEnabled = !WebViewSettings.IsGeneralAutofillEnabled;
        }

        void NavigateWithWebResourceRequestCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // Prepare post data as UTF-8 byte array and convert it to stream
            // as required by the application/x-www-form-urlencoded Content-Type
            var dialog = new TextInputDialog(
                title: "NavigateWithWebResourceRequest",
                description: "Specify post data to submit to https://www.w3schools.com/action_page.php.");
            if (dialog.ShowDialog() == true)
            {
                string postDataString = "input=" + dialog.Input.Text;
                UTF8Encoding utfEncoding = new UTF8Encoding();
                byte[] postData = utfEncoding.GetBytes(
                    postDataString);
                MemoryStream postDataStream = new MemoryStream(postDataString.Length);
                postDataStream.Write(postData, 0, postData.Length);
                postDataStream.Seek(0, SeekOrigin.Begin);
                CoreWebView2WebResourceRequest webResourceRequest =
                  WebViewEnvironment.CreateWebResourceRequest(
                    "https://www.w3schools.com/action_page.php",
                    "POST",
                    postDataStream,
                    "Content-Type: application/x-www-form-urlencoded\r\n");
                webView.CoreWebView2.NavigateWithWebResourceRequest(webResourceRequest);
            }
        }


        void PinchZoomCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            WebViewSettings.IsPinchZoomEnabled = !WebViewSettings.IsPinchZoomEnabled;
            MessageBox.Show("Pinch Zoom is" + (WebViewSettings.IsPinchZoomEnabled ? " enabled " : " disabled ") + "after the next navigation.");
        }

        void SwipeNavigationCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // Safeguarding the handler when unsupported runtime is used.
            try
            {
                WebViewSettings.IsSwipeNavigationEnabled = !WebViewSettings.IsSwipeNavigationEnabled;
                MessageBox.Show("Swipe to navigate is" + (WebViewSettings.IsSwipeNavigationEnabled ? " enabled " : " disabled ") + "after the next navigation.");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Toggle Swipe Navigation Failed: " + exception.Message, "Swipe Navigation");
            }
        }
        void NewBrowserVersionCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            foreach (Window window in Application.Current.Windows)
            {
                if (window is MainWindow mainWindow)
                {
                    // Simulate NewBrowserVersionAvailable being raised.
                    mainWindow.Environment_NewBrowserVersionAvailable(null, null);
                }
            }
        }

        void DownloadStartingCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                webView.CoreWebView2.DownloadStarting += delegate (
                  object sender, CoreWebView2DownloadStartingEventArgs args)
                {
            // Developer can obtain a deferral for the event so that the CoreWebView2
            // doesn't examine the properties we set on the event args until
            // after the deferral completes asynchronously.
            CoreWebView2Deferral deferral = args.GetDeferral();

            // We avoid potential reentrancy from running a message loop in the download
            // starting event handler by showing our download dialog later when we
            // complete the deferral asynchronously.
            System.Threading.SynchronizationContext.Current.Post((_) =>
            {
                              using (deferral)
                              {
                          // Hide the default download dialog.
                          args.Handled = true;
                                  var dialog = new TextInputDialog(
                                      title: "Download Starting",
                                      description: "Enter new result file path or select OK to keep default path. Select cancel to cancel the download.",
                                      defaultInput: args.ResultFilePath);
                                  if (dialog.ShowDialog() == true)
                                  {
                                      args.ResultFilePath = dialog.Input.Text;
                                      UpdateProgress(args.DownloadOperation);
                                  }
                                  else
                                  {
                                      args.Cancel = true;
                                  }
                              }
                          }, null);
                };
                webView.CoreWebView2.Navigate("https://demo.smartscreen.msft.net/");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "DownloadStarting Failed: " + exception.Message, "Download Starting");
            }
        }

        // Update download progress
        void UpdateProgress(CoreWebView2DownloadOperation download)
        {
            download.BytesReceivedChanged += delegate (object sender, Object e)
            {
          // Here developer can update download dialog to show progress of a
          // download using `download.BytesReceived` and `download.TotalBytesToReceive`
      };

            download.StateChanged += delegate (object sender, Object e)
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
        }

        // Turn off client certificate selection dialog using ClientCertificateRequested event handler
        // that disables the dialog. This example hides the default client certificate dialog and
        // always chooses the last certificate without prompting the user.
        private bool _isCustomClientCertificateSelection = false;
        void EnableCustomClientCertificateSelection()
        {
            // Safeguarding the handler when unsupported runtime is used.
            try
            {
                if (!_isCustomClientCertificateSelection)
                {
                    webView.CoreWebView2.ClientCertificateRequested += WebView_ClientCertificateRequested;
                }
                else
                {
                    webView.CoreWebView2.ClientCertificateRequested -= WebView_ClientCertificateRequested;
                }
                _isCustomClientCertificateSelection = !_isCustomClientCertificateSelection;

                MessageBox.Show(this,
                    _isCustomClientCertificateSelection ? "Custom client certificate selection has been enabled" : "Custom client certificate selection has been disabled",
                    "Custom client certificate selection");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Custom client certificate selection Failed: " + exception.Message, "Custom client certificate selection");
            }
        }

        void WebView_ClientCertificateRequested(object sender, CoreWebView2ClientCertificateRequestedEventArgs e)
        {
            IReadOnlyList<CoreWebView2ClientCertificate> certificateList = e.MutuallyTrustedCertificates;
            if (certificateList.Count() > 0)
            {
                // There is no significance to the order, picking a certificate arbitrarily.
                e.SelectedCertificate = certificateList.LastOrDefault();
                // Continue with the selected certificate to respond to the server.
                e.Handled = true;
            }
            else
            {
                // Continue without a certificate to respond to the server if certificate list is empty.
                e.Handled = true;
            }
        }

        // This example hides the default client certificate dialog and shows a custom dialog instead.
        // The dialog box displays mutually trusted certificates list and allows the user to select a certificate.
        // Selecting `OK` will continue the request with a certificate.
        // Selecting `CANCEL` will continue the request without a certificate
        private bool _isCustomClientCertificateSelectionDialog = false;
        void DeferredCustomClientCertificateSelectionDialog()
        {
            // Safeguarding the handler when unsupported runtime is used.
            try
            {
                if (!_isCustomClientCertificateSelectionDialog)
                {
                    webView.CoreWebView2.ClientCertificateRequested += delegate (
                        object sender, CoreWebView2ClientCertificateRequestedEventArgs args)
                    {
              // Developer can obtain a deferral for the event so that the WebView2
              // doesn't examine the properties we set on the event args until
              // after the deferral completes asynchronously.
              CoreWebView2Deferral deferral = args.GetDeferral();

                        System.Threading.SynchronizationContext.Current.Post((_) =>
                        {
                                    using (deferral)
                                    {
                                        IReadOnlyList<CoreWebView2ClientCertificate> certificateList = args.MutuallyTrustedCertificates;
                                        if (certificateList.Count() > 0)
                                        {
                                  // Display custom dialog box for the client certificate selection.
                                  var dialog = new ClientCertificateSelectionDialog(
                                                              title: "Select a Certificate for authentication",
                                                              host: args.Host,
                                                              port: args.Port,
                                                              client_cert_list: certificateList);
                                            if (dialog.ShowDialog() == true)
                                            {
                                      // Continue with the selected certificate to respond to the server if `OK` is selected.
                                      args.SelectedCertificate = (CoreWebView2ClientCertificate)dialog.CertificateDataBinding.SelectedItem;
                                            }
                                  // Continue without a certificate to respond to the server if `CANCEL` is selected.
                                  args.Handled = true;
                                        }
                                        else
                                        {
                                  // Continue without a certificate to respond to the server if certificate list is empty.
                                  args.Handled = true;
                                        }
                                    }

                                }, null);
                    };
                    _isCustomClientCertificateSelectionDialog = true;
                    MessageBox.Show("Custom Client Certificate selection dialog will be used next when WebView2 is making a " +
                        "request to an HTTP server that needs a client certificate.", "Client certificate selection");
                }
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Custom client certificate selection dialog Failed: " + exception.Message, "Client certificate selection");
            }
        }

        void GoToPageCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = webView != null && !_isNavigating;
        }

        async void GoToPageCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            await webView.EnsureCoreWebView2Async();

            var rawUrl = (string)e.Parameter;
            Uri uri = null;

            if (Uri.IsWellFormedUriString(rawUrl, UriKind.Absolute))
            {
                uri = new Uri(rawUrl);
            }
            else if (!rawUrl.Contains(" ") && rawUrl.Contains("."))
            {
                // An invalid URI contains a dot and no spaces, try tacking http:// on the front.
                uri = new Uri("http://" + rawUrl);
            }
            else
            {
                // Otherwise treat it as a web search.
                uri = new Uri("https://bing.com/search?q=" +
                    String.Join("+", Uri.EscapeDataString(rawUrl).Split(new string[] { "%20" }, StringSplitOptions.RemoveEmptyEntries)));
            }

            // Setting webView.Source will not trigger a navigation if the Source is the same
            // as the previous Source.  CoreWebView.Navigate() will always trigger a navigation.
            webView.CoreWebView2.Navigate(uri.ToString());
        }

        async void SuspendCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                bool isSuccessful = await webView.CoreWebView2.TrySuspendAsync();
                MessageBox.Show(this,
                    (isSuccessful) ? "TrySuspendAsync succeeded" : "TrySuspendAsync failed",
                    "TrySuspendAsync");
            }
            catch (System.Runtime.InteropServices.COMException exception)
            {
                MessageBox.Show(this, "TrySuspendAsync failed:" + exception.Message, "TrySuspendAsync");
            }
        }
        void ResumeCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                webView.CoreWebView2.Resume();
                MessageBox.Show(this, "Resume Succeeded", "Resume");
            }
            catch (System.Runtime.InteropServices.COMException exception)
            {
                MessageBox.Show(this, "Resume failed:" + exception.Message, "Resume");
            }
        }

        async void CheckUpdateCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                CoreWebView2UpdateRuntimeResult result = await webView.CoreWebView2.Environment.UpdateRuntimeAsync();
                string update_result = "status: " + result.Status + ", extended error:" + result.ExtendedError;
                MessageBox.Show(this, update_result, "UpdateRuntimeAsync result");
            }
            catch (System.Runtime.InteropServices.COMException exception)
            {
                MessageBox.Show(this, "UpdateRuntimeAsync failed:" + exception.Message, "UpdateRuntimeAsync");
            }
        }

        bool _allowWebViewShortcutKeys = true;
        bool _allowShortcutsEventRegistered = false;
        public bool AllowWebViewShortcutKeys
        {
            get => _allowWebViewShortcutKeys;
            set
            {
                _allowWebViewShortcutKeys = value;
                if (webView.CoreWebView2 != null)
                {
                    WebViewSettings.AreBrowserAcceleratorKeysEnabled = value;
                }
                else if (!_allowShortcutsEventRegistered)
                {
                    _allowShortcutsEventRegistered = true;
                    webView.CoreWebView2InitializationCompleted += (sender, e) =>
                    {
                        if (e.IsSuccess)
                        {
                            WebViewSettings.AreBrowserAcceleratorKeysEnabled = _allowWebViewShortcutKeys;
                        }
                    };
                }
            }
        }

        void WebView_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.IsRepeat) return;
            bool ctrl = e.KeyboardDevice.IsKeyDown(Key.LeftCtrl) || e.KeyboardDevice.IsKeyDown(Key.RightCtrl);
            bool alt = e.KeyboardDevice.IsKeyDown(Key.LeftAlt) || e.KeyboardDevice.IsKeyDown(Key.RightAlt);
            bool shift = e.KeyboardDevice.IsKeyDown(Key.LeftShift) || e.KeyboardDevice.IsKeyDown(Key.RightShift);
            if (e.Key == Key.N && ctrl && !alt && !shift)
            {
                new MainWindow().Show();
                e.Handled = true;
            }
            else if (e.Key == Key.W && ctrl && !alt && !shift)
            {
                Close();
                e.Handled = true;
            }
        }

        void WebView_NavigationStarting(object sender, CoreWebView2NavigationStartingEventArgs e)
        {
            _isNavigating = true;
            RequeryCommands();
        }

        void WebView_NavigationCompleted(object sender, CoreWebView2NavigationCompletedEventArgs e)
        {
            _isNavigating = false;
            RequeryCommands();
        }

        private bool shouldAttachEnvironmentEventHandlers = true;

        void WebView_CoreWebView2InitializationCompleted(object sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            if (e.IsSuccess)
            {
                webView.CoreWebView2.ProcessFailed += WebView_ProcessFailed;

                // The CoreWebView2Environment instance is reused when re-assigning CoreWebView2CreationProperties
                // to the replacement control. We don't need to re-attach the event handlers unless the environment
                // instance has changed.
                if (shouldAttachEnvironmentEventHandlers)
                {
                    try
                    {
                        WebViewEnvironment.BrowserProcessExited += Environment_BrowserProcessExited;
                        WebViewEnvironment.NewBrowserVersionAvailable += Environment_NewBrowserVersionAvailable;
                    }
                    catch (NotImplementedException)
                    {
                        newVersionMenuItem.IsEnabled = false;
                    }
                    shouldAttachEnvironmentEventHandlers = false;
                }
                return;
            }

            MessageBox.Show($"WebView2 creation failed with exception = {e.InitializationException}");
        }

        private bool shouldAttemptReinitOnBrowserExit = false;

        void Environment_BrowserProcessExited(object sender, CoreWebView2BrowserProcessExitedEventArgs e)
        {
            // Let ProcessFailed handler take care of process failure.
            if (e.BrowserProcessExitKind == CoreWebView2BrowserProcessExitKind.Failed)
            {
                return;
            }
            if (shouldAttemptReinitOnBrowserExit)
            {
                _webViewEnvironment = null;
                webView = GetReplacementControl(true);
                AttachControlToVisualTree(webView);
                shouldAttemptReinitOnBrowserExit = false;
            }
        }

        // A new version of the WebView2 Runtime is available, our handler gets called.
        // We close our WebView and set a handler to reinitialize it once the WebView2
        // Runtime collection of processes are gone, so we get the new version of the
        // WebView2 Runtime.
        void Environment_NewBrowserVersionAvailable(object sender, object e)
        {
            if (((App)Application.Current).newRuntimeEventHandled)
            {
                return;
            }

            ((App)Application.Current).newRuntimeEventHandled = true;
            System.Threading.SynchronizationContext.Current.Post((_) =>
            {
                UpdateIfSelectedByUser();
            }, null);
        }

        void UpdateIfSelectedByUser()
        {
            // New browser version available, ask user to close everything and re-init.
            StringBuilder messageBuilder = new StringBuilder(256);
            messageBuilder.Append("We detected there is a new version of the WebView2 Runtime installed. ");
            messageBuilder.Append("Do you want to switch to it now? This will re-create the WebView.");
            var selection = MessageBox.Show(this, messageBuilder.ToString(), "New WebView2 Runtime detected", MessageBoxButton.YesNo);
            if (selection == MessageBoxResult.Yes)
            {
                // If this or any other application creates additional WebViews from the same
                // environment configuration, all those WebViews need to be closed before
                // the browser process will exit. This sample creates a single WebView per
                // MainWindow, we let each MainWindow prepare to recreate and close its WebView.
                CloseAppWebViewsForUpdate();
            }
            ((App)Application.Current).newRuntimeEventHandled = false;
        }

        private void CloseAppWebViewsForUpdate()
        {
            foreach (Window window in Application.Current.Windows)
            {
                if (window is MainWindow mainWindow)
                {
                    mainWindow.CloseWebViewForUpdate();
                }
            }
        }

        private void CloseWebViewForUpdate()
        {
            // We dispose of the control so the internal WebView objects are released
            // and the associated browser process exits.
            shouldAttemptReinitOnBrowserExit = true;
            RemoveControlFromVisualTree(webView);
            webView.Dispose();
        }

        private static void OnShowNextWebResponseChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            MainWindow window = (MainWindow)d;
            if ((bool)e.NewValue)
            {
                window.webView.CoreWebView2.WebResourceResponseReceived += window.CoreWebView2_WebResourceResponseReceived;
            }
            else
            {
                window.webView.CoreWebView2.WebResourceResponseReceived -= window.CoreWebView2_WebResourceResponseReceived;
            }
        }

        public static readonly DependencyProperty ShowNextWebResponseProperty = DependencyProperty.Register(
            nameof(ShowNextWebResponse),
            typeof(Boolean),
            typeof(MainWindow),
            new PropertyMetadata(false, OnShowNextWebResponseChanged));

        public bool ShowNextWebResponse
        {
            get => (bool)this.GetValue(ShowNextWebResponseProperty);
            set => this.SetValue(ShowNextWebResponseProperty, value);
        }

        async void CoreWebView2_WebResourceResponseReceived(object sender, CoreWebView2WebResourceResponseReceivedEventArgs e)
        {
            ShowNextWebResponse = false;

            CoreWebView2WebResourceRequest request = e.Request;
            CoreWebView2WebResourceResponseView response = e.Response;

            string caption = "Web Resource Response Received";
            // Start with capacity 64 for minimum message size
            StringBuilder messageBuilder = new StringBuilder(64);
            string HttpMessageContentToString(System.IO.Stream content) => content == null ? "[null]" : "[data]";
            void AppendHeaders(IEnumerable headers)
            {
                foreach (var header in headers)
                {
                    messageBuilder.AppendLine($"  {header}");
                }
            }

            // Request
            messageBuilder.AppendLine("Request");
            messageBuilder.AppendLine($"URI: {request.Uri}");
            messageBuilder.AppendLine($"Method: {request.Method}");
            messageBuilder.AppendLine("Headers:");
            AppendHeaders(request.Headers);
            messageBuilder.AppendLine($"Content: {HttpMessageContentToString(request.Content)}");
            messageBuilder.AppendLine();

            // Response
            messageBuilder.AppendLine("Response");
            messageBuilder.AppendLine($"Status: {response.StatusCode}");
            messageBuilder.AppendLine($"Reason: {response.ReasonPhrase}");
            messageBuilder.AppendLine("Headers:");
            AppendHeaders(response.Headers);
            try
            {
                Stream content = await response.GetContentAsync();
                messageBuilder.AppendLine($"Content: {HttpMessageContentToString(content)}");
            }
            catch (System.Runtime.InteropServices.COMException)
            {
                messageBuilder.AppendLine($"Content: [failed to load]");
            }

            MessageBox.Show(messageBuilder.ToString(), caption);
        }

        void RequeryCommands()
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
