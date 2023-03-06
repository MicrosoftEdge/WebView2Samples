// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using webview2_sample_uwp;
using Windows.ApplicationModel.Core;
using Windows.Storage;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;


namespace WebView2_UWP.Pages
{
    public sealed partial class SettingsPage : Page
    {
        private class EnvVarSetting : INotifyPropertyChanged
        {
            public string Name { get; set; }
            public string Value { get; set; }
            public string DocsUri { get; set; }
            public Visibility BrowseButtonVisibility { get; set; } = Visibility.Collapsed;
            public bool RestartRequired { get; set; } = false;
            public Action<string> SettingsUpdater { get; set; }

            public void UpdateSettings()
            {
                SettingsUpdater(Value);
            }

            public event PropertyChangedEventHandler PropertyChanged;

            public void SetProperty(string name, object value)
            {
                typeof(EnvVarSetting).GetProperty(name).SetValue(this, value);
                PropertyChanged(this, new PropertyChangedEventArgs(name));
            }
        };

        private class AppDetailsItem
        {
            public string Name { get; set; }
            public string Value { get; set; }
        }

        private ObservableCollection<EnvVarSetting> envVarSettings;
        private ObservableCollection<AppDetailsItem> appDetailsItems;

        private string _webViewVersionForTitleBar = "0.0.0.0";

        public SettingsPage()
        {
            InitializeComponent();
            InitializeEnvVarSettings();
            InitializeAppDetailsItems();
        }

        private void InitializeEnvVarSettings()
        {
            var settings = App.Instance.Settings;

            envVarSettings = new ObservableCollection<EnvVarSetting>()
            {
                new EnvVarSetting
                {
                    Name = Settings.WebViewExecutableFolderEnvKey,
                    Value = settings.WebViewExecutableFolder,
                    DocsUri = "https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution#details-about-the-fixed-version-runtime-distribution-mode",
                    BrowseButtonVisibility = Visibility.Visible,
                    SettingsUpdater = value => settings.WebViewExecutableFolder = value,
                    RestartRequired = true
                },
                new EnvVarSetting
                {
                    Name = Settings.WebViewUserDataFolderEnvKey,
                    Value = settings.WebViewUserDataFolder,
                    DocsUri = "https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder",
                    BrowseButtonVisibility = Visibility.Visible,
                    SettingsUpdater = value => settings.WebViewUserDataFolder = value
                },
                new EnvVarSetting
                {
                    Name = Settings.WebViewReleaseChannelPrefEnvKey,
                    Value = settings.WebViewReleaseChannelPreference,
                    DocsUri = "https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/set-preview-channel#which-app-is-affected",
                    SettingsUpdater = value => settings.WebViewReleaseChannelPreference = value,
                    RestartRequired = true
                },
                new EnvVarSetting
                {
                    Name = Settings.WebViewAdditionalArgumentsEnvKey,
                    Value = settings.WebViewAdditionalArguments,
                    DocsUri = "https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2environmentoptions?view=webview2-winrt-1.0.1587.40#additionalbrowserarguments",
                    SettingsUpdater = value => settings.WebViewAdditionalArguments = value,
                    RestartRequired = true
                }
            };
        }

        private async void InitializeAppDetailsItems()
        {
            appDetailsItems = new ObservableCollection<AppDetailsItem>();

            var currentPackage = Windows.ApplicationModel.Package.Current;
            appDetailsItems.Add(new AppDetailsItem
            {
                Name = "Installed App Location",
                Value = currentPackage.InstalledPath
            });

            appDetailsItems.Add(new AppDetailsItem
            {
                Name = "App Data Folder",
                Value = ApplicationData.Current.LocalFolder.Path
            });

            var dependenciesList = currentPackage.Dependencies.Select(p => p.DisplayName);
            var dependencies = string.Join(", ", dependenciesList);
            appDetailsItems.Add(new AppDetailsItem
            {
                Name = "App Dependencies",
                Value = dependencies
            });

            WebView2 webview2 = new WebView2();
            webview2.CoreWebView2Initialized +=
                (WebView2 sender, CoreWebView2InitializedEventArgs args) =>
                {
                    if (args.Exception != null)
                    {
                        appDetailsItems.Add(new AppDetailsItem
                        {
                            Name = "WebView2 Error",
                            Value = args.Exception.Message
                        });
                    }
                    else
                    {
                        _webViewVersionForTitleBar = sender.GetExtendedVersionString(false);
                        appDetailsItems.Add(new AppDetailsItem
                        {
                            Name = "WebView2 Version",
                            Value = sender.GetExtendedVersionString(true)
                        });

                        appDetailsItems.Add(new AppDetailsItem
                        {
                            Name = "WebView2 Runtime Path",
                            Value = sender.GetRuntimePath()
                        });

                        appDetailsItems.Add(new AppDetailsItem
                        {
                            Name = "WebView2 User Data Folder",
                            Value = sender.CoreWebView2.Environment.UserDataFolder
                        });
                    }
                };
            await webview2.EnsureCoreWebView2Async();
        }

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            WebViewVersionInTitleBarToggle.IsOn = App.Instance.Settings.ShowWebViewVersionInTitleBar;
        }

        private void WebViewVersionInTitleBarToggle_Toggled(object sender, RoutedEventArgs e)
        {
            var settings = App.Instance.Settings;
            if (settings.ShowWebViewVersionInTitleBar != WebViewVersionInTitleBarToggle.IsOn)
            {
                settings.ShowWebViewVersionInTitleBar = WebViewVersionInTitleBarToggle.IsOn;
                App.Instance.UpdateAppTitle(_webViewVersionForTitleBar);
            }
        }

        private async void EnvVarBrowseButton_Click(object sender, RoutedEventArgs e)
        {
            var folderPicker = new Windows.Storage.Pickers.FolderPicker();
            folderPicker.SuggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.ComputerFolder;
            folderPicker.FileTypeFilter.Add("*");

            StorageFolder folder = await folderPicker.PickSingleFolderAsync();
            if (folder != null)
            {
                var envVarSetting = ((Button)sender).DataContext as EnvVarSetting;
                envVarSetting.SetProperty(nameof(envVarSetting.Value), folder.Path);
            }
        }

        private void EnvVarApplyButton_Click(object sender, RoutedEventArgs e)
        {
            var envVarSetting = ((Button)sender).DataContext as EnvVarSetting;
            envVarSetting.UpdateSettings();

            if (envVarSetting.RestartRequired)
            {
                RestartRequiredTextBlock.Visibility = Visibility.Visible;
            }
        }

        private void EnvVarClearButton_Click(object sender, RoutedEventArgs e)
        {
            var envVarSetting = ((Button)sender).DataContext as EnvVarSetting;
            envVarSetting.SetProperty(nameof(envVarSetting.Value), "");
            envVarSetting.UpdateSettings();

            if (envVarSetting.RestartRequired)
            {
                RestartRequiredTextBlock.Visibility = Visibility.Visible;
            }
        }

        private async void RestartButton_Click(object sender, RoutedEventArgs e)
        {
            var result = await CoreApplication.RequestRestartAsync("");

            if (result == AppRestartFailureReason.NotInForeground ||
                result == AppRestartFailureReason.RestartPending ||
                result == AppRestartFailureReason.InvalidUser ||
                result == AppRestartFailureReason.Other)
            {
                var msgBox = new MessageDialog(result.ToString(), "Restart Failed");
                await msgBox.ShowAsync();
            }
        }
    }
}
