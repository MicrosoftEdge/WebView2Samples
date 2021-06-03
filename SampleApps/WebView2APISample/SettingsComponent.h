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
    void SetReplaceImages(bool replaceImages);
    void ChangeUserAgent();
    void SetUserAgent(const std::wstring& userAgent);
    void CompleteScriptDialogDeferral();
    void EnableCustomClientCertificateSelection();

    ~SettingsComponent() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Settings> m_settings;
    wil::com_ptr<ICoreWebView2Settings2> m_settings2;
    wil::com_ptr<ICoreWebView2Settings3> m_settings3;
    wil::com_ptr<ICoreWebView2Settings4> m_settings4;
    wil::com_ptr<ICoreWebView2Settings5> m_settings5;
    wil::com_ptr<ICoreWebView2Experimental3> m_webViewExperimental;

    bool m_blockImages = false;
    bool m_replaceImages = false;
    bool m_deferScriptDialogs = false;
    bool m_changeUserAgent = false;
    bool m_isScriptEnabled = true;
    bool m_blockedSitesSet = false;
    bool m_raiseClientCertificate = false;
    std::map<std::tuple<std::wstring, COREWEBVIEW2_PERMISSION_KIND, BOOL>, bool>
        m_cached_permissions;
    std::vector<std::wstring> m_blockedSites;
    std::wstring m_overridingUserAgent;
    std::function<void()> m_completeDeferredDialog;

    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_frameNavigationStartingToken = {};
    EventRegistrationToken m_webResourceRequestedTokenForImageBlocking = {};
    EventRegistrationToken m_webResourceRequestedTokenForImageReplacing = {};
    EventRegistrationToken m_webResourceRequestedTokenForUserAgent = {};
    EventRegistrationToken m_scriptDialogOpeningToken = {};
    EventRegistrationToken m_permissionRequestedToken = {};
    EventRegistrationToken m_ClientCertificateRequestedToken = {};
};
