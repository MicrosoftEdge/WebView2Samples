// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioSharedWorkerWRR : public ComponentBase
{
public:
    ScenarioSharedWorkerWRR(AppWindow* appWindow);
    ~ScenarioSharedWorkerWRR() override;

private:
    EventRegistrationToken m_webResourceRequestedToken = {};

    wil::com_ptr<ICoreWebView2> m_webView;
};
