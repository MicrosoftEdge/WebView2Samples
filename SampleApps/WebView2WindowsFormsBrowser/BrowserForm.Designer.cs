// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;

namespace WebView2WindowsFormsBrowser
{
    partial class BrowserForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btnEvents = new System.Windows.Forms.Button();
            this.btnBack = new System.Windows.Forms.Button();
            this.btnForward = new System.Windows.Forms.Button();
            this.btnRefresh = new System.Windows.Forms.Button();
            this.btnStop = new System.Windows.Forms.Button();
            this.btnGo = new System.Windows.Forms.Button();
            this.txtUrl = new System.Windows.Forms.TextBox();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.windowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.closeWebViewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.createWebViewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.createNewWindowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.createNewWindowWithOptionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.createNewThreadToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.controlToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.acceleratorKeysEnabledToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.scriptToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.zoomToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.xToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.xToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.xToolStripMenuItem2 = new System.Windows.Forms.ToolStripMenuItem();
            this.xToolStripMenuItem3 = new System.Windows.Forms.ToolStripMenuItem();
            this.backgroundColorMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.blueBackgroundColorMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.redBackgroundColorMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.whiteBackgroundColorMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.taskManagerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.methodCDPToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.transparentBackgroundColorMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.allowExternalDropMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.setUsersAgentMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.getDocumentTitleMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.getUserDataFolderMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.printToPDFMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.portraitMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.landscapeMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toggleVisibilityMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.serverCertificateErrorMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toggleCustomServerCertificateSupportMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.clearServerCertificateErrorActionsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toggleDefaultScriptDialogsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.scenarioToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.addRemoteObjectMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.domContentLoadedMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.navigateWithWebResourceRequestMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.webMessageMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.processToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showBrowserProcessInfoMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.crashBrowserProcessMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.crashRendererProcessMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showPerformanceInfoMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.audioToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toggleMuteStateMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.blockedDomainsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.injectScriptMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.injectScriptIntoFrameMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.addInitializeScriptMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.removeInitializeScriptMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.webViewLogoBitmap = new System.Drawing.Bitmap(@"assets\AppStartPageBackground.png");
            this.webView2Control = new Microsoft.Web.WebView2.WinForms.WebView2();
            this.menuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.webView2Control)).BeginInit();
            this.SuspendLayout();
            //
            // btnEvents
            //
            this.btnEvents.Location = new System.Drawing.Point(1576, 48);
            this.btnEvents.Margin = new System.Windows.Forms.Padding(6);
            this.btnEvents.Name = "btnEvents";
            this.btnEvents.Size = new System.Drawing.Size(150, 44);
            this.btnEvents.TabIndex = 6;
            this.btnEvents.Text = "Events";
            this.btnEvents.UseVisualStyleBackColor = true;
            this.btnEvents.Click += new System.EventHandler(this.btnEvents_Click);
            //
            // btnBack
            //
            this.btnBack.Enabled = false;
            this.btnBack.Location = new System.Drawing.Point(24, 48);
            this.btnBack.Margin = new System.Windows.Forms.Padding(6);
            this.btnBack.Name = "btnBack";
            this.btnBack.Size = new System.Drawing.Size(150, 44);
            this.btnBack.TabIndex = 0;
            this.btnBack.Text = "Back";
            this.btnBack.UseVisualStyleBackColor = true;
            this.btnBack.Click += new System.EventHandler(this.btnBack_Click);
            //
            // btnForward
            //
            this.btnForward.Enabled = false;
            this.btnForward.Location = new System.Drawing.Point(186, 48);
            this.btnForward.Margin = new System.Windows.Forms.Padding(6);
            this.btnForward.Name = "btnForward";
            this.btnForward.Size = new System.Drawing.Size(150, 44);
            this.btnForward.TabIndex = 1;
            this.btnForward.Text = "Forward";
            this.btnForward.UseVisualStyleBackColor = true;
            this.btnForward.Click += new System.EventHandler(this.btnForward_Click);
            //
            // btnRefresh
            //
            this.btnRefresh.Location = new System.Drawing.Point(348, 48);
            this.btnRefresh.Margin = new System.Windows.Forms.Padding(6);
            this.btnRefresh.Name = "btnRefresh";
            this.btnRefresh.Size = new System.Drawing.Size(150, 44);
            this.btnRefresh.TabIndex = 2;
            this.btnRefresh.Text = "Reload";
            this.btnRefresh.UseVisualStyleBackColor = true;
            this.btnRefresh.Click += new System.EventHandler(this.BtnRefresh_Click);
            //
            // btnStop
            //
            this.btnStop.Location = new System.Drawing.Point(510, 48);
            this.btnStop.Margin = new System.Windows.Forms.Padding(6);
            this.btnStop.Name = "btnStop";
            this.btnStop.Size = new System.Drawing.Size(150, 44);
            this.btnStop.TabIndex = 3;
            this.btnStop.Text = "Cancel";
            this.btnStop.UseVisualStyleBackColor = true;
            //
            // btnGo
            //
            this.btnGo.Location = new System.Drawing.Point(1426, 48);
            this.btnGo.Margin = new System.Windows.Forms.Padding(6);
            this.btnGo.Name = "btnGo";
            this.btnGo.Size = new System.Drawing.Size(150, 44);
            this.btnGo.TabIndex = 5;
            this.btnGo.Text = "Go";
            this.btnGo.UseVisualStyleBackColor = true;
            this.btnGo.Click += new System.EventHandler(this.BtnGo_Click);
            //
            // txtUrl
            //
            this.txtUrl.Location = new System.Drawing.Point(672, 48);
            this.txtUrl.Margin = new System.Windows.Forms.Padding(6);
            this.txtUrl.Name = "txtUrl";
            this.txtUrl.Size = new System.Drawing.Size(738, 31);
            this.txtUrl.TabIndex = 4;
            this.txtUrl.Text = "https://www.bing.com/";
            //
            // menuStrip1
            //
            this.menuStrip1.ImageScalingSize = new System.Drawing.Size(32, 32);
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.processToolStripMenuItem,
            this.windowToolStripMenuItem,
            this.controlToolStripMenuItem,
            this.viewToolStripMenuItem,
            this.scriptToolStripMenuItem,
            this.scenarioToolStripMenuItem,
            this.audioToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(1580, 42);
            this.menuStrip1.TabIndex = 7;
            this.menuStrip1.Text = "menuStrip1";
            //
            // fileToolStripMenuItem
            //
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.printToPDFMenuItem,this.getDocumentTitleMenuItem,this.getUserDataFolderMenuItem,this.exitMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.fileToolStripMenuItem.Text = "File";
            //
            // getDocumentTitleMenuItem
            //
            this.getDocumentTitleMenuItem.Name = "getDocumentTitleMenuItem";
            this.getDocumentTitleMenuItem.Size = new System.Drawing.Size(359, 44);
            this.getDocumentTitleMenuItem.Text = "Get Document Title";
            this.getDocumentTitleMenuItem.Click += new System.EventHandler(this.getDocumentTitleMenuItem_Click);
            //
            // exitMenuItem
            //
            this.exitMenuItem.Name = "exitMenuItem";
            this.exitMenuItem.Size = new System.Drawing.Size(359, 44);
            this.exitMenuItem.Text = "Exit";
            this.exitMenuItem.Click += new System.EventHandler(this.exitMenuItem_Click);
            //
            // getUserDataFolderMenuItem
            //
            this.getUserDataFolderMenuItem.Name = "getUserDataFolderMenuItem";
            this.getUserDataFolderMenuItem.Size = new System.Drawing.Size(359, 44);
            this.getUserDataFolderMenuItem.Text = "Get User Data Folder";
            this.getUserDataFolderMenuItem.Click += new System.EventHandler(this.getUserDataFolderMenuItem_Click);
            //
            // printToPDFMenuItem
            //
            this.printToPDFMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.portraitMenuItem, this.landscapeMenuItem});
            this.printToPDFMenuItem.Name = "printToPDFMenuItem";
            this.printToPDFMenuItem.Size = new System.Drawing.Size(359, 44);
            this.printToPDFMenuItem.Text = "Print to PDF";
            //
            // portraitMenuItem
            //
            this.portraitMenuItem.Name = "portraitMenuItem";
            this.portraitMenuItem.Size = new System.Drawing.Size(359, 44);
            this.portraitMenuItem.Text = "Portrait";
            this.portraitMenuItem.Click += new System.EventHandler(this.portraitMenuItem_Click);
            //
            // landscapeMenuItem
            //
            this.landscapeMenuItem.Name = "landscapeMenuItem";
            this.landscapeMenuItem.Size = new System.Drawing.Size(359, 44);
            this.landscapeMenuItem.Text = "Landscape";
            this.landscapeMenuItem.Click += new System.EventHandler(this.landscapeMenuItem_Click);
            //
            // windowToolStripMenuItem
            //
            this.windowToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
              this.closeWebViewToolStripMenuItem,
              this.createWebViewToolStripMenuItem,
              this.createNewWindowToolStripMenuItem,
              this.createNewWindowWithOptionsToolStripMenuItem,
              this.createNewThreadToolStripMenuItem});
            this.windowToolStripMenuItem.Name = "windowToolStripMenuItem";
            this.windowToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.windowToolStripMenuItem.Text = "Window";
            //
            // closeWebViewToolStripMenuItem
            //
            this.closeWebViewToolStripMenuItem.Name = "closeWebViewToolStripMenuItem";
            this.closeWebViewToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.closeWebViewToolStripMenuItem.Text = "Close WebView";
            this.closeWebViewToolStripMenuItem.Click += new System.EventHandler(this.closeWebViewToolStripMenuItem_Click);
            //
            // createWebViewToolStripMenuItem
            //
            this.createWebViewToolStripMenuItem.Name = "createWebViewToolStripMenuItem";
            this.createWebViewToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.createWebViewToolStripMenuItem.Text = "Create WebView";
            this.createWebViewToolStripMenuItem.Click += new System.EventHandler(this.createWebViewToolStripMenuItem_Click);
            //
            // createNewWindowToolStripMenuItem
            //
            this.createNewWindowToolStripMenuItem.Name = "createNewWindowToolStripMenuItem";
            this.createNewWindowToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.createNewWindowToolStripMenuItem.Text = "Create New Window";
            this.createNewWindowToolStripMenuItem.Click += new System.EventHandler(this.createNewWindowToolStripMenuItem_Click);
            //
            // createNewWindowWithOptionsToolStripMenuItem
            //
            this.createNewWindowWithOptionsToolStripMenuItem.Name = "createNewWindowWithOptionsToolStripMenuItem";
            this.createNewWindowWithOptionsToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.createNewWindowWithOptionsToolStripMenuItem.Text = "Create New Window With Options";
            this.createNewWindowWithOptionsToolStripMenuItem.Click += new System.EventHandler(this.createNewWindowWithOptionsToolStripMenuItem_Click);
            //
            // createNewThreadToolStripMenuItem
            //
            this.createNewThreadToolStripMenuItem.Name = "createNewThreadToolStripMenuItem";
            this.createNewThreadToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.createNewThreadToolStripMenuItem.Text = "Create New Thread";
            this.createNewThreadToolStripMenuItem.Click += new System.EventHandler(this.createNewThreadToolStripMenuItem_Click);
            //
            // controlToolStripMenuItem
            //
            this.controlToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.acceleratorKeysEnabledToolStripMenuItem,
                this.allowExternalDropMenuItem,
                this.serverCertificateErrorMenuItem,
                this.setUsersAgentMenuItem,
                this.toggleDefaultScriptDialogsMenuItem});
            this.controlToolStripMenuItem.Name = "controlToolStripMenuItem";
            this.controlToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.controlToolStripMenuItem.Text = "Settings";
            //
            // acceleratorKeysEnabledToolStripMenuItem
            //
            this.acceleratorKeysEnabledToolStripMenuItem.Name = "acceleratorKeysEnabledToolStripMenuItem";
            this.acceleratorKeysEnabledToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.acceleratorKeysEnabledToolStripMenuItem.Text = "Toggle AcceleratorKeys";
            this.acceleratorKeysEnabledToolStripMenuItem.Checked = true;
            this.acceleratorKeysEnabledToolStripMenuItem.CheckOnClick = true;
            //
            // allowExternalDropMenuItem
            //
            this.allowExternalDropMenuItem.Name = "allowExternalDropMenuItem";
            this.allowExternalDropMenuItem.Size = new System.Drawing.Size(359, 44);
            this.allowExternalDropMenuItem.Text = "Toggle AllowExternalDrop";
            this.allowExternalDropMenuItem.Checked = true;
            this.allowExternalDropMenuItem.CheckOnClick = true;
            this.allowExternalDropMenuItem.Click += new System.EventHandler(this.allowExternalDropMenuItem_Click);
            //
            // serverCertificateErrorMenuItem
            //
            this.serverCertificateErrorMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.toggleCustomServerCertificateSupportMenuItem,
                this.clearServerCertificateErrorActionsMenuItem});
            this.serverCertificateErrorMenuItem.Name = "serverCertificateErrorMenuItem";
            this.serverCertificateErrorMenuItem.Size = new System.Drawing.Size(359, 44);
            this.serverCertificateErrorMenuItem.Text = "Server Certificate Error";
            //
            // setUsersAgentMenuItem
            //
            this.setUsersAgentMenuItem.Name = "setUsersAgentMenuItem";
            this.setUsersAgentMenuItem.Size = new System.Drawing.Size(359, 44);
            this.setUsersAgentMenuItem.Text = "Set Users Agent";
            this.setUsersAgentMenuItem.Click += new System.EventHandler(this.setUsersAgentMenuItem_Click);
            //
            // toggleCustomServerCertificateSupportMenuItem
            //
            this.toggleCustomServerCertificateSupportMenuItem.Name = "toggleCustomServerCertificateSupportMenuItem";
            this.toggleCustomServerCertificateSupportMenuItem.Size = new System.Drawing.Size(359, 44);
            this.toggleCustomServerCertificateSupportMenuItem.Text = "Toggle Custom Server Certificate Support";
            this.toggleCustomServerCertificateSupportMenuItem.Click += new System.EventHandler(this.toggleCustomServerCertificateSupportMenuItem_Click);
            //
            // clearServerCertificateErrorActionsMenuItem
            //
            this.clearServerCertificateErrorActionsMenuItem.Name = "clearServerCertificateErrorActionsMenuItem";
            this.clearServerCertificateErrorActionsMenuItem.Size = new System.Drawing.Size(359, 44);
            this.clearServerCertificateErrorActionsMenuItem.Text = "Clear Server Certificate Error Actions";
            this.clearServerCertificateErrorActionsMenuItem.Click += new System.EventHandler(this.clearServerCertificateErrorActionsMenuItem_Click);
            //
            // toggleDefaultScriptDialogsMenuItem
            //
            this.toggleDefaultScriptDialogsMenuItem.Name = "toggleDefaultScriptDialogsMenuItem";
            this.toggleDefaultScriptDialogsMenuItem.Size = new System.Drawing.Size(359, 44);
            this.toggleDefaultScriptDialogsMenuItem.Text = "Toggle Default Script Dialogs";
            this.toggleDefaultScriptDialogsMenuItem.Click += new System.EventHandler(this.toggleDefaultScriptDialogsMenuItem_Click);
            //
            // blockedDomainsMenuItem
            //
            this.blockedDomainsMenuItem.Name = "blockedDomainsMenuItem";
            this.blockedDomainsMenuItem.Size = new System.Drawing.Size(359, 44);
            this.blockedDomainsMenuItem.Text = "Blocked Domains";
            this.blockedDomainsMenuItem.Click += new System.EventHandler(this.blockedDomainsMenuItem_Click);
            //
            // viewToolStripMenuItem
            //
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.toggleVisibilityMenuItem, this.zoomToolStripMenuItem, this.backgroundColorMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            this.viewToolStripMenuItem.Size = new System.Drawing.Size(86, 38);
            this.viewToolStripMenuItem.Text = "View";
            //
            // scriptToolStripMenuItem
            //
            this.scriptToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.injectScriptMenuItem,
                this.injectScriptIntoFrameMenuItem,
                this.methodCDPToolStripMenuItem, 
                this.taskManagerToolStripMenuItem,
                this.addInitializeScriptMenuItem,
                this.removeInitializeScriptMenuItem});
            this.scriptToolStripMenuItem.Name = "scriptToolStripMenuItem";
            this.scriptToolStripMenuItem.Size = new System.Drawing.Size(86, 38);
            this.scriptToolStripMenuItem.Text = "Script";
            //
            // toggleVisibilityMenuItem
            //
            this.toggleVisibilityMenuItem.Name = "toggleVisibilityMenuItem";
            this.toggleVisibilityMenuItem.Size = new System.Drawing.Size(359, 44);
            this.toggleVisibilityMenuItem.Text = "Toggle Visibility";
            this.toggleVisibilityMenuItem.Checked = true;
            this.toggleVisibilityMenuItem.CheckOnClick = true;
            this.toggleVisibilityMenuItem.Click += new System.EventHandler(this.toggleVisibilityMenuItem_Click);
            //
            // zoomToolStripMenuItem
            //
            this.zoomToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.xToolStripMenuItem,
            this.xToolStripMenuItem1,
            this.xToolStripMenuItem2,
            this.xToolStripMenuItem3});
            this.zoomToolStripMenuItem.Name = "zoomToolStripMenuItem";
            this.zoomToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.zoomToolStripMenuItem.Text = "Zoom";
            //
            // xToolStripMenuItem
            //
            this.xToolStripMenuItem.Name = "xToolStripMenuItem";
            this.xToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.xToolStripMenuItem.Text = "0.5x";
            this.xToolStripMenuItem.Click += new System.EventHandler(this.xToolStripMenuItem05_Click);
            //
            // xToolStripMenuItem1
            //
            this.xToolStripMenuItem1.Name = "xToolStripMenuItem1";
            this.xToolStripMenuItem1.Size = new System.Drawing.Size(359, 44);
            this.xToolStripMenuItem1.Text = "1.0x";
            this.xToolStripMenuItem1.Click += new System.EventHandler(this.xToolStripMenuItem1_Click);
            //
            // xToolStripMenuItem2
            //
            this.xToolStripMenuItem2.Name = "xToolStripMenuItem2";
            this.xToolStripMenuItem2.Size = new System.Drawing.Size(359, 44);
            this.xToolStripMenuItem2.Text = "2.0x";
            this.xToolStripMenuItem2.Click += new System.EventHandler(this.xToolStripMenuItem2_Click);
            //
            // xToolStripMenuItem3
            //
            this.xToolStripMenuItem3.Name = "xToolStripMenuItem3";
            this.xToolStripMenuItem3.Size = new System.Drawing.Size(359, 44);
            this.xToolStripMenuItem3.Text = "Get ZoomFactor";
            this.xToolStripMenuItem3.Click += new System.EventHandler(this.xToolStripMenuItem3_Click);
            //
            // backgroundColorToolStripMenuItem
            //
            this.backgroundColorMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.whiteBackgroundColorMenuItem,
                this.redBackgroundColorMenuItem,
                this.blueBackgroundColorMenuItem,
                this.transparentBackgroundColorMenuItem});
            this.backgroundColorMenuItem.Name = "backgroundColorMenuItem";
            this.backgroundColorMenuItem.Size = new System.Drawing.Size(359, 44);
            this.backgroundColorMenuItem.Text = "Background Color";
            //
            // whiteBackgroundColorMenuItem
            //
            this.whiteBackgroundColorMenuItem.Name = "whiteBackgroundColorMenuItem";
            this.whiteBackgroundColorMenuItem.Size = new System.Drawing.Size(359, 44);
            this.whiteBackgroundColorMenuItem.Text = "White";
            this.whiteBackgroundColorMenuItem.Click += new System.EventHandler(this.backgroundColorMenuItem_Click);
            //
            // redBackgroundColorMenuItem
            //
            this.redBackgroundColorMenuItem.Name = "redBackgroundColorMenuItem";
            this.redBackgroundColorMenuItem.Size = new System.Drawing.Size(359, 44);
            this.redBackgroundColorMenuItem.Text = "Red";
            this.redBackgroundColorMenuItem.Click += new System.EventHandler(this.backgroundColorMenuItem_Click);
            //
            // blueBackgroundColorMenuItem
            //
            this.blueBackgroundColorMenuItem.Name = "blueBackgroundColorMenuItem";
            this.blueBackgroundColorMenuItem.Size = new System.Drawing.Size(359, 44);
            this.blueBackgroundColorMenuItem.Text = "Blue";
            this.blueBackgroundColorMenuItem.Click += new System.EventHandler(this.backgroundColorMenuItem_Click);
            //
            // methodCDPToolStripMenuItem
            //
            this.methodCDPToolStripMenuItem.Name = "methodCDPToolStripMenuItem";
            this.methodCDPToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.methodCDPToolStripMenuItem.Text = "Call CDP Method";
            this.methodCDPToolStripMenuItem.Click += new System.EventHandler(this.methodCDPToolStripMenuItem_Click);
            //
            // taskManagerToolStripMenuItem
            //
            this.taskManagerToolStripMenuItem.Name = "methodCDPToolStripMenuItem";
            this.taskManagerToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.taskManagerToolStripMenuItem.Text = "Open Task Manager";
            this.taskManagerToolStripMenuItem.Click += new System.EventHandler(this.taskManagerToolStripMenuItem_Click);
            //
            // injectScriptMenuItem
            //
            this.injectScriptMenuItem.Name = "injectScriptMenuItem";
            this.injectScriptMenuItem.Size = new System.Drawing.Size(359, 44);
            this.injectScriptMenuItem.Text = "Inject Script";
            this.injectScriptMenuItem.Click += new System.EventHandler(this.injectScriptMenuItem_Click);
            //
            // injectScriptIntoFrameMenuItem
            //
            this.injectScriptIntoFrameMenuItem.Name = "injectScriptIntoFrameMenuItem";
            this.injectScriptIntoFrameMenuItem.Size = new System.Drawing.Size(359, 44);
            this.injectScriptIntoFrameMenuItem.Text = "Inject Script Into Frame";
            this.injectScriptIntoFrameMenuItem.Click += new System.EventHandler(this.injectScriptIntoFrameMenuItem_Click);
            //
            // addInitializeScriptMenuItem
            //
            this.addInitializeScriptMenuItem.Name = "addInitializeScriptMenuItem";
            this.addInitializeScriptMenuItem.Size = new System.Drawing.Size(359, 44);
            this.addInitializeScriptMenuItem.Text = "Add Initialize Script";
            this.addInitializeScriptMenuItem.Click += new System.EventHandler(this.addInitializeScriptMenuItem_Click);
            //
            // removeInitializeScriptMenuItem
            //
            this.removeInitializeScriptMenuItem.Name = "removeInitializeScriptMenuItem";
            this.removeInitializeScriptMenuItem.Size = new System.Drawing.Size(359, 44);
            this.removeInitializeScriptMenuItem.Text = "Remove Initialize Script";
            this.removeInitializeScriptMenuItem.Click += new System.EventHandler(this.removeInitializeScriptMenuItem_Click);
            //
            // transparentBackgroundColorMenuItem
            //
            this.transparentBackgroundColorMenuItem.Name = "transparentBackgroundColorMenuItem";
            this.transparentBackgroundColorMenuItem.Size = new System.Drawing.Size(359, 44);
            this.transparentBackgroundColorMenuItem.Text = "Transparent";
            this.transparentBackgroundColorMenuItem.Click += new System.EventHandler(this.backgroundColorMenuItem_Click);
            //
            // scenarioToolStripMenuItem
            //
            this.scenarioToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripMenuItem[] {
                this.addRemoteObjectMenuItem,
                this.domContentLoadedMenuItem,
                this.navigateWithWebResourceRequestMenuItem,
                this.webMessageMenuItem });
            this.scenarioToolStripMenuItem.Name = "scenarioToolStripMenuItem";
            this.scenarioToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.scenarioToolStripMenuItem.Text = "Scenario";
            //
            // addRemoteObjectMenuItem
            //
            this.addRemoteObjectMenuItem.Name = "addRemoteObjectMenuItem";
            this.addRemoteObjectMenuItem.Size = new System.Drawing.Size(359, 44);
            this.addRemoteObjectMenuItem.Text = "Add Remote Object";
            this.addRemoteObjectMenuItem.Click += new System.EventHandler(this.addRemoteObjectMenuItem_Click);
            //
            // domContentLoadedMenuItem
            // 
            this.domContentLoadedMenuItem.Name = "domContentLoadedMenuItem";
            this.domContentLoadedMenuItem.Size = new System.Drawing.Size(359, 44);
            this.domContentLoadedMenuItem.Text = "DOM Content Loaded";
            this.domContentLoadedMenuItem.Click += new System.EventHandler(this.domContentLoadedMenuItem_Click);
            //
            // navigateWithWebResourceRequestMenuItem
            //
            this.navigateWithWebResourceRequestMenuItem.Name = "navigateWithWebResourceRequestMenuItem";
            this.navigateWithWebResourceRequestMenuItem.Size = new System.Drawing.Size(359, 44);
            this.navigateWithWebResourceRequestMenuItem.Text = "Navigate with WebResourceRequest";
            this.navigateWithWebResourceRequestMenuItem.Click += new System.EventHandler(this.navigateWithWebResourceRequestMenuItem_Click);
            //
            // webMessageMenuItem
            //
            this.webMessageMenuItem.Name = "webMessageMenuItem";
            this.webMessageMenuItem.Size = new System.Drawing.Size(359, 44);
            this.webMessageMenuItem.Text = "Web Message";
            this.webMessageMenuItem.Click += new System.EventHandler(this.webMessageMenuItem_Click);
            //
            // processToolStripMenuItem
            //
            this.processToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.showBrowserProcessInfoMenuItem,
                this.crashBrowserProcessMenuItem,
                this.crashRendererProcessMenuItem,
                this.showPerformanceInfoMenuItem});
            this.processToolStripMenuItem.Name = "processToolStripMenuItem";
            this.processToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.processToolStripMenuItem.Text = "Process";
            //
            // showBrowserProcessInfoMenuItem
            //
            this.showBrowserProcessInfoMenuItem.Name = "showBrowserProcessInfoMenuItem";
            this.showBrowserProcessInfoMenuItem.Size = new System.Drawing.Size(359, 44);
            this.showBrowserProcessInfoMenuItem.Text = "Show Browser Process Info";
            this.showBrowserProcessInfoMenuItem.Click += new System.EventHandler(this.showBrowserProcessInfoMenuItem_Click);
            //
            // crashBrowserProcessMenuItem
            //
            this.crashBrowserProcessMenuItem.Name = "crashBrowserProcessMenuItem";
            this.crashBrowserProcessMenuItem.Size = new System.Drawing.Size(359, 44);
            this.crashBrowserProcessMenuItem.Text = "Crash Browser Process";
            this.crashBrowserProcessMenuItem.Click += new System.EventHandler(this.crashBrowserProcessMenuItem_Click);
            //
            // crashRendererProcessMenuItem
            //
            this.crashRendererProcessMenuItem.Name = "crashRendererProcessMenuItem";
            this.crashRendererProcessMenuItem.Size = new System.Drawing.Size(359, 44);
            this.crashRendererProcessMenuItem.Text = "Crash Renderer Process";
            this.crashRendererProcessMenuItem.Click += new System.EventHandler(this.crashRendererProcessMenuItem_Click);
            //
            // showPerformanceInfoMenuItem
            //
            this.showPerformanceInfoMenuItem.Name = "showPerformanceInfoMenuItem";
            this.showPerformanceInfoMenuItem.Size = new System.Drawing.Size(359, 44);
            this.showPerformanceInfoMenuItem.Text = "Show Performance Info";
            this.showPerformanceInfoMenuItem.Click += new System.EventHandler(this.showPerformanceInfoMenuItem_Click);
            //
            // audioToolStripMenuItem
            //
            this.audioToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripMenuItem[] {
                this.toggleMuteStateMenuItem });
            this.audioToolStripMenuItem.Name = "audioToolStripMenuItem";
            this.audioToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.audioToolStripMenuItem.Text = "Audio";
            //
            // toggleMuteStateStripMenuItem
            //
            this.toggleMuteStateMenuItem.Name = "toggleMuteStateMenuItem";
            this.toggleMuteStateMenuItem.Size = new System.Drawing.Size(359, 44);
            this.toggleMuteStateMenuItem.Text = "Toggle Mute State";
            this.toggleMuteStateMenuItem.Click += new System.EventHandler(this.toggleMuteStateMenuItem_Click);
            this.toggleMuteStateMenuItem.Checked = true;
            this.toggleMuteStateMenuItem.CheckOnClick = true;
            //
            // helpToolStripMenuItem
            //
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                this.aboutToolStripMenuItem });
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(72, 38);
            this.helpToolStripMenuItem.Text = "Help";
            //
            // aboutToolStripMenuItem
            //
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(359, 44);
            this.aboutToolStripMenuItem.Text = "About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);

            //
            // webView2Control
            //
            this.webView2Control.Location = new System.Drawing.Point(0, 96);
            this.webView2Control.Name = "webView2Control";
            this.webView2Control.Size = new System.Drawing.Size(788, 410);

            this.webView2Control.CreationProperties = this.CreationProperties;

            this.webView2Control.Source = new Uri("https://www.bing.com/");
            this.webView2Control.DefaultBackgroundColor = System.Drawing.Color.Transparent;
            this.webView2Control.TabIndex = 7;
            //
            // BrowserForm
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(12F, 25F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1580, 865);
            this.Controls.Add(this.webView2Control);
            this.BackgroundImage = (System.Drawing.Image)this.webViewLogoBitmap;
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.Controls.Add(this.btnGo);
            this.Controls.Add(this.txtUrl);
            this.Controls.Add(this.btnStop);
            this.Controls.Add(this.btnRefresh);
            this.Controls.Add(this.btnForward);
            this.Controls.Add(this.btnBack);
            this.Controls.Add(this.btnEvents);
            this.Controls.Add(this.menuStrip1);
            this.MainMenuStrip = this.menuStrip1;
            this.Margin = new System.Windows.Forms.Padding(6);
            this.Name = "BrowserForm";
            this.Text = "BrowserForm";
            this.Resize += new System.EventHandler(this.Form_Resize);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.webView2Control)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();
        }
        #endregion

        private System.Windows.Forms.Button btnEvents;
        private System.Windows.Forms.Button btnBack;
        private System.Windows.Forms.Button btnForward;
        private System.Windows.Forms.Button btnRefresh;
        private System.Windows.Forms.Button btnStop;
        private System.Windows.Forms.Button btnGo;
        private System.Windows.Forms.TextBox txtUrl;
        private System.Drawing.Bitmap webViewLogoBitmap;
        private Microsoft.Web.WebView2.WinForms.WebView2 webView2Control;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem windowToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem closeWebViewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem createWebViewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem createNewWindowToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem createNewWindowWithOptionsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem createNewThreadToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem controlToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem acceleratorKeysEnabledToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem zoomToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem scriptToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem xToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem xToolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem xToolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem xToolStripMenuItem3;
        private System.Windows.Forms.ToolStripMenuItem backgroundColorMenuItem;
        private System.Windows.Forms.ToolStripMenuItem whiteBackgroundColorMenuItem;
        private System.Windows.Forms.ToolStripMenuItem redBackgroundColorMenuItem;
        private System.Windows.Forms.ToolStripMenuItem blueBackgroundColorMenuItem;
        private System.Windows.Forms.ToolStripMenuItem taskManagerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem methodCDPToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem transparentBackgroundColorMenuItem;
        private System.Windows.Forms.ToolStripMenuItem allowExternalDropMenuItem;
        private System.Windows.Forms.ToolStripMenuItem setUsersAgentMenuItem;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem getDocumentTitleMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitMenuItem;
        private System.Windows.Forms.ToolStripMenuItem getUserDataFolderMenuItem;
        private System.Windows.Forms.ToolStripMenuItem printToPDFMenuItem;
        private System.Windows.Forms.ToolStripMenuItem portraitMenuItem;
        private System.Windows.Forms.ToolStripMenuItem landscapeMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toggleVisibilityMenuItem;
        private System.Windows.Forms.ToolStripMenuItem serverCertificateErrorMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toggleCustomServerCertificateSupportMenuItem;
        private System.Windows.Forms.ToolStripMenuItem clearServerCertificateErrorActionsMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toggleDefaultScriptDialogsMenuItem;
        private System.Windows.Forms.ToolStripMenuItem scenarioToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem addRemoteObjectMenuItem;
        private System.Windows.Forms.ToolStripMenuItem domContentLoadedMenuItem;
        private System.Windows.Forms.ToolStripMenuItem navigateWithWebResourceRequestMenuItem;
        private System.Windows.Forms.ToolStripMenuItem webMessageMenuItem;
        private System.Windows.Forms.ToolStripMenuItem processToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem showBrowserProcessInfoMenuItem;
        private System.Windows.Forms.ToolStripMenuItem crashBrowserProcessMenuItem;
        private System.Windows.Forms.ToolStripMenuItem crashRendererProcessMenuItem;
        private System.Windows.Forms.ToolStripMenuItem showPerformanceInfoMenuItem;
        private System.Windows.Forms.ToolStripMenuItem audioToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toggleMuteStateMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem blockedDomainsMenuItem;
        private System.Windows.Forms.ToolStripMenuItem injectScriptMenuItem;
        private System.Windows.Forms.ToolStripMenuItem injectScriptIntoFrameMenuItem;
        private System.Windows.Forms.ToolStripMenuItem addInitializeScriptMenuItem;
        private System.Windows.Forms.ToolStripMenuItem removeInitializeScriptMenuItem;

    }
}
