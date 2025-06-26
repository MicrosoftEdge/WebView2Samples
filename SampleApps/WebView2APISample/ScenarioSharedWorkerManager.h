// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioSharedWorkerManager : public ComponentBase
{
public:
    ScenarioSharedWorkerManager(AppWindow* appWindow);
    ~ScenarioSharedWorkerManager() override;

    void GetAllSharedWorkers();

private:
    void SetupEventsOnWebview();
    void GetSharedWorkerManager();

    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2ExperimentalSharedWorkerManager> m_sharedWorkerManager;
    EventRegistrationToken m_sharedWorkerCreatedToken = {};
};
