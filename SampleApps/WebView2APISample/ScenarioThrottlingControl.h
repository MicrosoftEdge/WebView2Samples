// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioThrottlingControl : public ComponentBase
{
public:
    ScenarioThrottlingControl(AppWindow* appWindow);
    ~ScenarioThrottlingControl() override;

private:
    void WebViewMessageReceived(ICoreWebView2WebMessageReceivedEventArgs* args);

    // Owner/target AppWindow.
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webview;
    void OnWebViewCreated();
    void SetupIsolatedFramesHandler();

    int m_defaultIntervalForeground = 0;
    int m_defaultIntervalBackground = 0;
    int m_defaultIntervalIntensive = 0;
    int m_defaultIntervalOverride = 0;

    // Monitor AppWindow
    AppWindow* m_monitorAppWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_monitorWebview;

    std::wstring m_targetUri;
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_frameCreatedToken = {};

    // Handles commands from the monitor AppWindow
    EventRegistrationToken m_webMessageReceivedToken = {};

    // Scenarios
    void OnNoUserInteraction();
    void OnUserInteraction();
    void HideWebView();
    void ShowWebView();
};
