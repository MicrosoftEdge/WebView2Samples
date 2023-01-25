// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Diagnostics;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace WebView2WindowsFormsBrowser
{
    public partial class BrowserForm : Form
    {
        private CoreWebView2CreationProperties _creationProperties = null;
        public CoreWebView2CreationProperties CreationProperties
        {
            get
            {
                if (_creationProperties == null)
                {
                    _creationProperties = new Microsoft.Web.WebView2.WinForms.CoreWebView2CreationProperties();
                }
                return _creationProperties;
            }
            set
            {
                _creationProperties = value;
            }
        }

        public BrowserForm()
        {
            InitializeComponent();
            AttachControlEventHandlers(this.webView2Control);
            HandleResize();
        }

        public BrowserForm(CoreWebView2CreationProperties creationProperties = null)
        {
            this.CreationProperties = creationProperties;
            InitializeComponent();
            AttachControlEventHandlers(this.webView2Control);
            HandleResize();
        }

        private void UpdateTitleWithEvent(string message)
        {
            string currentDocumentTitle = this.webView2Control?.CoreWebView2?.DocumentTitle ?? "Uninitialized";
            this.Text = currentDocumentTitle + " (" + message + ")";
        }

        CoreWebView2Environment _webViewEnvironment;
        CoreWebView2Environment WebViewEnvironment
        {
            get
            {
                if (_webViewEnvironment == null && webView2Control?.CoreWebView2 != null)
                {
                    _webViewEnvironment = webView2Control.CoreWebView2.Environment;
                }
                return _webViewEnvironment;
            }
        }

        CoreWebView2Settings _webViewSettings;
        CoreWebView2Settings WebViewSettings
        {
            get
            {
                if (_webViewSettings == null && webView2Control?.CoreWebView2 != null)
                {
                    _webViewSettings = webView2Control.CoreWebView2.Settings;
                }
                return _webViewSettings;
            }
        }

        #region Event Handlers
        // Enable (or disable) buttons when webview2 is init (or disposed). Similar to the CanExecute feature of WPF.
        private void UpdateButtons(bool isEnabled)
        {
            this.btnEvents.Enabled = isEnabled;
            this.btnBack.Enabled = isEnabled && webView2Control != null && webView2Control.CanGoBack;
            this.btnForward.Enabled = isEnabled && webView2Control != null && webView2Control.CanGoForward;
            this.btnRefresh.Enabled = isEnabled;
            this.btnGo.Enabled = isEnabled;
            this.closeWebViewToolStripMenuItem.Enabled = isEnabled;
            this.allowExternalDropMenuItem.Enabled = isEnabled;
            this.xToolStripMenuItem.Enabled = isEnabled;
            this.xToolStripMenuItem1.Enabled = isEnabled;
            this.xToolStripMenuItem2.Enabled = isEnabled;
            this.xToolStripMenuItem3.Enabled = isEnabled;
            this.whiteBackgroundColorMenuItem.Enabled = isEnabled;
            this.redBackgroundColorMenuItem.Enabled = isEnabled;
            this.blueBackgroundColorMenuItem.Enabled = isEnabled;
            this.transparentBackgroundColorMenuItem.Enabled = isEnabled;
        }

        private void EnableButtons()
        {
            UpdateButtons(true);
        }

        private void DisableButtons(object sender, EventArgs e)
        {
            UpdateButtons(false);
        }

        private void WebView2Control_NavigationStarting(object sender, CoreWebView2NavigationStartingEventArgs e)
        {
            UpdateTitleWithEvent("NavigationStarting");
        }

        private void WebView2Control_NavigationCompleted(object sender, CoreWebView2NavigationCompletedEventArgs e)
        {
            UpdateTitleWithEvent("NavigationCompleted");
        }

        private void WebView2Control_SourceChanged(object sender, CoreWebView2SourceChangedEventArgs e)
        {
            txtUrl.Text = webView2Control.Source.AbsoluteUri;
        }

        private void WebView2Control_CoreWebView2InitializationCompleted(object sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            if (!e.IsSuccess)
            {
                MessageBox.Show($"WebView2 creation failed with exception = {e.InitializationException}");
                UpdateTitleWithEvent("CoreWebView2InitializationCompleted failed");
                return;
            }

            // Setup host resource mapping for local files
            this.webView2Control.CoreWebView2.SetVirtualHostNameToFolderMapping("appassets.example", "assets", CoreWebView2HostResourceAccessKind.DenyCors);
            this.webView2Control.Source = new Uri(GetStartPageUri(this.webView2Control.CoreWebView2));

            this.webView2Control.CoreWebView2.SourceChanged += CoreWebView2_SourceChanged;
            this.webView2Control.CoreWebView2.HistoryChanged += CoreWebView2_HistoryChanged;
            this.webView2Control.CoreWebView2.DocumentTitleChanged += CoreWebView2_DocumentTitleChanged;
            this.webView2Control.CoreWebView2.AddWebResourceRequestedFilter("*", CoreWebView2WebResourceContext.Image);
            UpdateTitleWithEvent("CoreWebView2InitializationCompleted succeeded");
            EnableButtons();
        }

        void AttachControlEventHandlers(Microsoft.Web.WebView2.WinForms.WebView2 control) {
            control.CoreWebView2InitializationCompleted += WebView2Control_CoreWebView2InitializationCompleted;
            control.NavigationStarting += WebView2Control_NavigationStarting;
            control.NavigationCompleted += WebView2Control_NavigationCompleted;
            control.SourceChanged += WebView2Control_SourceChanged;
            control.KeyDown += WebView2Control_KeyDown;
            control.KeyUp += WebView2Control_KeyUp;
            control.Disposed += DisableButtons;
        }

        private void WebView2Control_KeyUp(object sender, KeyEventArgs e)
        {
            UpdateTitleWithEvent($"KeyUp key={e.KeyCode}");
            if (!this.acceleratorKeysEnabledToolStripMenuItem.Checked)
                e.Handled = true;
        }

        private void WebView2Control_KeyDown(object sender, KeyEventArgs e)
        {
            UpdateTitleWithEvent($"KeyDown key={e.KeyCode}");
            if (!this.acceleratorKeysEnabledToolStripMenuItem.Checked)
                e.Handled = true;
        }

        private void CoreWebView2_HistoryChanged(object sender, object e)
        {
            // No explicit check for webView2Control initialization because the events can only start
            // firing after the CoreWebView2 and its events exist for us to subscribe.
            btnBack.Enabled = webView2Control.CoreWebView2.CanGoBack;
            btnForward.Enabled = webView2Control.CoreWebView2.CanGoForward;
            UpdateTitleWithEvent("HistoryChanged");
        }

        private void CoreWebView2_SourceChanged(object sender, CoreWebView2SourceChangedEventArgs e)
        {
            this.txtUrl.Text = this.webView2Control.Source.AbsoluteUri;
            UpdateTitleWithEvent("SourceChanged");
        }

        private void CoreWebView2_DocumentTitleChanged(object sender, object e)
        {
            this.Text = this.webView2Control.CoreWebView2.DocumentTitle;
            UpdateTitleWithEvent("DocumentTitleChanged");
        }
        #endregion

        #region UI event handlers
        private void BtnRefresh_Click(object sender, EventArgs e)
        {
            webView2Control.Reload();
        }

        private void BtnGo_Click(object sender, EventArgs e)
        {
            var rawUrl = txtUrl.Text;
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

            webView2Control.Source = uri;
        }

        private void btnBack_Click(object sender, EventArgs e)
        {
            webView2Control.GoBack();
        }

        private void btnEvents_Click(object sender, EventArgs e)
        {
            (new EventMonitor(this.webView2Control)).Show(this);
        }

        private void btnForward_Click(object sender, EventArgs e)
        {
            webView2Control.GoForward();
        }

        private void Form_Resize(object sender, EventArgs e)
        {
            HandleResize();
        }

        private void closeWebViewToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Controls.Remove(webView2Control);
            webView2Control.Dispose();
        }

        private void createNewWindowToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new BrowserForm().Show();
        }

        private void createNewWindowWithOptionsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var dialog = new NewWindowOptionsDialog();
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                new BrowserForm(dialog.CreationProperties).Show();
            }
        }

        private void ThreadProc(CoreWebView2CreationProperties creationProperties)
        {
            try
            {
                var creationProps = new CoreWebView2CreationProperties();
                // The CoreWebView2CreationProperties object cannot be assigned directly, because its member _task will also be assigned.
                creationProps.BrowserExecutableFolder = creationProperties.BrowserExecutableFolder;
                creationProps.UserDataFolder = creationProperties.UserDataFolder;
                creationProps.Language = creationProperties.Language;
                creationProps.AdditionalBrowserArguments = creationProperties.AdditionalBrowserArguments;
                creationProps.ProfileName = creationProperties.ProfileName;
                creationProps.IsInPrivateModeEnabled = creationProperties.IsInPrivateModeEnabled;
                var tempForm = new BrowserForm(creationProps);
                tempForm.Show();
                // Run the message pump
                Application.Run();
            }
            catch (Exception exception)
            {
                MessageBox.Show("Create New Thread Failed: " + exception.Message, "Create New Thread");
            }
        }

        private void createNewThreadToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Thread newFormThread = new Thread(() =>
            {
                ThreadProc(webView2Control.CreationProperties);
            });
            newFormThread.SetApartmentState(ApartmentState.STA);
            newFormThread.IsBackground = false;
            newFormThread.Start();
        }

        private void xToolStripMenuItem05_Click(object sender, EventArgs e)
        {
            this.webView2Control.ZoomFactor = 0.5;
        }

        private void xToolStripMenuItem1_Click(object sender, EventArgs e)
        {
            this.webView2Control.ZoomFactor = 1.0;
        }

        private void xToolStripMenuItem2_Click(object sender, EventArgs e)
        {
            this.webView2Control.ZoomFactor = 2.0;
        }

        private void xToolStripMenuItem3_Click(object sender, EventArgs e)
        {
            MessageBox.Show($"Zoom factor: {this.webView2Control.ZoomFactor}", "WebView Zoom factor");
        }

        private void backgroundColorMenuItem_Click(object sender, EventArgs e)
        {
            var menuItem = (ToolStripMenuItem)sender;
            Color backgroundColor = Color.FromName(menuItem.Text);
            this.webView2Control.DefaultBackgroundColor = backgroundColor;
        }

        private void taskManagerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                this.webView2Control.CoreWebView2.OpenTaskManagerWindow();
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.ToString(), "Open Task Manager Window failed");
            }
        }

        private async void methodCDPToolStripMenuItem_Click(object sender, EventArgs e)
        {
            TextInputDialog dialog = new TextInputDialog(
              title: "Call CDP Method",
              description: "Enter the CDP method name to call, followed by a space,\r\n" +
                "followed by the parameters in JSON format.",
              defaultInput: "Runtime.evaluate {\"expression\":\"alert(\\\"test\\\")\"}"
            );
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                string[] words = dialog.inputBox().Trim().Split(' ');
                if (words.Length == 1 && words[0] == "")
                {
                    MessageBox.Show(this, "Invalid argument:" + dialog.inputBox(), "CDP Method call failed");
                    return;
                }
                string methodName = words[0];
                string methodParams = (words.Length == 2 ? words[1] : "{}");

                try
                {
                    string cdpResult = await this.webView2Control.CoreWebView2.CallDevToolsProtocolMethodAsync(methodName, methodParams);
                    MessageBox.Show(this, cdpResult, "CDP method call successfully");
                }
                catch (Exception ex)
                {
                    MessageBox.Show(this, ex.ToString(), "CDP method call failed");
                }
            }
        }

        private void allowExternalDropMenuItem_Click(object sender, EventArgs e)
        {
            this.webView2Control.AllowExternalDrop = this.allowExternalDropMenuItem.Checked;
        }

        private void setUsersAgentMenuItem_Click(object sender, EventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "SetUserAgent",
                description: "Enter UserAgent");
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                // <SetUserAgent>
                WebViewSettings.UserAgent = dialog.inputBox();
                // </SetUserAgent>
            }
        }

        private void getDocumentTitleMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show(webView2Control.CoreWebView2.DocumentTitle, "Document Title");
        }

        private bool _isPrintToPdfInProgress = false;
        private async void portraitMenuItem_Click(object sender, EventArgs e)
        {
            if (_isPrintToPdfInProgress)
            {
                MessageBox.Show(this, "Print to PDF in progress", "Print To PDF");
                return;
            }
            try
            {
                // <PrintToPdf as Portrait>
                SaveFileDialog saveFileDialog = new SaveFileDialog();
                saveFileDialog.InitialDirectory = "C:\\";
                saveFileDialog.Filter = "Pdf Files|*.pdf";
                if (saveFileDialog.ShowDialog() == DialogResult.OK)
                {
                    _isPrintToPdfInProgress = true;
                    bool isSuccessful = await webView2Control.CoreWebView2.PrintToPdfAsync(
                        saveFileDialog.FileName);
                    _isPrintToPdfInProgress = false;
                    string message = (isSuccessful) ?
                        "Print to PDF succeeded" : "Print to PDF failed";
                    MessageBox.Show(this, message, "Print To PDF Completed");
                }
                // </PrintToPdf as Portrait>
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Print to PDF Failed: " + exception.Message,
                   "Print to PDF");
            }
        }

        private async void landscapeMenuItem_Click(object sender, EventArgs e)
        {
            {
                if (_isPrintToPdfInProgress)
                {
                    MessageBox.Show(this, "Print to PDF in progress", "Print To PDF");
                    return;
                }
                try
                {
                    // <PrintToPdf as landscape>
                    CoreWebView2PrintSettings printSettings = WebViewEnvironment.CreatePrintSettings();
                    printSettings.Orientation = CoreWebView2PrintOrientation.Landscape;
                    SaveFileDialog saveFileDialog = new SaveFileDialog();
                    saveFileDialog.InitialDirectory = "C:\\";
                    saveFileDialog.Filter = "Pdf Files|*.pdf";
                    if (saveFileDialog.ShowDialog() == DialogResult.OK)
                    {
                        _isPrintToPdfInProgress = true;
                        bool isSuccessful = await webView2Control.CoreWebView2.PrintToPdfAsync(
                            saveFileDialog.FileName, printSettings);
                        _isPrintToPdfInProgress = false;
                        string message = (isSuccessful) ?
                            "Print to PDF succeeded" : "Print to PDF failed";
                        MessageBox.Show(this, message, "Print To PDF Completed");
                    }
                    // </PrintToPdf as landscape>
                }
                catch (NotImplementedException exception)
                {
                    MessageBox.Show(this, "Print to PDF Failed: " + exception.Message,
                       "Print to PDF");
                }
            }
        }

        private void exitMenuItem_Click(object sender, EventArgs e)
        {
            if (_isPrintToPdfInProgress)
            {
                var selection = MessageBox.Show(
                    "Print to PDF in progress. Continue closing?",
                    "Print to PDF", MessageBoxButtons.YesNo);
                if (selection == DialogResult.No)
                {
                    return;
                }
            }
            this.Close();
        }

        private void getUserDataFolderMenuItem_Click(object sender, EventArgs e)
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

        private void toggleVisibilityMenuItem_Click(object sender, EventArgs e)
        {
            this.webView2Control.Visible = this.toggleVisibilityMenuItem.Checked;
        }

        private void toggleCustomServerCertificateSupportMenuItem_Click(object sender, EventArgs e)
        {
            ToggleCustomServerCertificateSupport();
        }

        private void clearServerCertificateErrorActionsMenuItem_Click(object sender, EventArgs e)
        {
            ClearServerCertificateErrorActions();
        }

        private void toggleDefaultScriptDialogsMenuItem_Click(object sender, EventArgs e)
        {

            WebViewSettings.AreDefaultScriptDialogsEnabled = !WebViewSettings.AreDefaultScriptDialogsEnabled;

            MessageBox.Show("Default script dialogs will be " + (WebViewSettings.AreDefaultScriptDialogsEnabled ? "enabled" : "disabled"),"after the next navigation.");
        }

        // <ServerCertificateError>
        // When WebView2 doesn't trust a TLS certificate but host app does, this example bypasses
        // the default TLS interstitial page using the ServerCertificateErrorDetected event handler and
        // continues the request to a server. Otherwise, cancel the request.
        private bool _enableServerCertificateError = false;
        private void ToggleCustomServerCertificateSupport()
        {
            // Safeguarding the handler when unsupported runtime is used.
            try
            {
                if (!_enableServerCertificateError)
                {
                    this.webView2Control.CoreWebView2.ServerCertificateErrorDetected += WebView_ServerCertificateErrorDetected;
                }
                else
                {
                    this.webView2Control.CoreWebView2.ServerCertificateErrorDetected -= WebView_ServerCertificateErrorDetected;
                }
                _enableServerCertificateError = !_enableServerCertificateError;

                MessageBox.Show(this, "Custom server certificate support has been " +
                    (_enableServerCertificateError ? "enabled" : "disabled"),
                    "Custom server certificate support");
            }
            catch (NotImplementedException exception)
            {
                MessageBox.Show(this, "Custom server certificate support failed: " + exception.Message, "Custom server certificate support");
            }
        }

        private void WebView_ServerCertificateErrorDetected(object sender, CoreWebView2ServerCertificateErrorDetectedEventArgs e)
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
            await this.webView2Control.CoreWebView2.ClearServerCertificateErrorActionsAsync();
            MessageBox.Show(this, "message", "Clear server certificate error actions are succeeded");
        }
        // </ServerCertificateError>
        #endregion

        private void HandleResize()
        {
            // Resize the webview
            webView2Control.Size = this.ClientSize - new System.Drawing.Size(webView2Control.Location);

            // Move the Events button
            btnEvents.Left = this.ClientSize.Width - btnEvents.Width;
            // Move the Go button
            btnGo.Left = this.btnEvents.Left - btnGo.Size.Width;

            // Resize the URL textbox
            txtUrl.Width = btnGo.Left - txtUrl.Left;
        }

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
                Trace.WriteLine(e.Message);
            }
            catch (SystemException e)
            {
                Trace.WriteLine(e.Message);
            }
            return "";
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
    }
}
