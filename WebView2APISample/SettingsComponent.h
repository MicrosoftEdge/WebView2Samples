// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <functional>
#include <string>
#include <vector>

#include "AppWindow.h"
#include "ComponentBase.h"

// This component handles commands from the Settings menu.  It also handles the
// NavigationStarting, FrameNavigationStarting, WebResourceRequested, ScriptDialogOpening,
// and PermissionRequested events.
class SettingsComponent : public ComponentBase
{
public:
    SettingsComponent(
        AppWindow* appWindow, ICoreWebView2Environment* environment,
        SettingsComponent* old = nullptr);

    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void ChangeBlockedSites();
    bool ShouldBlockUri(PWSTR uri);
    bool ShouldBlockScriptForUri(PWSTR uri);
    void SetBlockImages(bool blockImages);
    void ChangeUserAgent();
    void SetUserAgent(const std::wstring& userAgent);
    void CompleteScriptDialogDeferral();

    ~SettingsComponent() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Settings> m_settings;

    bool m_blockImages = false;
    bool m_deferScriptDialogs = false;
    bool m_changeUserAgent = false;
    bool m_isScriptEnabled = true;
    bool m_blockedSitesSet = false;
    std::vector<std::wstring> m_blockedSites;
    std::wstring m_overridingUserAgent;
    std::function<void()> m_completeDeferredDialog;

    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_frameNavigationStartingToken = {};
    EventRegistrationToken m_webResourceRequestedTokenForImageBlocking = {};
    EventRegistrationToken m_webResourceRequestedTokenForUserAgent = {};
    EventRegistrationToken m_scriptDialogOpeningToken = {};
    EventRegistrationToken m_permissionRequestedToken = {};
};

