﻿// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
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
using System.Windows.Threading;
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
        public static RoutedCommand InjectScriptIFrameCommand = new RoutedCommand();
        public static RoutedCommand InjectScriptWithResultCommand = new RoutedCommand();
        public static RoutedCommand PrintToPdfCommand = new RoutedCommand();
        public static RoutedCommand NavigateWithWebResourceRequestCommand = new RoutedCommand();
        public static RoutedCommand DOMContentLoadedCommand = new RoutedCommand();
        public static RoutedCommand WebMessagesCommand = new RoutedCommand();
        public static RoutedCommand GetCookiesCommand = new RoutedCommand();
        public static RoutedCommand SuspendCommand = new RoutedCommand();
        public static RoutedCommand ResumeCommand = new RoutedCommand();
        public static RoutedCommand CheckUpdateCommand = new RoutedCommand();
        public static RoutedCommand NewBrowserVersionCommand = new RoutedCommand();
        public static RoutedCommand PdfToolbarSaveCommand = new RoutedCommand();
        public static RoutedCommand SmartScreenEnabledCommand = new RoutedCommand();
        public static RoutedCommand AuthenticationCommand = new RoutedCommand();
        public static RoutedCommand FaviconChangedCommand = new RoutedCommand();
        public static RoutedCommand ClearBrowsingDataCommand = new RoutedCommand();
        public static RoutedCommand SetDefaultDownloadPathCommand = new RoutedCommand();
        public static RoutedCommand CreateDownloadsButtonCommand = new RoutedCommand();
        public static RoutedCommand CustomClientCertificateSelectionCommand = new RoutedCommand();
        public static RoutedCommand CustomContextMenuCommand = new RoutedCommand();
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
        public static RoutedCommand ToggleMuteStateCommand = new RoutedCommand();
        public static RoutedCommand AllowExternalDropCommand = new RoutedCommand();
        public static RoutedCommand PerfInfoCommand = new RoutedCommand();
        public static RoutedCommand CustomServerCertificateSupportCommand = new RoutedCommand();
        public static RoutedCommand ClearServerCertificateErrorActionsCommand = new RoutedCommand();
        public static RoutedCommand NewWindowWithOptionsCommand = new RoutedCommand();
        public static RoutedCommand CreateNewThreadCommand = new RoutedCommand();
        public static RoutedCommand PrintDialogCommand = new RoutedCommand();
        public static RoutedCommand PrintToDefaultPrinterCommand = new RoutedCommand();
        public static RoutedCommand PrintToPrinterCommand = new RoutedCommand();
        public static RoutedCommand PrintToPdfStreamCommand = new RoutedCommand();
        // Commands(V2)
        public static RoutedCommand AboutCommand = new RoutedCommand();



        public static RoutedCommand CrashBrowserProcessCommand = new RoutedCommand();
        public static RoutedCommand CrashRenderProcessCommand = new RoutedCommand();


        public static RoutedCommand GetDocumentTitleCommand = new RoutedCommand();
        public static RoutedCommand GetUserDataFolderCommand = new RoutedCommand();
        public static RoutedCommand SharedBufferRequestedCommand = new RoutedCommand();
        public static RoutedCommand PostMessageStringCommand = new RoutedCommand();
        public static RoutedCommand PostMessageJSONCommand = new RoutedCommand();


        public static RoutedCommand HostObjectsAllowedCommand = new RoutedCommand();
        public static RoutedCommand BrowserAcceleratorKeyEnabledCommand = new RoutedCommand();

        public static RoutedCommand AddInitializeScriptCommand = new RoutedCommand();
        public static RoutedCommand RemoveInitializeScriptCommand = new RoutedCommand();

        public static RoutedCommand CallCdpMethodCommand = new RoutedCommand();
        public static RoutedCommand OpenDevToolsCommand = new RoutedCommand();
        public static RoutedCommand OpenTaskManagerCommand = new RoutedCommand();

        bool _isNavigating = false;

        // for add/remove initialize script
        string m_lastInitializeScriptId;

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
        CoreWebView2Profile _webViewProfile;
        CoreWebView2Profile WebViewProfile
        {
            get
            {
                if (_webViewProfile == null && webView?.CoreWebView2 != null)
                {
                    // <Profile>
                    _webViewProfile = webView.CoreWebView2.Profile;
                    // </Profile>
                }
                return _webViewProfile;
            }
        }

        List<CoreWebView2Frame> _webViewFrames = new List<CoreWebView2Frame>();
        IReadOnlyList<CoreWebView2ProcessInfo> _processList = new List<CoreWebView2ProcessInfo>();

        IDictionary<(string, CoreWebView2PermissionKind, bool), bool> _cachedPermissions = 
            new Dictionary<(string, CoreWebView2PermissionKind, bool), bool>();

        public CoreWebView2CreationProperties CreationProperties { get; set; } = null;

        public MainWindow()
        {
            DataContext = this;
            InitializeComponent();
            AttachControlEventHandlers(webView);
            // Set background transparent
            //webView.DefaultBackgroundColor = System.Drawing.Color.Transparent;
        }

        public MainWindow(CoreWebView2CreationProperties creationProperties = null)
        {
            this.CreationProperties = creationProperties;
            DataContext = this;
            InitializeComponent();
            AttachControlEventHandlers(webView);
            // Set background transparent
            //webView.DefaultBackgroundColor = System.Drawing.Color.Transparent;
        }

        void AttachControlEventHandlers(WebView2 control)
        {
            // <NavigationStarting>
            control.NavigationStarting += WebView_NavigationStarting;
            // </NavigationStarting>
            // <NavigationCompleted>
            control.NavigationCompleted += WebView_NavigationCompleted;
            // </NavigationCompleted>
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

        void AssertCondition(bool condition, string message)
        {
            if (condition)
                return;
            MessageBox.Show(message, "Assertion Failed");
        }

        void NewCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
            new MainWindow().Show();
        }

        void CloseCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
            if (_isPrintToPdfInProgress)
            {
                var selection = MessageBox.Show(
                    "Print to PDF in progress. Continue closing?",
                    "Print to PDF", MessageBoxButton.YesNo);
                if (selection == MessageBoxResult.No)
                {
                    return;
                }
            }
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

        void CustomServerCertificateSupportCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            ToggleCustomServerCertificateSupport();
        }

        void ClearServerCertificateErrorActionsCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            ClearServerCertificateErrorActions();
        }

        void PrintDialogCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            ShowPrintUI(target, e);
        }

        void PrintToDefaultPrinterCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            PrintToDefaultPrinter();
        }

        void PrintToPrinterCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            PrintToPrinter();
        }

        void PrintToPdfStreamCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            PrintToPdfStream();
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
            void ReinitIfSelectedByUser(string caption, string message)
            {
                this.Dispatcher.InvokeAsync(() =>
                {
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
                        // Set background transparent
                        webView.DefaultBackgroundColor = System.Drawing.Color.Transparent;
                    }
                });
            }

            void ReloadIfSelectedByUser(string caption, string message)
            {
                this.Dispatcher.InvokeAsync(() =>
                {
                    var selection = MessageBox.Show(message, caption, MessageBoxButton.YesNo);
                    if (selection == MessageBoxResult.Yes)
                    {
                        webView.Reload();
                        // Set background transparent
                        webView.DefaultBackgroundColor = System.Drawing.Color.Transparent;
                    }
                });
            }

            bool IsAppContentUri(Uri source)
            {
                // Sample virtual host name for the app's content.
                // See CoreWebView2.SetVirtualHostNameToFolderMapping: https://docs.microsoft.com/dotnet/api/microsoft.web.webview2.core.corewebview2.setvirtualhostnametofoldermapping
                return source.Host == "appassets.example";
            }

            if (e.ProcessFailedKind == CoreWebView2ProcessFailedKind.FrameRenderProcessExited)
            {
                // A frame-only renderer has exited unexpectedly. Check if reload is needed.
                // In this sample we only reload if the app's content has been impacted.
                foreach (CoreWebView2FrameInfo frameInfo in e.FrameInfosForFailedProcess)
                {
                    if (IsAppContentUri(new System.Uri(frameInfo.Source)))
                    {
                        System.Threading.SynchronizationContext.Current.Post((_) =>
                        {
                            ReloadIfSelectedByUser("App content frame unresponsive",
                                "Browser render process for app frame exited unexpectedly. Reload page?");
                        }, null);
                    }
                }

                return;
            }

            // Show the process failure details. Apps can collect info for their logging purposes.
            this.Dispatcher.InvokeAsync(() =>
            {
                StringBuilder messageBuilder = new StringBuilder();
                messageBuilder.AppendLine($"Process kind: {e.ProcessFailedKind}");
                messageBuilder.AppendLine($"Reason: {e.Reason}");
                messageBuilder.AppendLine($"Exit code: {e.ExitCode}");
                messageBuilder.AppendLine($"Process description: {e.ProcessDescription}");
                MessageBox.Show(messageBuilder.ToString(), "Child process failed", MessageBoxButton.OK);
            });

            if (e.ProcessFailedKind == CoreWebView2ProcessFailedKind.BrowserProcessExited)
            {
                ReinitIfSelectedByUser("Browser process exited",
                    "Browser process exited unexpectedly. Recreate webview?");
            }
            else if (e.ProcessFailedKind == CoreWebView2ProcessFailedKind.RenderProcessUnresponsive)
            {
                ReinitIfSelectedByUser("Web page unresponsive",
                    "Browser render process has stopped responding. Recreate webview?");
            }
            else if (e.ProcessFailedKind == CoreWebView2ProcessFailedKind.RenderProcessExited)
            {
                ReloadIfSelectedByUser("Web page unresponsive",
                    "Browser render process exited unexpectedly. Reload page?");
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
            // <DefaultBackgroundColor>
            System.Drawing.Color backgroundColor = System.Drawing.Color.FromName(e.Parameter.ToString());
            webView.DefaultBackgroundColor = backgroundColor;
            // </DefaultBackgroundColor>
        }

        async void InjectScriptCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ExecuteScript>
            var dialog = new TextInputDialog(
                title: "Inject Script",
                description: "Enter some JavaScript to be executed in the context of this page.",
                defaultInput: "window.getComputedStyle(document.body).backgroundColor");
            if (dialog.ShowDialog() == true)
            {
                string scriptResult = await webView.ExecuteScriptAsync(dialog.Input.Text);
                MessageBox.Show(this, scriptResult, "Script Result");
            }
            // </ExecuteScript>
        }

        async void InjectScriptIFrameCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ExecuteScriptFrame>
            string iframesData = WebViewFrames_ToString();
            string iframesInfo = "Enter iframe to run the JavaScript code in.\r\nAvailable iframes: " + iframesData;
            var dialogIFrames = new TextInputDialog(
                title: "Inject Script Into IFrame",
                description: iframesInfo,
                defaultInput: "0");
            if (dialogIFrames.ShowDialog() == true)
            {
                int iframeNumber = -1;
                try
                {
                    iframeNumber = Int32.Parse(dialogIFrames.Input.Text);
                }
                catch (FormatException)
                {
                    Console.WriteLine("Can not convert " + dialogIFrames.Input.Text + " to int");
                }
                if (iframeNumber >= 0 && iframeNumber < _webViewFrames.Count)
                {
                    var dialog = new TextInputDialog(
                        title: "Inject Script",
                        description: "Enter some JavaScript to be executed in the context of iframe " + dialogIFrames.Input.Text,
                        defaultInput: "window.getComputedStyle(document.body).backgroundColor");
                    if (dialog.ShowDialog() == true)
                    {
                        string scriptResult = await _webViewFrames[iframeNumber].ExecuteScriptAsync(dialog.Input.Text);
                        MessageBox.Show(this, scriptResult, "Script Result");
                    }
                }
            }
            // </ExecuteScriptFrame>
        }

        private bool _isPrintToPdfInProgress = false;
        async void PrintToPdfCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            if (_isPrintToPdfInProgress)
            {
                MessageBox.Show(this, "Print to PDF in progress", "Print To PDF");
                return;
            }
            try
            {
                // <PrintToPdf>
                CoreWebView2PrintSettings printSettings = null;
                string orientationString = e.Parameter.ToString();
                if (orientationString == "Landscape")
                {
                    printSettings = WebViewEnvironment.CreatePrintSettings();
                    printSettings.Orientation =
                        CoreWebView2PrintOrientation.Landscape;
                }

                Microsoft.Win32.SaveFileDialog saveFileDialog =
                    new Microsoft.Win32.SaveFileDialog();
                saveFileDialog.InitialDirectory = "C:\\";
                saveFileDialog.Filter = "Pdf Files|*.pdf";
                Nullable<bool> result = saveFileDialog.ShowDialog();
                if (result == true) {
                    _isPrintToPdfInProgress = true;
                    bool isSuccessful = await webView.CoreWebView2.PrintToPdfAsync(
                        saveFileDialog.FileName, printSettings);
                    _isPrintToPdfInProgress = false;
                    string message = (isSuccessful) ?
                        "Print to PDF succeeded" : "Print to PDF failed";
                    MessageBox.Show(this, message, "Print To PDF Completed");
                }
                // </PrintToPdf>
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Print to PDF Failed: " + exception.Message,
                   "Print to PDF");
            }
        }

        // Shows the user a print dialog. If `printDialogKind` is browser print preview,
        // opens a browser print preview dialog, CoreWebView2PrintDialogKind.System opens a system print dialog.
        void ShowPrintUI(object target, ExecutedRoutedEventArgs e)
        {
            string printDialog = e.Parameter.ToString();
            if (printDialog == "Browser")
            {
                // Opens the browser print preview dialog.
                webView.CoreWebView2.ShowPrintUI();
            }
            else
            {
                // Opens the system print dialog.
                webView.CoreWebView2.ShowPrintUI(CoreWebView2PrintDialogKind.System);
            }
        }

        // This example prints the current web page without a print dialog to default printer.
        async void PrintToDefaultPrinter ()
        {
            string title = webView.CoreWebView2.DocumentTitle;

            try
            {
                // Passing null for `PrintSettings` results in default print settings used.
                // Prints current web page with the default page and printer settings.
                CoreWebView2PrintStatus printStatus = await webView.CoreWebView2.PrintAsync(null);

                if (printStatus == CoreWebView2PrintStatus.Succeeded)
                {
                    MessageBox.Show(this, "Printing " + title + " document to printer is succeeded", "Print");
                }
                else if (printStatus == CoreWebView2PrintStatus.PrinterUnavailable)
                {
                    MessageBox.Show(this, "Printer is not available, offline or error state", "Print");
                }
                else 
                {
                    MessageBox.Show(this, "Printing " + title + " document to printer is failed",
                        "Print");
                }
            }
            catch (Exception)
            {
               MessageBox.Show(this, "Printing " + title + " document already in progress",
                        "Print");
            }
        }

        // <PrintToPrinter>
        // Function to get printer name by displaying printer text input dialog to the user.
        // User has to specify the desired printer name by querying the installed printers list on the
        // OS to print the web page.
        // You may also choose to display printers list to the user and return user selected printer.
        string GetPrinterName()
        {
            string printerName = "";

            var dialog = new TextInputDialog(
                        title: "Printer Name",
                        description: "Specify a printer name from the installed printers list on the OS.",
                        defaultInput: "");
            if (dialog.ShowDialog() == true)
            {
                printerName = dialog.Input.Text;
            }
            return printerName;

            // or
            //
            // Use GetPrintQueues() of LocalPrintServer from System.Printing to get list of locally installed printers.
            // Display the printer list to the user and get the desired printer to print.
            // Return the user selected printer name.
        }

        // Function to get print settings for the selected printer.
        // You may also choose get the capabilties from the native printer API, display to the user to get
        // the print settings for the current web page and for the selected printer.
        CoreWebView2PrintSettings GetSelectedPrinterPrintSettings(string printerName)
        {
            CoreWebView2PrintSettings printSettings = null;
            printSettings = WebViewEnvironment.CreatePrintSettings();
            printSettings.ShouldPrintBackgrounds = true;
            printSettings.ShouldPrintHeaderAndFooter = true;

            return printSettings;

            // or
            //
            // Get PrintQueue for the selected printer and use GetPrintCapabilities() of PrintQueue from System.Printing
            // to get the capabilities of the selected printer.
            // Display the printer capabilities to the user along with the page settings.
            // Return the user selected settings.
        }

        // This example prints the current web page to the specified printer with the settings.
        async void PrintToPrinter()
        {
            string printerName = GetPrinterName();
            CoreWebView2PrintSettings printSettings = GetSelectedPrinterPrintSettings(printerName);
            string title = webView.CoreWebView2.DocumentTitle;
            try
            {
                CoreWebView2PrintStatus printStatus = await webView.CoreWebView2.PrintAsync(printSettings);

                if (printStatus == CoreWebView2PrintStatus.Succeeded)
                {
                    MessageBox.Show(this, "Printing " + title + " document to printer is succeeded", "Print to printer");
                }
                else if (printStatus == CoreWebView2PrintStatus.PrinterUnavailable)
                {
                    MessageBox.Show(this, "Selected printer is not found, not available, offline or error state", "Print to printer");
                }
                else
                {
                    MessageBox.Show(this, "Printing " + title + " document to printer is failed",
                        "Print");
                }
            }
            catch(ArgumentException)
            {
                MessageBox.Show(this, "Invalid settings provided for the specified printer",
                    "Print");
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Printing " + title + " document already in progress",
                        "Print");

            }
        }
        // </PrintToPrinter>

        // <PrintToPdfStream>
        // This example prints the Pdf data of the current web page to a stream.
        async void PrintToPdfStream()
        {
            try
            {
                string title = webView.CoreWebView2.DocumentTitle;
             
                // Passing null for `PrintSettings` results in default print settings used.
                System.IO.Stream stream = await webView.CoreWebView2.PrintToPdfStreamAsync(null);
                DisplayPdfDataInPrintDialog(stream);
                MessageBox.Show(this, "Printing" + title + " document to PDF Stream " + ((stream != null) ? "succeeded" : "failed"), "Print To PDF Stream");
            }
            catch (Exception exception)
            {
                MessageBox.Show(this, "Printing to PDF Stream failed: " + exception.Message,
                   "Print to PDF Stream");
            }
        }

        // Function to display current page pdf data in a custom print preview dialog.
        void DisplayPdfDataInPrintDialog(Stream pdfData)
        {
            // You can display the printable pdf data in a custom print preview dialog to the end user.
        }
        // </PrintToPdfStream>

        async void GetCookiesCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <GetCookies>
            List<CoreWebView2Cookie> cookieList = await webView.CoreWebView2.CookieManager.GetCookiesAsync("https://www.bing.com");
            StringBuilder cookieResult = new StringBuilder(cookieList.Count + " cookie(s) received from https://www.bing.com\n");
            for (int i = 0; i < cookieList.Count; ++i)
            {
                CoreWebView2Cookie cookie = webView.CoreWebView2.CookieManager.CreateCookieWithSystemNetCookie(cookieList[i].ToSystemNetCookie());
                cookieResult.Append($"\n{cookie.Name} {cookie.Value} {(cookie.IsSession ? "[session cookie]" : cookie.Expires.ToString("G"))}");
            }
            MessageBox.Show(this, cookieResult.ToString(), "GetCookiesAsync");
            // </GetCookies>
        }

        void AddOrUpdateCookieCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <AddOrUpdateCookie>
            CoreWebView2Cookie cookie = webView.CoreWebView2.CookieManager.CreateCookie("CookieName", "CookieValue", ".bing.com", "/");
            webView.CoreWebView2.CookieManager.AddOrUpdateCookie(cookie);
            // </AddOrUpdateCookie>
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
                // <SetUserAgent>
                WebViewSettings.UserAgent = dialog.Input.Text;
                // </SetUserAgent>
            }
        }

        void WebMessagesCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.WebMessageReceived += WebView_WebMessageReceived;
            webView.CoreWebView2.FrameCreated += WebView_FrameCreatedWebMessages;
            webView.CoreWebView2.SetVirtualHostNameToFolderMapping(
                "appassets.example", "assets", CoreWebView2HostResourceAccessKind.DenyCors);
            webView.Source = new Uri("https://appassets.example/webMessages.html");
        }


        void HandleWebMessage(CoreWebView2WebMessageReceivedEventArgs args)
        {
            try
            {
                if (args.Source != "https://appassets.example/webMessages.html")
                {
                    // Throw exception from untrusted sources.
                    throw new Exception();
                }

                string message = args.TryGetWebMessageAsString();

                if (message.Contains("SetTitleText"))
                {
                    int msgLength = "SetTitleText".Length;
                    this.Title = message.Substring(msgLength);

                }

                else if (message == "GetWindowBounds")
                {
                    string reply = "{\"WindowBounds\":\"Left:" + 0 +
                                   "\\nTop:" + 0 +
                                   "\\nRight:" + webView.ActualWidth +
                                   "\\nBottom:" + webView.ActualHeight +
                                   "\"}";

                    webView.CoreWebView2.PostWebMessageAsJson(reply);
                }
                else
                {
                    // Ignore unrecognized messages, but log them
                    // since it suggests a mismatch between the web content and the host.
                    Debug.WriteLine($"Unexpected message received: {message}");
                }
            }
            catch (Exception e)
            {
                MessageBox.Show($"Unexpected message received: {e.Message}");
            }
        }

        void WebView_WebMessageReceived(object sender, CoreWebView2WebMessageReceivedEventArgs args)
        {
            HandleWebMessage(args);
        }

        // <WebMessageReceivedIFrame>
        void WebView_FrameCreatedWebMessages(object sender, CoreWebView2FrameCreatedEventArgs args)
        {
            args.Frame.WebMessageReceived += (WebMessageReceivedSender, WebMessageReceivedArgs) =>
            {
                HandleWebMessage(WebMessageReceivedArgs);
            };
        }
        // </WebMessageReceivedIFrame>


        // <DOMContentLoaded>
        void DOMContentLoadedCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.DOMContentLoaded += WebView_DOMContentLoaded;
            webView.CoreWebView2.FrameCreated += WebView_FrameCreatedDOMContentLoaded;
            webView.NavigateToString(@"<!DOCTYPE html>" +
                                      "<h1>DOMContentLoaded sample page</h1>" +
                                      "<h2>The content to the iframe and below will be added after DOM content is loaded </h2>" +
                                      "<iframe style='height: 200px; width: 100%;'/>");
            webView.CoreWebView2.NavigationCompleted += (sender, args) =>
            {
                webView.CoreWebView2.DOMContentLoaded -= WebView_DOMContentLoaded;
                webView.CoreWebView2.FrameCreated -= WebView_FrameCreatedDOMContentLoaded;
            };
        }

        void WebView_DOMContentLoaded(object sender, CoreWebView2DOMContentLoadedEventArgs arg)
        {
            _ = webView.ExecuteScriptAsync(
                    "let content = document.createElement(\"h2\");" +
                    "content.style.color = 'blue';" +
                    "content.textContent = \"This text was added by the host app\";" +
                    "document.body.appendChild(content);");
        }
        // </DOMContentLoaded>

        void WebView_FrameCreatedDOMContentLoaded(object sender, CoreWebView2FrameCreatedEventArgs args)
        {
            args.Frame.DOMContentLoaded += (frameSender, DOMContentLoadedArgs) =>
            {
                args.Frame.ExecuteScriptAsync(
                    "let content = document.createElement(\"h2\");" +
                    "content.style.color = 'blue';" +
                    "content.textContent = \"This text was added to the iframe by the host app\";" +
                    "document.body.appendChild(content);");
            };
        }

        void PasswordAutosaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <PasswordAutosaveEnabled>
            WebViewSettings.IsPasswordAutosaveEnabled = !WebViewSettings.IsPasswordAutosaveEnabled;
            // </PasswordAutosaveEnabled>
        }
        void GeneralAutofillCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <GeneralAutofillEnabled>
            WebViewSettings.IsGeneralAutofillEnabled = !WebViewSettings.IsGeneralAutofillEnabled;
            // </GeneralAutofillEnabled>
        }

        void NavigateWithWebResourceRequestCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <NavigateWithWebResourceRequest>
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
            // </NavigateWithWebResourceRequest>
        }

        private bool _isCustomContextMenu = false;

        void CustomContextMenuCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (!_isCustomContextMenu)
                {
                    webView.CoreWebView2.ContextMenuRequested +=
                        WebView_ContextMenuRequested;
                }
                else
                {
                    webView.CoreWebView2.ContextMenuRequested -=
                        WebView_ContextMenuRequested;
                }
                _isCustomContextMenu = !_isCustomContextMenu;
                MessageBox.Show(this,
                                _isCustomContextMenu
                                    ? "Custom context menus have been enabled"
                                    : "Custom context menus have been disabled",
                                "Custom context menus");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Custom context menu Failed: " + exception.Message,
                                "Custom context menus");
            }
        }

        private CoreWebView2ContextMenuItem displayUriParentContextMenuItem = null;

        // <CustomContextMenu>
        void WebView_ContextMenuRequested(
              object sender,
              CoreWebView2ContextMenuRequestedEventArgs args)
        {
            IList<CoreWebView2ContextMenuItem> menuList = args.MenuItems;
            CoreWebView2ContextMenuTargetKind context = args.ContextMenuTarget.Kind;
            // Using custom context menu UI
            if (context == CoreWebView2ContextMenuTargetKind.SelectedText)
            {
                CoreWebView2Deferral deferral = args.GetDeferral();
                args.Handled = true;
                ContextMenu cm = new ContextMenu();
                cm.Closed += (s, ex) => deferral.Complete();
                PopulateContextMenu(args, menuList, cm);
                cm.IsOpen = true;
            }
            // Remove item from WebView context menu
            else if (context == CoreWebView2ContextMenuTargetKind.Image)
            {
                /// removes the last item in the collection
                menuList.RemoveAt(menuList.Count - 1);
            }
            // Add item to WebView context menu
            else if (context == CoreWebView2ContextMenuTargetKind.Page)
            {
                // Created context menu items should be reused.
                if (displayUriParentContextMenuItem == null)
                {
                    CoreWebView2ContextMenuItem subItem =
                    webView.CoreWebView2.Environment.CreateContextMenuItem(
                        "Display Page Uri", null,
                        CoreWebView2ContextMenuItemKind.Command);
                    subItem.CustomItemSelected += delegate (object send, Object ex) {
                        string pageUrl = args.ContextMenuTarget.PageUri;
                        System.Threading.SynchronizationContext.Current.Post((_) => {
                            MessageBox.Show(pageUrl, "Display Page Uri", MessageBoxButton.YesNo);
                        }, null);
                    };
                    displayUriParentContextMenuItem =
                      webView.CoreWebView2.Environment.CreateContextMenuItem(
                          "New Submenu", null,
                          CoreWebView2ContextMenuItemKind.Submenu);
                    IList<CoreWebView2ContextMenuItem> submenuList = displayUriParentContextMenuItem.Children;
                    submenuList.Insert(0, subItem);
                }

                menuList.Insert(menuList.Count, displayUriParentContextMenuItem);
            }
        }
        // </CustomContextMenu>

        void PopulateContextMenu(CoreWebView2ContextMenuRequestedEventArgs args,
                                IList<CoreWebView2ContextMenuItem> menuList,
                                ItemsControl cm)
        {
            for (int i = 0; i < menuList.Count; i++)
            {
                CoreWebView2ContextMenuItem current = menuList[i];
                if (current.Kind == CoreWebView2ContextMenuItemKind.Separator)
                {
                    Separator sep = new Separator();
                    cm.Items.Add(sep);
                    continue;
                }
                MenuItem newItem = new MenuItem();
                // The accessibility key is the key after the & in the label
                newItem.Header = current.Label.Replace('&', '_');
                newItem.InputGestureText = current.ShortcutKeyDescription;
                newItem.IsEnabled = current.IsEnabled;
                if (current.Kind == CoreWebView2ContextMenuItemKind.Submenu)
                {
                    PopulateContextMenu(args, current.Children, newItem);
                }
                else
                {
                    if (current.Kind == CoreWebView2ContextMenuItemKind.CheckBox ||
                        current.Kind == CoreWebView2ContextMenuItemKind.Radio)
                    {
                        newItem.IsCheckable = true;
                        newItem.IsChecked = current.IsChecked;
                    }

                    newItem.Click +=
                        (s, ex) => { args.SelectedCommandId = current.CommandId; };
                }
                cm.Items.Add(newItem);
            }
        }

        void AuthenticationCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <BasicAuthenticationRequested>
            webView.CoreWebView2.BasicAuthenticationRequested += delegate (object sender, CoreWebView2BasicAuthenticationRequestedEventArgs args)
            {
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Demo credentials in https://authenticationtest.com")]
                args.Response.UserName = "user";
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Demo credentials in https://authenticationtest.com")]
                args.Response.Password = "pass";
            };
            webView.CoreWebView2.Navigate("https://authenticationtest.com/HTTPAuth");
            // </BasicAuthenticationRequested>
        }

        private bool _isFaviconChanged = false;
        void FaviconChangedCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (!_isFaviconChanged)
                {
                    webView.CoreWebView2.FaviconChanged +=
                        Webview2_FaviconChanged;
                }
                else
                {
                    webView.CoreWebView2.FaviconChanged -=
                        Webview2_FaviconChanged;
                }
                _isFaviconChanged = !_isFaviconChanged;
                MessageBox.Show(this,
                                _isFaviconChanged
                                    ? "Favicon Changed Listeners have been enabled"
                                    : "Favicon Changed Listeners have been disabled",
                                "Favicon Changed Listeners");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Favicon Changed Listeners: " + exception.Message,
                                "Favicon Changed Listeners");
            }
        }

        async void Webview2_FaviconChanged(object sender,object args) {
            string value = webView.CoreWebView2.FaviconUri;
            System.IO.Stream stream = await webView.CoreWebView2.GetFaviconAsync(
              CoreWebView2FaviconImageFormat.Png);
            if (stream == null || stream.Length == 0)
                this.Icon = null;
            else
                this.Icon = BitmapFrame.Create(stream);
        }

        void PinchZoomCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <TogglePinchZoomEnabled>
            WebViewSettings.IsPinchZoomEnabled = !WebViewSettings.IsPinchZoomEnabled;
            // </TogglePinchZoomEnabled>
            MessageBox.Show("Pinch Zoom is" + (WebViewSettings.IsPinchZoomEnabled ? " enabled " : " disabled ") + "after the next navigation.");
        }

        void SwipeNavigationCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // Safeguarding the handler when unsupported runtime is used.
            try
            {
                // <ToggleSwipeNavigationEnabled>
                WebViewSettings.IsSwipeNavigationEnabled = !WebViewSettings.IsSwipeNavigationEnabled;
                // </ToggleSwipeNavigationEnabled>
                MessageBox.Show("Swipe to navigate is" + (WebViewSettings.IsSwipeNavigationEnabled ? " enabled " : " disabled ") + "after the next navigation.");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Toggle Swipe Navigation Failed: " + exception.Message, "Swipe Navigation");
            }
        }

        void PdfToolbarSaveCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ToggleHiddenPdfToolbarItems>
            if (WebViewSettings.HiddenPdfToolbarItems.HasFlag(CoreWebView2PdfToolbarItems.Save))
            {
                WebViewSettings.HiddenPdfToolbarItems = CoreWebView2PdfToolbarItems.None;
                MessageBox.Show("Save button on PDF toolbar is enabled after the next navigation.");
            }
            else
            {
                WebViewSettings.HiddenPdfToolbarItems = CoreWebView2PdfToolbarItems.Save;
                MessageBox.Show("Save button on PDF toolbar is disabled after the next navigation.");
            }
            // </ToggleHiddenPdfToolbarItems>
        }

        async void ClearBrowsingDataCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ClearBrowsingData>
            string dataKindString = e.Parameter.ToString();
            CoreWebView2BrowsingDataKinds dataKinds;
            if (dataKindString == "Cookies")
            {
                dataKinds = CoreWebView2BrowsingDataKinds.Cookies;
            }
            else if (dataKindString == "DOM Storage")
            {
                dataKinds = CoreWebView2BrowsingDataKinds.AllDomStorage;
            }
            else if (dataKindString == "Site")
            {
                dataKinds = CoreWebView2BrowsingDataKinds.AllSite;
            }
            else if (dataKindString == "Disk Cache")
            {
                dataKinds = CoreWebView2BrowsingDataKinds.DiskCache;
            }
            else if (dataKindString == "Download History")
            {
                dataKinds = CoreWebView2BrowsingDataKinds.DownloadHistory;
            }
            else if (dataKindString == "Autofill")
            {
                dataKinds = (CoreWebView2BrowsingDataKinds)(CoreWebView2BrowsingDataKinds.GeneralAutofill | CoreWebView2BrowsingDataKinds.PasswordAutosave);
            }
            else if (dataKindString == "Browsing History")
            {
                dataKinds = CoreWebView2BrowsingDataKinds.BrowsingHistory;
            }
            else
            {
                dataKinds = CoreWebView2BrowsingDataKinds.AllProfile;
            }
            System.DateTime endTime = DateTime.Now;
            System.DateTime startTime = DateTime.Now.AddHours(-1);

            // Clear the browsing data from the last hour.
            await WebViewProfile.ClearBrowsingDataAsync(dataKinds, startTime, endTime);
            MessageBox.Show(this,
               "Completed",
               "Clear Browsing Data");
            // </ClearBrowsingData>
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
                // <DownloadStarting>
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
                // </DownloadStarting>
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "DownloadStarting Failed: " + exception.Message, "Download Starting");
            }
        }

        void SetDefaultDownloadPathCmdExecuted(object target,
            ExecutedRoutedEventArgs e)
        {
            try
            {
                var dialog = new TextInputDialog(
                    title: "Set Default Download Folder Path",
                    description: "Enter the new default download folder path.",
                    defaultInput: WebViewProfile.DefaultDownloadFolderPath);
                if (dialog.ShowDialog() == true)
                {
                    WebViewProfile.DefaultDownloadFolderPath = dialog.Input.Text;
                }
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this,
                    "Set default download folder path failed: " +
                    exception.Message, "Set Default Download Folder Path");
            }
        }

        // Update download progress
        void UpdateProgress(CoreWebView2DownloadOperation download)
        {
            // <BytesReceivedChanged>
            download.BytesReceivedChanged += delegate (object sender, Object e)
            {
                // Here developer can update download dialog to show progress of a
                // download using `download.BytesReceived` and `download.TotalBytesToReceive`
            };
            // </BytesReceivedChanged>

            // <StateChanged>
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
            // </StateChanged>
        }

        // <ClientCertificateRequested1>
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
        // </ClientCertificateRequested1>

        // <ClientCertificateRequested2>
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
        // </ClientCertificateRequested2>

        // <ServerCertificateErrorDetected>
        // When WebView2 doesn't trust a TLS certificate but host app does, this example bypasses
        // the default TLS interstitial page using the ServerCertificateErrorDetected event handler and
        // continues the request to a server. Otherwise, cancel the request.
        private bool _isServerCertificateError = false;
        void ToggleCustomServerCertificateSupport()
        {
            // Safeguarding the handler when unsupported runtime is used.
            try
            {
                if (!_isServerCertificateError)
                {
                    webView.CoreWebView2.ServerCertificateErrorDetected += WebView_ServerCertificateErrorDetected;
                }
                else
                {
                    webView.CoreWebView2.ServerCertificateErrorDetected -= WebView_ServerCertificateErrorDetected;
                }
                _isServerCertificateError = !_isServerCertificateError;

                MessageBox.Show(this, "Custom server certificate support has been" +
                    (_isServerCertificateError ? "enabled" : "disabled"),
                    "Custom server certificate support");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Custom server certificate support failed: " + exception.Message, "Custom server certificate support");
            }
        }

        void WebView_ServerCertificateErrorDetected(object sender, CoreWebView2ServerCertificateErrorDetectedEventArgs e)
        {
           CoreWebView2Certificate certificate = e.ServerCertificate;

            // Continues the request to a server with a TLS certificate if the error status
            // is of type `COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID`
            // and trusted by the host app.
            if (e.ErrorStatus == CoreWebView2WebErrorStatus.CertificateIsInvalid &&
                            ValidateServerCertificate(certificate))
            {
                e.Action = CoreWebView2ServerCertificateErrorAction.AlwaysAllow;
            }
            else
            {
                // Cancel the request for other TLS certificate error types or if untrusted by the host app.
                e.Action = CoreWebView2ServerCertificateErrorAction.Cancel;
            }
        }

        // Function to validate the server certificate for untrusted root or self-signed certificate.
        // You may also choose to defer server certificate validation.
        bool ValidateServerCertificate(CoreWebView2Certificate certificate)
        {
           // You may want to validate certificates in different ways depending on your app and
           // scenario. One way might be the following:
           // First, get the list of host app trusted certificates and its thumbprint.
           //
           // Then get the last chain element using `ICoreWebView2Certificate::get_PemEncodedIssuerCertificateChain`
           // that contains the raw data of the untrusted root CA/self-signed certificate. Get the untrusted
           // root CA/self signed certificate thumbprint from the raw certificate data and validate the thumbprint
           // against the host app trusted certificate list.
           //
           // Finally, return true if it exists in the host app's certificate trusted list, or otherwise return false.
           return true;
        }

        // This example clears `AlwaysAllow` response that are added for proceeding with TLS certificate errors.
        async void ClearServerCertificateErrorActions()
        {
            await webView.CoreWebView2.ClearServerCertificateErrorActionsAsync();
            MessageBox.Show(this, "message", "Clear server certificate error actions are succeeded");
        }
        // </ServerCertificateErrorDetected>

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

            // <Navigate>
            // Setting webView.Source will not trigger a navigation if the Source is the same
            // as the previous Source.  CoreWebView.Navigate() will always trigger a navigation.
            webView.CoreWebView2.Navigate(uri.ToString());
            // </Navigate>
        }

        async void SuspendCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                // <TrySuspend>
                bool isSuccessful = await webView.CoreWebView2.TrySuspendAsync();
                MessageBox.Show(this,
                    (isSuccessful) ? "TrySuspendAsync succeeded" : "TrySuspendAsync failed",
                    "TrySuspendAsync");
                // </TrySuspend>
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
                // <Resume>
                webView.CoreWebView2.Resume();
                MessageBox.Show(this, "Resume Succeeded", "Resume");
                // </Resume>
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
                // <UpdateRuntime>
                CoreWebView2UpdateRuntimeResult result = await webView.CoreWebView2.Environment.UpdateRuntimeAsync();
                string update_result = "status: " + result.Status + ", extended error:" + result.ExtendedError;
                MessageBox.Show(this, update_result, "UpdateRuntimeAsync result");
                // </UpdateRuntime>
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
                // <AllowWebViewShortcutKeys>
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
                // </AllowWebViewShortcutKeys>
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

        private string GetSdkBuildVersion()
        {
            CoreWebView2EnvironmentOptions options = new CoreWebView2EnvironmentOptions();

            // The full version string A.B.C.D
            var targetVersionMajorAndRest = options.TargetCompatibleBrowserVersion;
            var versionList = targetVersionMajorAndRest.Split('.');
            if (versionList.Length != 4)
            {
                return "Invalid SDK build version";
            }
            // Keep C.D
            return versionList[2] + "." + versionList[3];
        }

        private string GetRuntimeVersion(CoreWebView2 webView2)
        {
            return webView2.Environment.BrowserVersionString;
        }

        private string GetAppPath()
        {
            return System.AppDomain.CurrentDomain.SetupInformation.ApplicationBase;
        }

        private string GetRuntimePath(CoreWebView2 webView2)
        {
            int processId = (int)webView2.BrowserProcessId;
            try
            {
                Process process = System.Diagnostics.Process.GetProcessById(processId);
                var fileName = process.MainModule.FileName;
                return System.IO.Path.GetDirectoryName(fileName);
            }
            catch (ArgumentException e)
            {
                return e.Message;
            }
            catch (InvalidOperationException e)
            {
                return e.Message;
            }
            catch (Win32Exception)
            {
                return "N/A";
            }
        }

        private string GetStartPageUri(CoreWebView2 webView2)
        {
            string uri = "https://appassets.example/AppStartPage.html";
            if (webView2 == null)
            {
                return uri;
            }
            string sdkBuildVersion = GetSdkBuildVersion(),
                   runtimeVersion = GetRuntimeVersion(webView2),
                   appPath = GetAppPath(),
                   runtimePath = GetRuntimePath(webView2);
            string newUri = $"{uri}?sdkBuild={sdkBuildVersion}&runtimeVersion={runtimeVersion}" +
                $"&appPath={appPath}&runtimePath={runtimePath}";
            return newUri;
        }

        void WebView_CoreWebView2InitializationCompleted(object sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            if (e.IsSuccess)
            {
                // Setup host resource mapping for local files
                webView.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.example", "assets", CoreWebView2HostResourceAccessKind.DenyCors);
                // Set StartPage Uri
                webView.Source = new Uri(GetStartPageUri(webView.CoreWebView2));

                // <ProcessFailed>
                webView.CoreWebView2.ProcessFailed += WebView_ProcessFailed;
                // </ProcessFailed>
                // <DocumentTitleChanged>
                webView.CoreWebView2.DocumentTitleChanged += WebView_DocumentTitleChanged;
                // </DocumentTitleChanged>
                // <IsDocumentPlayingAudioChanged>
                webView.CoreWebView2.IsDocumentPlayingAudioChanged += WebView_IsDocumentPlayingAudioChanged;
                // </IsDocumentPlayingAudioChanged>
                // <IsMutedChanged>
                webView.CoreWebView2.IsMutedChanged += WebView_IsMutedChanged;
                // </IsMutedChanged>
                // <PermissionRequested>
                webView.CoreWebView2.PermissionRequested += WebView_PermissionRequested;
                // </PermissionRequested>

                // The CoreWebView2Environment instance is reused when re-assigning CoreWebView2CreationProperties
                // to the replacement control. We don't need to re-attach the event handlers unless the environment
                // instance has changed.
                if (shouldAttachEnvironmentEventHandlers)
                {
                    try
                    {
                        // <SubscribeToBrowserProcessExited>
                        WebViewEnvironment.BrowserProcessExited += Environment_BrowserProcessExited;
                        // </SubscribeToBrowserProcessExited>
                        // <SubscribeToNewBrowserVersionAvailable>
                        WebViewEnvironment.NewBrowserVersionAvailable += Environment_NewBrowserVersionAvailable;
                        // </SubscribeToNewBrowserVersionAvailable>
                        // <ProcessInfosChanged>
                        WebViewEnvironment.ProcessInfosChanged += WebView_ProcessInfosChanged;
                        // </ProcessInfosChanged>
                    }
                    catch (NotImplementedException)
                    {
                        newVersionMenuItem.IsEnabled = false;
                    }
                    shouldAttachEnvironmentEventHandlers = false;
                }

                webView.CoreWebView2.FrameCreated += WebView_HandleIFrames;

                SetDefaultDownloadDialogPosition();

                return;
            }

            MessageBox.Show($"WebView2 creation failed with exception = {e.InitializationException}");
        }
        private void SetDefaultDownloadDialogPosition()
        {
            try
            {
                // <SetDefaultDownloadDialogPosition>
                const int defaultMarginX = 75, defaultMarginY = 0;
                CoreWebView2DefaultDownloadDialogCornerAlignment cornerAlignment
                    = CoreWebView2DefaultDownloadDialogCornerAlignment.TopLeft;
                System.Drawing.Point margin = new System.Drawing.Point(
                    defaultMarginX, defaultMarginY);
                webView.CoreWebView2.DefaultDownloadDialogCornerAlignment =
                    cornerAlignment;
                webView.CoreWebView2.DefaultDownloadDialogMargin = margin;
                // </SetDefaultDownloadDialogPosition>
            }
            catch (NotImplementedException) {}
        }

        // <BrowserProcessExited>
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
        // </BrowserProcessExited>

        void WebView_HandleIFrames(object sender, CoreWebView2FrameCreatedEventArgs args)
        {
            _webViewFrames.Add(args.Frame);
            args.Frame.Destroyed += (frameDestroyedSender, frameDestroyedArgs) =>
            {
                var frameToRemove = _webViewFrames.SingleOrDefault(r => r.IsDestroyed() == 1);
                if (frameToRemove != null)
                    _webViewFrames.Remove(frameToRemove);
            };
        }

        string WebViewFrames_ToString()
        {
            string result = "";
            for (var i = 0; i < _webViewFrames.Count; i++)
            {
                if (i > 0) result += "; ";
                result += i.ToString() + " " +
                    (String.IsNullOrEmpty(_webViewFrames[i].Name) ? "<empty_name>" : _webViewFrames[i].Name);
            }
            return String.IsNullOrEmpty(result) ? "no iframes available." : result;
        }

        // <NewBrowserVersionAvailable>
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
        // </NewBrowserVersionAvailable>

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
            // <WebResourceResponseReceived>
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
            // </WebResourceResponseReceived>
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

        void WebView_DocumentTitleChanged(object sender, object e)
        {
            // <DocumentTitle>
            this.Title = webView.CoreWebView2.DocumentTitle;
            // </DocumentTitle>
        }

        void UpdateTitleWithMuteState()
        {
            // <UpdateTitleWithMuteState>
            bool isDocumentPlayingAudio = webView.CoreWebView2.IsDocumentPlayingAudio;
            bool isMuted = webView.CoreWebView2.IsMuted;
            string currentDocumentTitle = webView.CoreWebView2.DocumentTitle;
            if (isDocumentPlayingAudio)
            {
                if (isMuted)
                {
                    this.Title = "🔇 " + currentDocumentTitle;
                }
                else
                {
                    this.Title = "🔊 " + currentDocumentTitle;
                }
            }
            else
            {
                this.Title = currentDocumentTitle;
            }
            // </UpdateTitleWithMuteState>
        }
        void WebView_IsMutedChanged(object sender, object e)
        {
            UpdateTitleWithMuteState();
        }
        void WebView_IsDocumentPlayingAudioChanged(object sender, object e)
        {
            UpdateTitleWithMuteState();
        }

        void ToggleMuteStateCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ToggleIsMuted>
            webView.CoreWebView2.IsMuted = !webView.CoreWebView2.IsMuted;
            // </ToggleIsMuted>
        }

        void AllowExternalDropCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ToggleAllowExternalDrop>
            webView.AllowExternalDrop = !webView.AllowExternalDrop;
            // </ToggleAllowExternalDrop>
        }

        // <GetProcessInfos>
        void WebView_ProcessInfosChanged(object sender, object e)
        {
            _processList = WebViewEnvironment.GetProcessInfos();
        }

        void PerfInfoCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            string result;
            int processListCount = _processList.Count;
            if (processListCount == 0)
            {
                result = "No process found.";
            }
            else
            {
                result = $"{processListCount} child process(s) found\n\n";
                for (int i = 0; i < processListCount; ++i)
                {
                    int processId = _processList[i].ProcessId;
                    CoreWebView2ProcessKind kind = _processList[i].Kind;

                    var proc = Process.GetProcessById(processId);
                    var memoryInBytes = proc.PrivateMemorySize64;
                    var b2kb = memoryInBytes / 1024;
                    result = result + $"Process ID: {processId} | Process Kind: {kind} | Memory: {b2kb} KB\n";
                }
            }

            MessageBox.Show(this, result, "Process List");
        }
        // </GetProcessInfos>

        void CreateDownloadsButtonCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            Button downloadsButton = new Button();
            downloadsButton.Content = "Downloads";
            downloadsButton.Click +=
                new RoutedEventHandler(ToggleDownloadDialog);
            DockPanel.SetDock(downloadsButton, Dock.Left);
            dockPanel.Children.Insert(dockPanel.Children.IndexOf(url),
                downloadsButton);
            try
            {
                // Subscribe to the `IsDefaultDownloadDialogOpenChanged` event
                // to make changes in response to the default download dialog
                // opening or closing. For example, if the dialog is anchored to
                // a button in the application, the button appearance can change
                // depending on whether the dialog is opened or closed.
                webView.CoreWebView2.IsDefaultDownloadDialogOpenChanged +=
                    (sender, args) =>
                {
                    if (webView.CoreWebView2.IsDefaultDownloadDialogOpen)
                    {
                        downloadsButton.Background = new SolidColorBrush(
                            Colors.LightBlue);
                    }
                    else
                    {
                        downloadsButton.Background = new SolidColorBrush(
                            Colors.AliceBlue);
                    }
                };
            }
            catch (NotImplementedException) {}
        }
        private void ToggleDownloadDialog(object target, RoutedEventArgs e)
        {
            try
            {
                // <ToggleDefaultDownloadDialog>
                if (webView.CoreWebView2.IsDefaultDownloadDialogOpen)
                {
                    webView.CoreWebView2.CloseDefaultDownloadDialog();
                }
                else
                {
                    webView.CoreWebView2.OpenDefaultDownloadDialog();
                }
                // </ToggleDefaultDownloadDialog>
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Toggle download dialog: " + exception.Message,
                    "Download Dialog");
            }
        }

        void NewWindowWithOptionsCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            var dialog = new NewWindowOptionsDialog();
            if (dialog.ShowDialog() == true)
            {
                new MainWindow(dialog.CreationProperties).Show();
            }
        }

        private void ThreadProc(string browserExecutableFolder, string userDataFolder, string language, string additionalBrowserArguments, string profileName, bool? isInPrivateModeEnabled)
        {
            try
            {
                // Some class members are global static properties, and VerifyAccess() will throw exceptions when accessing across threads. So prepare copies of them.
                // See DispatcherObject Class on MSDN: Only the thread that the Dispatcher was created on may access the DispatcherObject directly.
                CoreWebView2CreationProperties creationProperties = new CoreWebView2CreationProperties();
                creationProperties.BrowserExecutableFolder = browserExecutableFolder;
                creationProperties.UserDataFolder = userDataFolder;
                creationProperties.Language = language;
                creationProperties.AdditionalBrowserArguments = additionalBrowserArguments;
                creationProperties.ProfileName = profileName;
                creationProperties.IsInPrivateModeEnabled = isInPrivateModeEnabled;
                var tempWindow = new MainWindow(creationProperties);
                tempWindow.Show();
                // Causes dispatcher to shutdown when window is closed.
                tempWindow.Closed += (s, e) =>
                {
                    System.Windows.Threading.Dispatcher.CurrentDispatcher.InvokeShutdown();
                };
                // Run the message pump
                System.Windows.Threading.Dispatcher.Run();
            }
            catch (Exception exception)
            {
                MessageBox.Show("Create New Thread Failed: " + exception.Message, "Create New Thread");
            }
        }

        void CreateNewThreadCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            string browserExecutableFolder = webView.CreationProperties.BrowserExecutableFolder;
            string userDataFolder = webView.CreationProperties.UserDataFolder;
            string language = webView.CreationProperties.Language;
            string additionalBrowserArguments = webView.CreationProperties.AdditionalBrowserArguments;
            string profileName = webView.CreationProperties.ProfileName;
            bool? isInPrivateModeEnabled = webView.CreationProperties.IsInPrivateModeEnabled;
            Thread newWindowThread = new Thread(() =>
            {
                ThreadProc(browserExecutableFolder, userDataFolder, language, additionalBrowserArguments, profileName, isInPrivateModeEnabled);
            });
            newWindowThread.SetApartmentState(ApartmentState.STA);
            newWindowThread.IsBackground = false;
            newWindowThread.Start();
        }
        // Commands(V2)
        void AboutCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            MessageBox.Show(this, "WebView2WpfBrowser, Version 1.0\nCopyright(C) 2022", "About WebView2WpfBrowser");
        }

        private void SmartScreenEnabledExecuted(object sender, ExecutedRoutedEventArgs e)
        {
            // <ToggleSmartScreenEnabled>
            WebViewSettings.IsReputationCheckingRequired = !WebViewSettings.IsReputationCheckingRequired;
            // </ToggleSmartScreenEnabled>
            MessageBox.Show("SmartScreen is" + (WebViewSettings.IsReputationCheckingRequired ? " enabled " : " disabled ") + "after the next navigation.");
        }


        void PostMessageStringCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Post Web Message String",
                description: "Web message string:\r\nEnter the web message as a string.");

            try
            {
                if (dialog.ShowDialog() == true)
                {
                    webView.CoreWebView2.PostWebMessageAsString(dialog.Input.Text);
                }
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "PostMessageAsString Failed: " + exception.Message,
                   "Post Message As String");
            }
        }


        void PostMessageJSONCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Post Web Message JSON",
                description: "Web message JSON:\r\nEnter the web message as JSON.",
                 defaultInput: "{\"SetColor\":\"blue\"}");

            try
            {
                if (dialog.ShowDialog() == true)
                {
                    webView.CoreWebView2.PostWebMessageAsJson(dialog.Input.Text);
                }
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "PostMessageAsJSON Failed: " + exception.Message,
                   "Post Message As JSON");
            }
        }   

        void GetDocumentTitleCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            MessageBox.Show(webView.CoreWebView2.DocumentTitle, "Document Title");
        }

        void GetUserDataFolderCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            try
            {
                MessageBox.Show(WebViewEnvironment.UserDataFolder, "User Data Folder");
            }
            catch (Exception exception)
            {
                MessageBox.Show(this, "Get User Data Folder Failed: " + exception.Message, "User Data Folder");
            }
        }

        // <SharedBuffer>
        private CoreWebView2SharedBuffer sharedBuffer = null;
        void SharedBufferRequestedCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.WebMessageReceived += WebView_WebMessageReceivedSharedBuffer;
            webView.CoreWebView2.FrameCreated += WebView_FrameCreatedSharedBuffer;
            webView.CoreWebView2.SetVirtualHostNameToFolderMapping(
                "appassets.example", "assets", CoreWebView2HostResourceAccessKind.DenyCors);
            webView.Source = new Uri("https://appassets.example/sharedBuffer.html");
        }

        void EnsureSharedBuffer()
        {
            ulong bufferSize = 128;
            if (sharedBuffer == null)
            {
                sharedBuffer = WebViewEnvironment.CreateSharedBuffer(bufferSize);

                using (Stream stream = sharedBuffer.OpenStream())
                {
                    using (StreamWriter writer = new StreamWriter(stream))
                    {
                        writer.Write("some data from .NET");
                    }
                }
            }
        }

        void WebView_WebMessageReceivedSharedBuffer(object sender, CoreWebView2WebMessageReceivedEventArgs args)
        {
            bool forWebView = (sender is CoreWebView2);
            if (args.TryGetWebMessageAsString() == "RequestShareBuffer")
            {
                EnsureSharedBuffer();
                if (forWebView)
                {
                    ((CoreWebView2)sender).PostSharedBufferToScript(sharedBuffer, CoreWebView2SharedBufferAccess.ReadWrite, null);
                }
                else
                {
                    ((CoreWebView2Frame)sender).PostSharedBufferToScript(sharedBuffer, CoreWebView2SharedBufferAccess.ReadWrite, null);
                }
            }
            else if (args.TryGetWebMessageAsString() == "RequestOneTimeShareBuffer")
            {
                ulong bufferSize = 128;
                string data = "some read only data";
                // <OneTimeShareBuffer>
                SafeHandle fileMappingHandle;
                using (CoreWebView2SharedBuffer buffer = WebViewEnvironment.CreateSharedBuffer(bufferSize))
                {
                    fileMappingHandle = buffer.FileMappingHandle;
                    AssertCondition(fileMappingHandle.IsInvalid == false, "FileMappingHandle of a valid shared buffer should be valid");
                    using (Stream stream = buffer.OpenStream())
                    {
                        using (StreamWriter writer = new StreamWriter(stream))
                        {
                            writer.Write(data);
                        }
                    }
                    string additionalDataAsJson = "{\"myBufferType\":\"bufferType1\"}";
                    if (forWebView)
                    {
                        ((CoreWebView2)sender).PostSharedBufferToScript(buffer, CoreWebView2SharedBufferAccess.ReadOnly, additionalDataAsJson);
                    }
                    else
                    {
                        ((CoreWebView2Frame)sender).PostSharedBufferToScript(buffer, CoreWebView2SharedBufferAccess.ReadOnly, additionalDataAsJson);
                    }
                }
                AssertCondition(fileMappingHandle.IsInvalid == true, "FileMappingHandle of a disposed shared buffer should be invalid");
                // </OneTimeShareBuffer>
            }
            else if (args.TryGetWebMessageAsString() == "SharedBufferDataUpdated")
            {
                ShowSharedBuffer();
            }
        }

        void WebView_FrameCreatedSharedBuffer(object sender, CoreWebView2FrameCreatedEventArgs args)
        {
            args.Frame.WebMessageReceived += (WebMessageReceivedSender, WebMessageReceivedArgs) =>
            {
                WebView_WebMessageReceivedSharedBuffer(WebMessageReceivedSender, WebMessageReceivedArgs);
            };
        }

        void ShowSharedBuffer()
        {
            using (Stream stream = sharedBuffer.OpenStream())
            {
                using (StreamReader reader = new StreamReader(stream))
                {
                    string text_content = reader.ReadLine();
                    MessageBox.Show(text_content, "shared buffer updated");
                }
            }
        }
        // </SharedBuffer>
        
        // Prompt the user for some script and register it to execute whenever a new page loads.
        private async void AddInitializeScriptCmdExecuted(object sender, ExecutedRoutedEventArgs e) {
            TextInputDialog dialog = new TextInputDialog(
              title: "Add Initialize Script",
              description: "Enter the JavaScript code to run as the initialization script that runs before any script in the HTML document.",
              // This example script stops child frames from opening new windows.  Because
              // the initialization script runs before any script in the HTML document, we
              // can trust the results of our checks on window.parent and window.top.
              defaultInput: "if (window.parent !== window.top) {\r\n" +
              "    delete window.open;\r\n" +
              "}");
            if (dialog.ShowDialog() == true)
            {
                try
                {
                  string scriptId = await webView.CoreWebView2.AddScriptToExecuteOnDocumentCreatedAsync(dialog.Input.Text);
                  m_lastInitializeScriptId = scriptId;
                  MessageBox.Show(this, scriptId, "AddScriptToExecuteOnDocumentCreated Id");
                }
                catch (Exception ex)
                {
                  MessageBox.Show(this, ex.ToString(), "AddScriptToExecuteOnDocumentCreated failed");
                }
            }
        }

        // Prompt the user for an initialization script ID and deregister that script.
        private void RemoveInitializeScriptCmdExecuted(object sender, ExecutedRoutedEventArgs e) {
            TextInputDialog dialog = new TextInputDialog(
              title: "Remove Initialize Script",
              description: "Enter the ID created from Add Initialize Script.",
              defaultInput: m_lastInitializeScriptId);
            if (dialog.ShowDialog() == true)
            {
                string scriptId = dialog.Input.Text;
                // check valid
                try
                {
                  Int64 result = Int64.Parse(scriptId);
                  Int64 lastId = Int64.Parse(m_lastInitializeScriptId);
                  if (result > lastId) {
                    MessageBox.Show(this, scriptId, "Invalid ScriptId, should be less or equal than " + m_lastInitializeScriptId);
                  } else {
                    webView.CoreWebView2.RemoveScriptToExecuteOnDocumentCreated(scriptId);
                    if (result == lastId && lastId >= 2) {
                      m_lastInitializeScriptId = (lastId - 1).ToString();
                    }
                    MessageBox.Show(this, scriptId, "RemoveScriptToExecuteOnDocumentCreated Id");
                  }
                }
                catch (FormatException) {
                  MessageBox.Show(this, scriptId, "Invalid ScriptId, should be Integer");
                }
            }
        }
        // Prompt the user for the name and parameters of a CDP method, then call it.
        private async void CallCdpMethodCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
          TextInputDialog dialog = new TextInputDialog(
            title: "Call CDP Method",
            description: "Enter the CDP method name to call, followed by a space,\r\n" +
              "followed by the parameters in JSON format.",
            defaultInput: "Runtime.evaluate {\"expression\":\"alert(\\\"test\\\")\"}"
          );
          if (dialog.ShowDialog() == true)
          {
            string[] words = dialog.Input.Text.Trim().Split(' ');
            if (words.Length == 1 && words[0] == "") {
              MessageBox.Show(this, "Invalid argument:" + dialog.Input.Text, "CDP Method call failed");
              return;
            }
            string methodName = words[0];
            string methodParams = (words.Length == 2 ? words[1] : "{}");

            try
            {
              string cdpResult = await webView.CoreWebView2.CallDevToolsProtocolMethodAsync(methodName, methodParams);
              MessageBox.Show(this, cdpResult, "CDP method call successfully");
            }
            catch (Exception ex)
            {
              MessageBox.Show(this, ex.ToString(), "CDP method call failed");
            }
          }
        }

        private void OpenDevToolsCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
          try
          {
            webView.CoreWebView2.OpenDevToolsWindow();
          }
          catch (Exception ex) {
            MessageBox.Show(this, ex.ToString(), "Open DevTools Window failed");
          }
        }

        private void OpenTaskManagerCmdExecuted(object sender, ExecutedRoutedEventArgs e)
        {
          try
          {
            webView.CoreWebView2.OpenTaskManagerWindow();
          }
          catch (Exception ex) {
            MessageBox.Show(this, ex.ToString(), "Open Task Manager Window failed");
          }
        }


        void CrashBrowserProcessCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.Navigate("edge://inducebrowsercrashforrealz");
        }

        void CrashRenderProcessCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            webView.CoreWebView2.Navigate("edge://kill");
        }



        void HostObjectsAllowedCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ToggleHostObjectsAllowed>
            WebViewSettings.AreHostObjectsAllowed = !WebViewSettings.AreHostObjectsAllowed;
            // </ToggleHostObjectsAllowed>
            MessageBox.Show("Access to host objects will be" + (WebViewSettings.AreHostObjectsAllowed ? " allowed " : " denied ") + "after the next navigation.");
        }

        void BrowserAcceleratorKeyEnabledCommandExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ToggleBrowserAcceleratorKeyEnabled>
            WebViewSettings.AreBrowserAcceleratorKeysEnabled = !WebViewSettings.AreBrowserAcceleratorKeysEnabled;
            // </ToggleBrowserAcceleratorKeyEnabled>
            MessageBox.Show("Browser-specific accelerator keys will be" + (WebViewSettings.AreBrowserAcceleratorKeysEnabled ? " enabled " : " disabled ") + "after the next navigation.");
        }


        async void InjectScriptWithResultCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            // <ExecuteScriptWithResult>
            var dialog = new TextInputDialog(
                title: "Inject Script With Result",
                description: "Enter some JavaScript to be executed in the context of this page, and get the error info when execution failed",
                defaultInput: ""
            );
            if (dialog.ShowDialog() == true) {
                var result = await webView.CoreWebView2.ExecuteScriptWithResultAsync(dialog.Input.Text);
                if (result.Succeeded)
                {
                    MessageBox.Show(this, result.ResultAsJson, "ExecuteScript Json Result");
                    int is_success = 0;
                    string str = "";
                    result.TryGetResultAsString(out str, out is_success);
                    if (is_success == 1)
                    {
                        MessageBox.Show(this, str, "ExecuteScript String Result");
                    }
                    else {
                        MessageBox.Show(this, "Get string failed", "ExecuteScript String Result");
                    }
                }
                else {
                    var exception = result.Exception;
                    MessageBox.Show(this, exception.Name, "ExecuteScript Exception Name");
                    MessageBox.Show(this, exception.Message, "ExecuteScript Exception Message");
                    MessageBox.Show(this, exception.ToJson, "ExecuteScript Exception Detail");
                    var location_info = "LineNumber:" + exception.LineNumber + ", ColumnNumber:" + exception.ColumnNumber;
                    MessageBox.Show(this, location_info, "ExecuteScript Exception Location");
                }
            }
            // </ExecuteScriptWithResult>
        }

        string NameOfPermissionKind(CoreWebView2PermissionKind kind)
        {
            switch (kind)
            {
                case CoreWebView2PermissionKind.Microphone:
                    return "Microphone";
                case CoreWebView2PermissionKind.Camera:
                    return "Camera";
                case CoreWebView2PermissionKind.Geolocation:
                    return "Geolocation";
                case CoreWebView2PermissionKind.Notifications:
                    return "Notifications";
                case CoreWebView2PermissionKind.OtherSensors:
                    return "OtherSensors";
                case CoreWebView2PermissionKind.ClipboardRead:
                    return "ClipboardRead";
                default:
                    return "Unknown";
            }
        }

        // <OnPermissionRequested>
        void WebView_PermissionRequested(object sender, CoreWebView2PermissionRequestedEventArgs args)
        {
            CoreWebView2Deferral deferral = args.GetDeferral();

            System.Threading.SynchronizationContext.Current.Post((_) =>
            {
                using (deferral)
                {
                    (string, CoreWebView2PermissionKind, bool) cachedKey = (args.Uri, args.PermissionKind, args.IsUserInitiated);
                    if (_cachedPermissions.ContainsKey(cachedKey))
                    {
                        args.State = _cachedPermissions[cachedKey]
                            ? CoreWebView2PermissionState.Allow
                            : CoreWebView2PermissionState.Deny;
                    }
                    else
                    {
                        string message = "An iframe has requested device permission for ";
                        message += NameOfPermissionKind(args.PermissionKind) + " to the website at ";
                        message += args.Uri + ".\n\nDo you want to grant permission?\n";
                        message += args.IsUserInitiated ? "This request came from a user gesture." : "This request did not come from a user gesture.";
                        var selection = MessageBox.Show(
                            message, "Permission Request", MessageBoxButton.YesNoCancel);
                        switch (selection)
                        {
                            case MessageBoxResult.Yes:
                                args.State = CoreWebView2PermissionState.Allow;
                                break;
                            case MessageBoxResult.No:
                                args.State = CoreWebView2PermissionState.Deny;
                                break;
                            case MessageBoxResult.Cancel:
                                args.State = CoreWebView2PermissionState.Default;
                                break;
                        }
                    }
                }
            }, null);
        }
        // </OnPermissionRequested>
    }
}
