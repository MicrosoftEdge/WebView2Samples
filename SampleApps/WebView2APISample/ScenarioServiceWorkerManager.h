// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioServiceWorkerManager : public ComponentBase
{
public:
    ScenarioServiceWorkerManager(AppWindow* appWindow);
    ~ScenarioServiceWorkerManager() override;

    void GetAllServiceWorkerRegistrations();
    void GetServiceWorkerRegisteredForScope();

private:
    void SetupEventsOnWebview();
    void CreateServiceWorkerManager();

    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2ExperimentalServiceWorkerManager> m_serviceWorkerManager;
    EventRegistrationToken m_serviceWorkerRegisteredToken = {};
};
