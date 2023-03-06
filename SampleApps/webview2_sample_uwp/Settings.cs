// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using Windows.Foundation.Collections;
using Windows.Storage;

namespace WebView2_UWP
{
    public class Settings
    {
        private const int SettingsVersion = 1;
        private IPropertySet _localSettings = null;

        // Settings keys
        private const string SettingsVersionKey = "SettingsVersion";
        private const string WebViewUserDataFolderKey = "WebViewUserDataFolder";
        private const string WebViewExecutableFolderKey = "WebViewExecutableFolder";
        private const string WebViewReleaseChannelPrefKey = "WebViewReleaseChannelPreference";
        private const string WebViewAdditionalArgumentsKey = "WebViewAdditionalBrowserArguments";
        private const string ShowWebViewVersionInTitleBarKey = "ShowWebViewVersionInTitleBar";

        // Environmental variable keys
        public const string WebViewUserDataFolderEnvKey = "WEBVIEW2_USER_DATA_FOLDER";
        public const string WebViewExecutableFolderEnvKey = "WEBVIEW2_BROWSER_EXECUTABLE_FOLDER";
        public const string WebViewReleaseChannelPrefEnvKey = "WEBVIEW2_RELEASE_CHANNEL_PREFERENCE";
        public const string WebViewAdditionalArgumentsEnvKey = "WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS";

        public Settings()
        {
            _localSettings = ApplicationData.Current.LocalSettings.Values;

            if (_localSettings.ContainsKey(SettingsVersionKey))
            {
                int version = (int)_localSettings[SettingsVersionKey];
                if (version != SettingsVersion)
                {
                    // There are currently no other versions of
                    // settings so throw an exception for anything
                    // other than the current version.
                    var errorMessage = $"Unsupported settings version. Found version {version}.";
                    throw new ApplicationException(errorMessage);
                }
            }
            else
            {
                _localSettings[SettingsVersionKey] = SettingsVersion;
            }

#if USE_WEBVIEW2_SMOKETEST
            var webViewExecutableFolder = ApplicationData.Current.LocalFolder.Path + "\\EdgeBin";
            Environment.SetEnvironmentVariable(WebViewExecutableFolderEnvKey, webViewExecutableFolder);
#else
            InitializeEnvSetting(WebViewExecutableFolderKey, WebViewExecutableFolderEnvKey);
#endif
            InitializeEnvSetting(WebViewUserDataFolderKey, WebViewUserDataFolderEnvKey);
            InitializeEnvSetting(WebViewReleaseChannelPrefKey, WebViewReleaseChannelPrefEnvKey);
            InitializeEnvSetting(WebViewAdditionalArgumentsKey, WebViewAdditionalArgumentsEnvKey);
        }

        private void InitializeEnvSetting(string settingsKey, string envKey)
        {
            // Environmental variables override values stored in the local settings.
            var envValue = Environment.GetEnvironmentVariable(envKey);
            if (envValue != null)
            {
                _localSettings[settingsKey] = envValue;
            }
            else if (_localSettings.TryGetValue(settingsKey, out object value))
            {
                var settingsValue = value as string;

                if (!string.IsNullOrEmpty(settingsValue))
                {
                    Environment.SetEnvironmentVariable(envKey, settingsValue);
                }
            }
        }

        private T GetLocalSetting<T>(string key, T defaultValue)
        {
            return _localSettings.ContainsKey(key) ? (T)_localSettings[key] : defaultValue;
        }

        public bool ShowWebViewVersionInTitleBar
        {
            get { return GetLocalSetting<bool>(ShowWebViewVersionInTitleBarKey, true); }
            set { _localSettings[ShowWebViewVersionInTitleBarKey] = value; }
        }

        public string WebViewExecutableFolder
        {
            get { return GetLocalSetting<string>(WebViewExecutableFolderKey, string.Empty); }
            set { _localSettings[WebViewExecutableFolderKey] = value; }
        }

        public string WebViewReleaseChannelPreference
        {
            get { return GetLocalSetting<string>(WebViewReleaseChannelPrefKey, string.Empty); }
            set { _localSettings[WebViewReleaseChannelPrefKey] = value; }
        }

        public string WebViewUserDataFolder
        {
            get { return GetLocalSetting<string>(WebViewUserDataFolderKey, string.Empty); }
            set
            {
                _localSettings[WebViewUserDataFolderKey] = value;
                Environment.SetEnvironmentVariable(WebViewUserDataFolderEnvKey, value);
            }
        }

        public string WebViewAdditionalArguments
        {
            get { return GetLocalSetting<string>(WebViewAdditionalArgumentsKey, string.Empty); }
            set
            {
                _localSettings[WebViewAdditionalArgumentsKey] = value;
                Environment.SetEnvironmentVariable(WebViewAdditionalArgumentsEnvKey, value);
            }
        }
    }
}
