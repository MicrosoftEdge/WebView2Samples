// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioServiceWorkerPostMessage : public ComponentBase
{
public:
    ScenarioServiceWorkerPostMessage(AppWindow* appWindow);
    ~ScenarioServiceWorkerPostMessage() override;

private:
    void SetupEventsOnWebview();
    void CreateServiceWorkerManager();
    void SetupEventsOnServiceWorker(
        wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker);
    void AddToCache(
        std::wstring url, wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker);

    AppWindow* m_appWindow;
    ULONGLONG m_ServiceWorkerStartTimeViaNewMethod;
    ULONGLONG m_ServiceWorkerStartTimeViaMainThread;
    wil::com_ptr<ICoreWebView2> m_webView;
    std::wstring m_sampleUri;
    wil::com_ptr<ICoreWebView2ExperimentalServiceWorkerManager> m_serviceWorkerManager;
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_serviceWorkerRegisteredToken = {};
};
