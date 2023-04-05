// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.Wpf;

namespace WebView2WpfBrowser
{
    /// <summary>
    /// Interaction logic for NewWindowOptionsDialog.xaml
    /// </summary>
    public partial class NewWindowOptionsDialog : Window
    {
        public NewWindowOptionsDialog()
        {
            InitializeComponent();

            CreationProperties = new CoreWebView2CreationProperties();
            BrowserExecutableFolder.Focus();
            BrowserExecutableFolder.SelectAll();
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
                    ScriptLocale.Text = null;
                }
                else
                {
                    // Copy the values to the controls.
                    BrowserExecutableFolder.Text = _creationProperties.BrowserExecutableFolder;
                    UserDataFolder.Text = _creationProperties.UserDataFolder;
                    EnvLanguage.Text = _creationProperties.Language;
                    ProfileName.Text = _creationProperties.ProfileName;
                    ScriptLocale.Text = _creationProperties.ScriptLocale;
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

        void OK_Clicked (object sender, RoutedEventArgs args) {
            CreationProperties.BrowserExecutableFolder = BrowserExecutableFolder.Text == "" ? null : BrowserExecutableFolder.Text;
            CreationProperties.UserDataFolder = UserDataFolder.Text == "" ? null : UserDataFolder.Text;
            CreationProperties.Language = EnvLanguage.Text == "" ? null : EnvLanguage.Text;
            CreationProperties.ProfileName = ProfileName.Text == "" ? null : ProfileName.Text;
            CreationProperties.ScriptLocale = ScriptLocale.Text == "" ? null : ScriptLocale.Text;
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

            this.DialogResult = true;
        }
    }
}
