// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Threading;
using System.Text;
using System.IO;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;
using System.Linq;
using System.ComponentModel;

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

        string _lastInitializeScriptId;

        List<CoreWebView2Frame> _webViewFrames = new List<CoreWebView2Frame>();
        void WebView_HandleIFrames(object sender, CoreWebView2FrameCreatedEventArgs args)
        {
            _webViewFrames.Add(args.Frame);
            args.Frame.Destroyed += WebViewFrames_DestoryedNestedIFrames;
        }

        void WebViewFrames_DestoryedNestedIFrames(object sender, object args)
        {
            try
            {
                var frameToRemove = _webViewFrames.SingleOrDefault(r => r.IsDestroyed() == 1);
                if (frameToRemove != null)
                    _webViewFrames.Remove(frameToRemove);
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show(ex.Message);
            }
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
      this.webView2Control.CoreWebView2.ProcessFailed += CoreWebView2_ProcessFailed;
      this.webView2Control.CoreWebView2.FrameCreated += WebView_HandleIFrames;

      UpdateTitleWithEvent("CoreWebView2InitializationCompleted succeeded");
      EnableButtons();
    }

    void AttachControlEventHandlers(Microsoft.Web.WebView2.WinForms.WebView2 control)
    {
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
      if (ShouldBlockUri())
      {
          webView2Control.CoreWebView2.NavigateToString("You've attempted to navigate to a domain in the blocked sites list. Press back to return to the previous page.");
      }
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

    private void createWebViewToolStripMenuItem_Click(object sender, EventArgs e)
    {
      void EnsureProcessIsClose(uint pid) {
        try
        {
          var process = Process.GetProcessById((int)pid);
          process.Kill();
        }
        catch (ArgumentException)
        {
          // Process already exited.
        }
      }
      if (this.webView2Control.CoreWebView2 != null)
      {
        var processId = this.webView2Control.CoreWebView2.BrowserProcessId;
        this.Controls.Remove(this.webView2Control);
        this.webView2Control.Dispose();
        EnsureProcessIsClose(processId);
      }
      this.webView2Control = GetReplacementControl(false);
      // Set background transparent
      this.webView2Control.DefaultBackgroundColor = System.Drawing.Color.Transparent;
      this.Controls.Add(this.webView2Control);
      HandleResize();
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

      MessageBox.Show("Default script dialogs will be " + (WebViewSettings.AreDefaultScriptDialogsEnabled ? "enabled" : "disabled"), "after the next navigation.");
    }

        private void addRemoteObjectMenuItem_Click(object sender, EventArgs e) 
        {
            try
            {
                this.webView2Control.CoreWebView2.AddHostObjectToScript("bridge", new BridgeAddRemoteObject());
            }
            catch (NotSupportedException exception)
            {
                MessageBox.Show("CoreWebView2.AddRemoteObject failed: " + exception.Message);
            }

            this.webView2Control.CoreWebView2.FrameCreated += (s, args) =>
            {
                if (args.Frame.Name.Equals("iframe_name"))
                {
                    try
                    {
                        string[] origins = new string[] { "https://appassets.example" };
                        args.Frame.AddHostObjectToScript("bridge", new BridgeAddRemoteObject(), origins);
                    }
                    catch (NotSupportedException exception)
                    {
                        MessageBox.Show("Frame.AddHostObjectToScript failed: " + exception.Message);
                    }
                }
                args.Frame.NameChanged += (nameChangedSender, nameChangedArgs) =>
                {
                    CoreWebView2Frame frame = (CoreWebView2Frame)nameChangedSender;
                    MessageBox.Show("Frame.NameChanged: " + frame.Name);
                };
                args.Frame.Destroyed += (frameDestroyedSender, frameDestroyedArgs) =>
                {
                    // Handle frame destroyed
                };
            };

            this.webView2Control.CoreWebView2.SetVirtualHostNameToFolderMapping(
                "appassets.example", "assets", CoreWebView2HostResourceAccessKind.DenyCors);
            this.webView2Control.Source = new Uri("https://appassets.example/hostObject.html");
        }

        // <DOMContentLoaded>
        private void domContentLoadedMenuItem_Click(object sender, EventArgs e)
        {
            this.webView2Control.CoreWebView2.DOMContentLoaded += WebView_DOMContentLoaded;
            this.webView2Control.CoreWebView2.FrameCreated += WebView_FrameCreatedDOMContentLoaded;
            this.webView2Control.NavigateToString(@"<!DOCTYPE html>" +
                                      "<h1>DOMContentLoaded sample page</h1>" +
                                      "<h2>The content to the iframe and below will be added after DOM content is loaded </h2>" +
                                      "<iframe style='height: 200px; width: 100%;'/>");
            this.webView2Control.CoreWebView2.NavigationCompleted += (s, args) =>
            {
                this.webView2Control.CoreWebView2.DOMContentLoaded -= WebView_DOMContentLoaded;
                this.webView2Control.CoreWebView2.FrameCreated -= WebView_FrameCreatedDOMContentLoaded;
            };
        }
        void WebView_DOMContentLoaded(object sender, CoreWebView2DOMContentLoadedEventArgs arg)
        {
            _ = this.webView2Control.ExecuteScriptAsync(
                    "let content = document.createElement(\"h2\");" +
                    "content.style.color = 'blue';" +
                    "content.textContent = \"This text was added by the host app\";" +
                    "document.body.appendChild(content);");
        }
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
        // </DOMContentLoaded>

        private void navigateWithWebResourceRequestMenuItem_Click(object sender, EventArgs e)
        {
            // <NavigateWithWebResourceRequest>
            // Prepare post data as UTF-8 byte array and convert it to stream
            // as required by the application/x-www-form-urlencoded Content-Type
            var dialog = new TextInputDialog(
                title: "NavigateWithWebResourceRequest",
                description: "Specify post data to submit to https://www.w3schools.com/action_page.php.");
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                string postDataString = "input=" + dialog.inputBox();
                UTF8Encoding utfEncoding = new UTF8Encoding();
                byte[] postData = utfEncoding.GetBytes(postDataString);
                MemoryStream postDataStream = new MemoryStream(postDataString.Length);
                postDataStream.Write(postData, 0, postData.Length);
                postDataStream.Seek(0, SeekOrigin.Begin);
                CoreWebView2WebResourceRequest webResourceRequest =
                  WebViewEnvironment.CreateWebResourceRequest(
                    "https://www.w3schools.com/action_page.php",
                    "POST",
                    postDataStream,
                    "Content-Type: application/x-www-form-urlencoded\r\n");
                this.webView2Control.CoreWebView2.NavigateWithWebResourceRequest(webResourceRequest);
            }
            // </NavigateWithWebResourceRequest>
        }

        // <WebMessage>
        private void webMessageMenuItem_Click(object sender, EventArgs e)
        {
            this.webView2Control.CoreWebView2.WebMessageReceived += WebView_WebMessageReceived;
            this.webView2Control.CoreWebView2.FrameCreated += WebView_FrameCreatedWebMessages;
            this.webView2Control.CoreWebView2.SetVirtualHostNameToFolderMapping(
                "appassets.example", "assets", CoreWebView2HostResourceAccessKind.DenyCors);
            this.webView2Control.Source = new Uri("https://appassets.example/webMessages.html");
        }

        void HandleWebMessage(CoreWebView2WebMessageReceivedEventArgs args, CoreWebView2Frame frame = null)
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
                    this.Text = message.Substring(msgLength);
                }
                else if (message == "GetWindowBounds")
                {
                    string reply = "{\"WindowBounds\":\"Left:" + 0 +
                                   "\\nTop:" + 0 +
                                   "\\nRight:" + this.webView2Control.Width +
                                   "\\nBottom:" + this.webView2Control.Height +
                                   "\"}";
                    if (frame != null) 
                    {
                        frame.PostWebMessageAsJson(reply);
                    }
                    else 
                    {
                        this.webView2Control.CoreWebView2.PostWebMessageAsJson(reply);
                    }
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
                HandleWebMessage(WebMessageReceivedArgs, args.Frame);
            };
        }
        // </WebMessageReceivedIFrame>
        // </WebMessage>
    private void toggleMuteStateMenuItem_Click(object sender, EventArgs e)
    {
      this.webView2Control.CoreWebView2.IsMuted = !this.webView2Control.CoreWebView2.IsMuted;
      MessageBox.Show("Mute state will be " + (this.webView2Control.CoreWebView2.IsMuted ? "enabled" : "disabled"), "Mute");
    }

    private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
    {
      MessageBox.Show(this, "WebView2WindowsFormsBrowser, Version 1.0\nCopyright(C) 2023", "About WebView2WindowsFormsBrowser");
    }

    void AuthenticationMenuItem_Click(object sender, EventArgs e)
    {
        // <BasicAuthenticationRequested>
        this.webView2Control.CoreWebView2.BasicAuthenticationRequested += delegate (object requestSender, CoreWebView2BasicAuthenticationRequestedEventArgs args)
        {
            // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Demo credentials in https://authenticationtest.com")]
            args.Response.UserName = "user";
            // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Demo credentials in https://authenticationtest.com")]
            args.Response.Password = "pass";
        };
        this.webView2Control.CoreWebView2.Navigate("https://authenticationtest.com/HTTPAuth");
        // </BasicAuthenticationRequested>
    }

    async void ClearBrowsingData(object target, EventArgs e, CoreWebView2BrowsingDataKinds dataKinds)
    {
        // Clear the browsing data from the last hour.
        await this.webView2Control.CoreWebView2.Profile.ClearBrowsingDataAsync(dataKinds);
        MessageBox.Show(this,
            "Completed",
            "Clear Browsing Data");
        // </ClearBrowsingData>
    }

    void WebView_ClientCertificateRequested(object sender, CoreWebView2ClientCertificateRequestedEventArgs e)
    {
        IReadOnlyList<CoreWebView2ClientCertificate> certificateList = e.MutuallyTrustedCertificates;
        if (certificateList.Count() > 0)
        {
            // There is no significance to the order, picking a certificate arbitrarily.
            e.SelectedCertificate = certificateList.LastOrDefault();
        }
        e.Handled = true;
    }

    private bool _isCustomClientCertificateSelection = false;
    void CustomClientCertificateSelectionMenuItem_Click(object sender, EventArgs e)
    {
        // Safeguarding the handler when unsupported runtime is used.
        try
        {
            if (!_isCustomClientCertificateSelection)
            {
                this.webView2Control.CoreWebView2.ClientCertificateRequested += WebView_ClientCertificateRequested;
            }
            else
            {
                this.webView2Control.CoreWebView2.ClientCertificateRequested -= WebView_ClientCertificateRequested;
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
    // <ClientCertificateRequested2>
    // This example hides the default client certificate dialog and shows a custom dialog instead.
    // The dialog box displays mutually trusted certificates list and allows the user to select a certificate.
    // Selecting `OK` will continue the request with a certificate.
    // Selecting `CANCEL` will continue the request without a certificate
    private bool _isCustomClientCertificateSelectionDialog = false;
    void DeferredCustomCertificateDialogMenuItem_Click(object sender, EventArgs e)
    {
        // Safeguarding the handler when unsupported runtime is used.
        try
        {
            if (!_isCustomClientCertificateSelectionDialog)
            {
                this.webView2Control.CoreWebView2.ClientCertificateRequested += delegate (
                    object requestSender, CoreWebView2ClientCertificateRequestedEventArgs args)
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
                                if (dialog.ShowDialog() == DialogResult.OK)
                                {
                                    // Continue with the selected certificate to respond to the server if `OK` is selected.
                                    args.SelectedCertificate = (CoreWebView2ClientCertificate)dialog.CertificateDataBinding.SelectedItems[0].Tag;
                                }
                            }
                            args.Handled = true;
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

    async void GetCookiesMenuItem_Click(object sender, EventArgs e, string address)
    {
        // <GetCookies>
        List<CoreWebView2Cookie> cookieList = await this.webView2Control.CoreWebView2.CookieManager.GetCookiesAsync(address);
        StringBuilder cookieResult = new StringBuilder(cookieList.Count + " cookie(s) received from " + address);
        for (int i = 0; i < cookieList.Count; ++i)
        {
            CoreWebView2Cookie cookie = this.webView2Control.CoreWebView2.CookieManager.CreateCookieWithSystemNetCookie(cookieList[i].ToSystemNetCookie());
            cookieResult.Append($"\n{cookie.Name} {cookie.Value} {(cookie.IsSession ? "[session cookie]" : cookie.Expires.ToString("G"))}");
        }
        MessageBox.Show(this, cookieResult.ToString(), "GetCookiesAsync");
        // </GetCookies>
    }

    void AddOrUpdateCookieMenuItem_Click(object sender, EventArgs e, string domain)
    {
        // <AddOrUpdateCookie>
        CoreWebView2Cookie cookie = this.webView2Control.CoreWebView2.CookieManager.CreateCookie("CookieName", "CookieValue", domain, "/");
        this.webView2Control.CoreWebView2.CookieManager.AddOrUpdateCookie(cookie);
        // </AddOrUpdateCookie>
    }

    void DeleteAllCookiesMenuItem_Click(object sender, EventArgs e)
    {
        this.webView2Control.CoreWebView2.CookieManager.DeleteAllCookies();
    }

    void DeleteCookiesMenuItem_Click(object sender, EventArgs e, string domain)
    {
        this.webView2Control.CoreWebView2.CookieManager.DeleteCookiesWithDomainAndPath("CookieName", domain, "/");
    }

    private void showBrowserProcessInfoMenuItem_Click(object sender, EventArgs e)
    {
      var browserInfo = this.webView2Control.CoreWebView2.BrowserProcessId;
      MessageBox.Show(this, "Browser ID: " + browserInfo.ToString(), "Process ID");
    }

    private void showPerformanceInfoMenuItem_Click(object sender, EventArgs e)
    {
      var processInfoList = WebViewEnvironment.GetProcessInfos();
      var processListCount = processInfoList.Count;
      string message = "";
      if (processListCount == 0)
      {
        message = "No process found.";
      }
      else
      {
        message = $"{processListCount} processes found:\n\n";
        for (int i = 0; i < processListCount; ++i)
        {
          int processId = processInfoList[i].ProcessId;
          CoreWebView2ProcessKind processKind = processInfoList[i].Kind;
          var proc = Process.GetProcessById(processId);
          var memoryInBytes = proc.PrivateMemorySize64;
          var b2kb = memoryInBytes / 1024;
          message += $"Process ID: {processId}, Process Kind: {processKind}, Memory Usage: {b2kb} KB\n";
        }
      }
      MessageBox.Show(this, message, "Process Info");
    }
    // <ProcessFailed>
    // Register a handler for the ProcessFailed event.
    // This handler checks the failure kind and tries to:
    //   * Recreate the webview for browser failure and render unresponsive.
    //   * Reload the webview for render failure.
    //   * Reload the webview for frame-only render failure impacting app content.
    //   * Log information about the failure for other failures.
    private void CoreWebView2_ProcessFailed(object sender, CoreWebView2ProcessFailedEventArgs e)
    {
      void ReinitIfSelectedByUser(string caption, string message)
      {
        this.webView2Control.BeginInvoke(new Action(() =>
        {
          var selection = MessageBox.Show(this, message, caption, MessageBoxButtons.YesNo);
          if (selection == DialogResult.Yes)
          {
            this.Controls.Remove(this.webView2Control);
            this.webView2Control.Dispose();
            this.webView2Control = GetReplacementControl(false);
            // Set background transparent
            this.webView2Control.DefaultBackgroundColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.webView2Control);
            HandleResize();
          }
        }));
      }

      void ReloadIfSelectedByUser(string caption, string message)
      {
        this.webView2Control.BeginInvoke(new Action(() =>
        {
          var selection = MessageBox.Show(this, message, caption, MessageBoxButtons.YesNo);
          if (selection == DialogResult.Yes)
          {
            this.webView2Control.CoreWebView2.Reload();
          }
        }));
      }

      this.webView2Control.Invoke(new Action(() =>
      {
        StringBuilder messageBuilder = new StringBuilder();
        messageBuilder.AppendLine($"Process kind: {e.ProcessFailedKind}");
        messageBuilder.AppendLine($"Reason: {e.Reason}");
        messageBuilder.AppendLine($"Exit code: {e.ExitCode}");
        messageBuilder.AppendLine($"Process description: {e.ProcessDescription}");
        MessageBox.Show(messageBuilder.ToString(), "Child process failed", MessageBoxButtons.OK);
      }));

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
    WebView2 GetReplacementControl(bool useNewEnvironment)
    {
      WebView2 webView = this.webView2Control;
      WebView2 replacementControl = new WebView2();
      ((System.ComponentModel.ISupportInitialize)(replacementControl)).BeginInit();
      // Setup properties.
      if (useNewEnvironment)
      {
        // Create a new CoreWebView2CreationProperties instance so the environment
        // is made anew.
        replacementControl.CreationProperties = new CoreWebView2CreationProperties();
        replacementControl.CreationProperties.BrowserExecutableFolder = webView.CreationProperties.BrowserExecutableFolder;
        replacementControl.CreationProperties.Language = webView.CreationProperties.Language;
        replacementControl.CreationProperties.UserDataFolder = webView.CreationProperties.UserDataFolder;
        replacementControl.CreationProperties.AdditionalBrowserArguments = webView.CreationProperties.AdditionalBrowserArguments;
      }
      else
      {
        replacementControl.CreationProperties = webView.CreationProperties;
      }
      AttachControlEventHandlers(replacementControl);
      replacementControl.Source = webView.Source ?? new Uri("https://www.bing.com");
      ((System.ComponentModel.ISupportInitialize)(replacementControl)).EndInit();

      return replacementControl;
    }
    // </ProcessFailed>

    // Crash the browser's process on command, to test crash handlers.
    private void crashBrowserProcessMenuItem_Click(object sender, EventArgs e)
    {
      this.webView2Control.CoreWebView2.Navigate("edge://inducebrowsercrashforrealz");
    }

    // Crash the browser's render process on command, to test crash handlers.
    private void crashRendererProcessMenuItem_Click(object sender, EventArgs e)
    {
      this.webView2Control.CoreWebView2.Navigate("edge://kill");
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

        // Prompt the user for a list of blocked domains
        private bool _blockedSitesSet = false;
        private HashSet<string> _blockedSitesList = new HashSet<string>();
        private void blockedDomainsMenuItem_Click(object sender, EventArgs e)
        {
            var blockedSitesString = "";
            if (_blockedSitesSet)
            {
                blockedSitesString = String.Join(";", _blockedSitesList);
            }
            else
            {
                blockedSitesString = "foo.com;bar.org";
            }
            var textDialog = new TextInputDialog(
                title: "Blocked Domains",
                description: "Enter hostnames to block, sparately by semicolons",
                defaultInput: blockedSitesString);
            if (textDialog.ShowDialog() == DialogResult.OK)
            {
                _blockedSitesSet = true;
                _blockedSitesList.Clear();
                if (textDialog.inputBox() != null)
                {
                    string[] textcontent = textDialog.inputBox().Split(';');
                    foreach (string site in textcontent)
                    {
                        _blockedSitesList.Add(site);
                    }
                }
            }

        }

        // Check the URI against the blocked sites list
        private bool ShouldBlockUri()
        {
            foreach (string site in _blockedSitesList)
            {
                if (site.Equals(txtUrl.Text))
                {
                    return true;
                }
            }
            return false;
        }

        private async void injectScriptMenuItem_Click(object sender, EventArgs e)
        {
            // <ExecuteScript>
            var dialog = new TextInputDialog(
                title: "Inject Script",
                description: "Enter some JavaScript to be executed in the context of this page.",
                defaultInput: "window.getComputedStyle(document.body).backgroundColor");
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    string scriptResult = await this.webView2Control.ExecuteScriptAsync(dialog.inputBox());
                    MessageBox.Show(this, scriptResult, "Script Result");
                }
                catch (InvalidOperationException ex)
                {
                    MessageBox.Show(this, ex.Message, "Execute Script Fails");
                }
            }
            // </ExecuteScript>
        }

        private async void injectScriptIntoFrameMenuItem_Click(object sender, EventArgs e)
        {
            // <ExecuteScriptFrame>
            string iframesData = WebViewFrames_ToString();
            string iframesInfo = "Enter iframe to run the JavaScript code in.\r\nAvailable iframes: " + iframesData;
            var dialogIFrames = new TextInputDialog(
                title: "Inject Script Into IFrame",
                description: iframesInfo,
                defaultInput: "0");
            if (dialogIFrames.ShowDialog() == DialogResult.OK)
            {
                int iframeNumber = -1;
                try
                {
                    iframeNumber = Int32.Parse(dialogIFrames.inputBox());
                }
                catch (FormatException)
                {
                    Console.WriteLine("Can not convert " + dialogIFrames.inputBox() + " to int");
                }
                if (iframeNumber >= 0 && iframeNumber < _webViewFrames.Count)
                {
                    var dialog = new TextInputDialog(
                        title: "Inject Script",
                        description: "Enter some JavaScript to be executed in the context of iframe " + dialogIFrames.inputBox(),
                        defaultInput: "window.getComputedStyle(document.body).backgroundColor");
                    if (dialog.ShowDialog() == DialogResult.OK)
                    {
                        try
                        {
                            string scriptResult = await _webViewFrames[iframeNumber].ExecuteScriptAsync(dialog.inputBox());
                            MessageBox.Show(this, scriptResult, "Script Result");
                        }
                        catch (InvalidOperationException ex)
                        {
                            MessageBox.Show(this, ex.Message, "Execute Script Frame Fails");
                        }
                    }
                }
                else
                {
                    MessageBox.Show("Can not read frame index or it is out of available range");
                }
            }
            // </ExecuteScriptFrame>
        }

        // Prompt the user for some scripts and register it to execute whenever a new page loads.
        private async void addInitializeScriptMenuItem_Click(object sender, EventArgs e)
        {
            TextInputDialog dialog = new TextInputDialog(
              title: "Add Initialize Script",
              description: "Enter the JavaScript code to run as the initialization script that runs before any script in the HTML document.",
              // This example script stops child frames from opening new windows.  Because
              // the initialization script runs before any script in the HTML document, we
              // can trust the results of our checks on window.parent and window.top.
              defaultInput: "if (window.parent !== window.top) {\r\n" +
              "    delete window.open;\r\n" +
              "}");
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    string scriptId = await this.webView2Control.CoreWebView2.AddScriptToExecuteOnDocumentCreatedAsync(dialog.inputBox());
                    _lastInitializeScriptId = scriptId;
                    MessageBox.Show(this, scriptId, "AddScriptToExecuteOnDocumentCreated Id");
                }
                catch (Exception ex)
                {
                    MessageBox.Show(this, ex.ToString(), "AddScriptToExecuteOnDocumentCreated failed");
                }
            }
        }

        // Prompt the user for an initialization script ID and deregister that script.
        private void removeInitializeScriptMenuItem_Click(object sender, EventArgs e)
        {
            TextInputDialog dialog = new TextInputDialog(
              title: "Remove Initialize Script",
              description: "Enter the ID created from Add Initialize Script.",
              defaultInput: _lastInitializeScriptId);
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                string scriptId = dialog.inputBox();
                // check valid
                try
                {
                    Int64 result = Int64.Parse(scriptId);
                    Int64 lastId = Int64.Parse(_lastInitializeScriptId);
                    if (result > lastId)
                    {
                        MessageBox.Show(this, scriptId, "Invalid ScriptId, should be less or equal than " + _lastInitializeScriptId);
                    }
                    else
                    {
                        this.webView2Control.CoreWebView2.RemoveScriptToExecuteOnDocumentCreated(scriptId);
                        if (result == lastId && lastId >= 2)
                        {
                            _lastInitializeScriptId = (lastId - 1).ToString();
                        }
                        MessageBox.Show(this, scriptId, "RemoveScriptToExecuteOnDocumentCreated Id");
                    }
                }
                catch (FormatException)
                {
                    MessageBox.Show(this, scriptId, "Invalid ScriptId, should be Integer");

                }
            }
        }

        // Prompt the user for a string and then post it as a web message.
        private void postMessageStringMenuItem_Click(object sender, EventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Post Web Message String",
                description: "Web message string:\r\nEnter the web message as a string.");

            try
            {
                if (dialog.ShowDialog() == DialogResult.OK)
                {
                    this.webView2Control.CoreWebView2.PostWebMessageAsString(dialog.inputBox());
                }
            }
            catch (Exception exception)
            {
                MessageBox.Show(this, "PostMessageAsString Failed: " + exception.Message,
                   "Post Message As String");
            }
        }

        // Prompt the user for some JSON and then post it as a web message.
        private void postMessageJsonMenuItem_Click(object sender, EventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Post Web Message JSON",
                description: "Web message JSON:\r\nEnter the web message as JSON.",
                 defaultInput: "{\"SetColor\":\"blue\"}");

            try
            {
                if (dialog.ShowDialog() == DialogResult.OK)
                {
                    this.webView2Control.CoreWebView2.PostWebMessageAsJson(dialog.inputBox());
                }
            }
            catch (Exception exception)
            {
                MessageBox.Show(this, "PostMessageAsJSON Failed: " + exception.Message,
                   "Post Message As JSON");
            }
        }

        // Prompt the user for a string and then post it as a web message to the first iframe.
        private void postMessageStringIframeMenuItem_Click(object sender, EventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Post Web Message String Iframe",
                description: "Web message string:\r\nEnter the web message as a string.");

            try
            {
                if (dialog.ShowDialog() == DialogResult.OK)
                {
                    if (_webViewFrames.Count != 0)
                    {
                        _webViewFrames[0].PostWebMessageAsString(dialog.inputBox());
                    }
                    else
                    {
                        MessageBox.Show("No iframes found");
                    }
                }

            }
            catch (Exception exception)
            {
                MessageBox.Show(this, "PostMessageAsStringIframe Failed: " + exception.Message,
                   "Post Message As String");
            }
        }

        // Prompt the user for some JSON and then post it as a web message to the first iframe.
        private void postMessageJsonIframeMenuItem_Click(object sender, EventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Post Web Message JSON Iframe",
                description: "Web message JSON:\r\nEnter the web message as JSON.",
                 defaultInput: "{\"SetColor\":\"blue\"}");

            try
            {
                if (dialog.ShowDialog() == DialogResult.OK)
                {
                     if (_webViewFrames.Count != 0)
                    {
                        _webViewFrames[0].PostWebMessageAsJson(dialog.inputBox());
                    }
                    else
                    {
                        MessageBox.Show("No iframes found");
                    }
                }
            }
            catch (Exception exception)
            {
                MessageBox.Show(this, "PostMessageAsJSONIframe Failed: " + exception.Message,
                   "Post Message As JSON");
            }
        }
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
            return e.Message;
        }
        catch (InvalidOperationException e)
        {
            return e.Message;
        }
        // Occurred when a 32-bit process wants to access the modules of a 64-bit process.
        catch (Win32Exception e)
        {
            return e.Message;
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
  }
}
