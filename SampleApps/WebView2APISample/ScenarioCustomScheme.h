// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioCustomScheme : public ComponentBase
{
public:
    ScenarioCustomScheme(AppWindow* appWindow);
    ~ScenarioCustomScheme() override;

private:
    EventRegistrationToken m_webResourceRequestedToken = {};
    EventRegistrationToken m_navigationCompletedToken = {};

    wil::com_ptr<ICoreWebView2_22> m_webView2_22;

    AppWindow* m_appWindow = nullptr;
};
