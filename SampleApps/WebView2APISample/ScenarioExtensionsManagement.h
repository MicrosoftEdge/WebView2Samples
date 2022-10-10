// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioExtensionsManagement : public ComponentBase
{
public:
    ScenarioExtensionsManagement(AppWindow* appWindow, bool offload);

private:
    // If this scenario is run, then the user is choosing to install all default extensions.
    // Any extension marked as default would be installed if it is not already installed.
    // If it is already installed but is disabled, it would be enabled. If it is already
    // installed and enabled, then there is no work to be done. The user can also manually
    // install any extension marked as default. This scenario is just a way to install and
    // enable all default extensions (if not already installed and enabled).
    void InstallDefaultExtensions();

    // If this scenario is run, then the user is choosing to offload default extensions.
    // These default extensions would only be removed if there are too many extensions
    // installed. In the case of this scenario, "too many extensions" is considered to be more
    // than `m_maxInstalledExtensions` which is set to 2, however this is just a simplified way
    // to indicate having too many extensions installed. The user can manually remove any
    // extensions before running the scenario. When the scenario is run, if the number of
    // installed extensions is less than or equal to `m_maxInstalledExtensions`, then none of
    // them would be offloaded (regardless of whether any of those extensions are marked as
    // default). If more than `m_maxInstalledExtensions` extensions are installed, then all
    // default extensions would be offloaded since we have the case of "too many extensions".
    void OffloadDefaultExtensionsIfExtraExtensionsInstalled();

    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2> m_webView;

    // This extension ID is for the "Example DevTools Extension" located under
    // assets/extensions. This extension is treated as the default extension for the
    // `InstallDefaultExtensions` and `OffloadDefaultExtensionsIfExtraExtensionsInstalled`
    // scenarios.
    const std::wstring m_extensionId = L"bhjomlfgkfmjffpoikcekmhcblpgefmc";

    // `m_maxInstalledExtensions` is considered to be the maximum number of extensions we can
    // have installed, before we have the case of "too many extensions". If we have too many
    // extensions, then the `OffloadDefaultExtensionsIfExtraExtensionsInstalled` scenario
    // offloads the default extensions.
    const UINT m_maxInstalledExtensions = 2;
};
