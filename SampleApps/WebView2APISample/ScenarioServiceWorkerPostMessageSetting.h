// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioServiceWorkerPostMessageSetting : public ComponentBase
{
public:
    ScenarioServiceWorkerPostMessageSetting(AppWindow* appWindow);
    ~ScenarioServiceWorkerPostMessageSetting() override;
    void SetupEventsOnWebview();
    void SetupEventsOnServiceWorker(
        wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker);

    void ToggleServiceWorkerJsApiSetting();
    void UnregisterAllServiceWorkers();

private:
    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2ExperimentalServiceWorkerManager> m_serviceWorkerManager;
    std::wstring m_sampleUri;
    EventRegistrationToken m_serviceWorkerRegisteredToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
};
