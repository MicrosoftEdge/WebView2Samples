// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

// This sample demonstrates how to intercept Service Worker requests with
// WebResourceRequested event.
class ScenarioServiceWorkerWRR : public ComponentBase
{
public:
    ScenarioServiceWorkerWRR(AppWindow* appWindow);
    ~ScenarioServiceWorkerWRR() override;

private:
    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView;
    std::wstring m_sampleUri;
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_webResourceRequestedToken = {};
};
