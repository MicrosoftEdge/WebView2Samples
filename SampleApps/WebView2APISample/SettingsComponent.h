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
#include "CustomStatusBar.h"

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
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;

    void AddMenuItems(
        HMENU hPopupMenu, wil::com_ptr<ICoreWebView2ContextMenuItemCollection> items);

    void ChangeBlockedSites();
    bool ShouldBlockUri(PWSTR uri);
    bool ShouldBlockScriptForUri(PWSTR uri);
    void SetBlockImages(bool blockImages);
    void SetReplaceImages(bool replaceImages);
    void ChangeUserAgent();
    void SetUserAgent(const std::wstring& userAgent);
    void EnableCustomClientCertificateSelection();
    void ToggleCustomServerCertificateSupport();
    void SetTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL value);

    ~SettingsComponent() override;

private:
    HRESULT OnPermissionRequested(
        ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args);
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_5> m_webView2_5;
    wil::com_ptr<ICoreWebView2_11> m_webView2_11;
    wil::com_ptr<ICoreWebView2_12> m_webView2_12;
    wil::com_ptr<ICoreWebView2_13> m_webView2_13;
    wil::com_ptr<ICoreWebView2_14> m_webView2_14;
    wil::com_ptr<ICoreWebView2_15> m_webView2_15;
    wil::com_ptr<ICoreWebView2Settings> m_settings;
    wil::com_ptr<ICoreWebView2Settings2> m_settings2;
    wil::com_ptr<ICoreWebView2Settings3> m_settings3;
    wil::com_ptr<ICoreWebView2Settings4> m_settings4;
    wil::com_ptr<ICoreWebView2Settings5> m_settings5;
    wil::com_ptr<ICoreWebView2Settings6> m_settings6;
    wil::com_ptr<ICoreWebView2Settings7> m_settings7;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2Controller3> m_controller3;
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Experimental5> m_webViewExperimental5;
    wil::com_ptr<ICoreWebView2ContextMenuItem> m_displayPageUrlContextSubMenuItem;
    bool m_blockImages = false;
    bool m_replaceImages = false;
    bool m_changeUserAgent = false;
    bool m_isScriptEnabled = true;
    bool m_blockedSitesSet = false;
    bool m_raiseClientCertificate = false;
    BOOL m_allowCustomMenus = false;
    std::map<std::tuple<std::wstring, COREWEBVIEW2_PERMISSION_KIND, BOOL>, bool>
        m_cached_permissions;
    std::vector<std::wstring> m_blockedSites;
    std::wstring m_overridingUserAgent;
    ULONG_PTR gdiplusToken_;
    bool m_faviconChanged = false;
    wil::unique_hicon m_favicon;
    CustomStatusBar m_statusBar;
    bool m_customStatusBar = false;
    bool m_raiseServerCertificateError = false;

    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_frameNavigationStartingToken = {};
    EventRegistrationToken m_webResourceRequestedTokenForImageBlocking = {};
    EventRegistrationToken m_webResourceRequestedTokenForImageReplacing = {};
    EventRegistrationToken m_webResourceRequestedTokenForUserAgent = {};
    EventRegistrationToken m_scriptDialogOpeningToken = {};
    EventRegistrationToken m_permissionRequestedToken = {};
    EventRegistrationToken m_ClientCertificateRequestedToken = {};
    EventRegistrationToken m_contextMenuRequestedToken = {};
    EventRegistrationToken m_faviconChangedToken = {};
    EventRegistrationToken m_statusBarTextChangedToken = {};
    EventRegistrationToken m_ServerCertificateErrorToken = {};
};
