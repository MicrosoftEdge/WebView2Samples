// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

std::wstring PermissionKindToString(COREWEBVIEW2_PERMISSION_KIND type);

std::wstring PermissionStateToString(COREWEBVIEW2_PERMISSION_STATE state);

class ScenarioPermissionManagement : public ComponentBase
{
public:
    ScenarioPermissionManagement(AppWindow* appWindow);
    ~ScenarioPermissionManagement() override;

    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;

private:
    void NavigateToPermissionManager();
    void ShowSetPermissionDialog();

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_2> m_webView2;
    wil::com_ptr<ICoreWebView2Profile4> m_webViewProfile4;
    std::wstring m_sampleUri;
    EventRegistrationToken m_DOMContentLoadedToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
};
