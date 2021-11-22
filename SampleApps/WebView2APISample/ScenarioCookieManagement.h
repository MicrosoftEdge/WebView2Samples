// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioCookieManagement : public ComponentBase
{
public:
    ScenarioCookieManagement(AppWindow* appWindow);
    ~ScenarioCookieManagement() override;

private:
    void GetCookiesHelper(std::wstring uri);
    void SetupEventsOnWebview();

    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2CookieManager> m_cookieManager;
    std::wstring m_sampleUri;
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
};
