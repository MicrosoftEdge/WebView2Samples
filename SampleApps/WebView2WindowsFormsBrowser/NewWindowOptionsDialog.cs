// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System.Windows.Forms;
using Microsoft.Web.WebView2.WinForms;

namespace WebView2WindowsFormsBrowser
{
    // Use visual studio's toolbox to draw form or window, and automatically generate UI code.
    // See https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls/walkthrough-arranging-controls-on-windows-forms-using-a-flowlayoutpanel?view=netframeworkdesktop-4.8
    public partial class NewWindowOptionsDialog : Form
    {
        public NewWindowOptionsDialog()
        {
            InitializeComponent();

            CreationProperties = new CoreWebView2CreationProperties();
        }

        private CoreWebView2CreationProperties _creationProperties = null;
        public CoreWebView2CreationProperties CreationProperties
        {
            get
            {
                return _creationProperties;
            }
            set
            {
                _creationProperties = value;
                if (_creationProperties == null)
                {
                    // Reset the controls to defaults.
                    BrowserExecutableFolder.Text = null;
                    UserDataFolder.Text = null;
                    EnvLanguage.Text = null;
                    ProfileName.Text = null;
                    comboBox_IsInPrivateModeEnabled.SelectedIndex = 2;
                }
                else
                {
                    // Copy the values to the controls.
                    BrowserExecutableFolder.Text = _creationProperties.BrowserExecutableFolder;
                    UserDataFolder.Text = _creationProperties.UserDataFolder;
                    EnvLanguage.Text = _creationProperties.Language;
                    ProfileName.Text = _creationProperties.ProfileName;
                    if (_creationProperties.IsInPrivateModeEnabled == null)
                    {
                        comboBox_IsInPrivateModeEnabled.SelectedIndex = 2;
                    }
                    else if (_creationProperties.IsInPrivateModeEnabled == true)
                    {
                        comboBox_IsInPrivateModeEnabled.SelectedIndex = 0;
                    }
                    else if (_creationProperties.IsInPrivateModeEnabled == false)
                    {
                        comboBox_IsInPrivateModeEnabled.SelectedIndex = 1;
                    }
                }
            }
        }

        private void OKBtn_Click(object sender, System.EventArgs e)
        {
            CreationProperties.BrowserExecutableFolder = BrowserExecutableFolder.Text == "" ? null : BrowserExecutableFolder.Text;
            CreationProperties.UserDataFolder = UserDataFolder.Text == "" ? null : UserDataFolder.Text;
            CreationProperties.Language = EnvLanguage.Text == "" ? null : EnvLanguage.Text;
            CreationProperties.ProfileName = ProfileName.Text == "" ? null : ProfileName.Text;
            if (comboBox_IsInPrivateModeEnabled.SelectedIndex == 0)
            {
                CreationProperties.IsInPrivateModeEnabled = true;
            }
            else if (comboBox_IsInPrivateModeEnabled.SelectedIndex == 1)
            {
                CreationProperties.IsInPrivateModeEnabled = false;
            }
            else if (comboBox_IsInPrivateModeEnabled.SelectedIndex == 2)
            {
                CreationProperties.IsInPrivateModeEnabled = null;
            }

            this.DialogResult = DialogResult.OK;
        }

        private void CancelBtn_Click(object sender, System.EventArgs e)
        {
            Close();
        }
    }
}
