namespace WinForms_GettingStarted
{
    partial class Form1
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
            this.webView = new Microsoft.Web.WebView2.WinForms.WebView2();
            this.addressBar = new System.Windows.Forms.TextBox();
            this.goButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // webView
            // 
            this.webView.AccessibleName = "webView";
            this.webView.Location = new System.Drawing.Point(1, 27);
            this.webView.Name = "webView";
            this.webView.Size = new System.Drawing.Size(800, 424);
            this.webView.Source = new System.Uri("https://microsoft.com", System.UriKind.Absolute);
            this.webView.TabIndex = 0;
            this.webView.ZoomFactor = 1D;
            this.webView.Click += new System.EventHandler(this.webView21_Click);
            // 
            // addressBar
            // 
            this.addressBar.AccessibleName = "addressBar";
            this.addressBar.Location = new System.Drawing.Point(1, 2);
            this.addressBar.Name = "addressBar";
            this.addressBar.Size = new System.Drawing.Size(729, 20);
            this.addressBar.TabIndex = 1;
            // 
            // goButton
            // 
            this.goButton.AccessibleName = "goButton";
            this.goButton.Location = new System.Drawing.Point(727, 1);
            this.goButton.Name = "goButton";
            this.goButton.Size = new System.Drawing.Size(74, 21);
            this.goButton.TabIndex = 2;
            this.goButton.Text = "Go!";
            this.goButton.UseVisualStyleBackColor = true;
            this.goButton.Click += new System.EventHandler(this.goButton_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.goButton);
            this.Controls.Add(this.addressBar);
            this.Controls.Add(this.webView);
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Microsoft.Web.WebView2.WinForms.WebView2 webView;
        private System.Windows.Forms.TextBox addressBar;
        private System.Windows.Forms.Button goButton;
    }
}

