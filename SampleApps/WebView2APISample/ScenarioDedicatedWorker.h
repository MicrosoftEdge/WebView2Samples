// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioDedicatedWorker : public ComponentBase
{
public:
    ScenarioDedicatedWorker(AppWindow* appWindow);
    ~ScenarioDedicatedWorker() override;

private:
    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2Experimental30> m_webView2Experimental_30;
    EventRegistrationToken m_dedicatedWorkerCreatedToken = {};
};
