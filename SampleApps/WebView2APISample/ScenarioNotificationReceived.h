// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioNotificationReceived : public ComponentBase
{
public:
    ScenarioNotificationReceived(AppWindow* appWindow);
    ~ScenarioNotificationReceived() override;

    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;

private:
    void NavigateToNotificationPage();
    void ShowNotification(
        ICoreWebView2ExperimentalNotification* notification, std::wstring origin);
    void RemoveNotification(ICoreWebView2ExperimentalNotification* notification);

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Experimental22> m_webView2Experimental22;
    std::wstring m_sampleUri;
    EventRegistrationToken m_notificationReceivedToken = {};
    EventRegistrationToken m_notificationCloseRequestedToken = {};
};
