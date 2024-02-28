// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioFileSystemHandleShare : public ComponentBase
{
public:
    ScenarioFileSystemHandleShare(AppWindow* appWindow);
    ~ScenarioFileSystemHandleShare() override;

private:
    EventRegistrationToken m_navigationCompletedToken = {};

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView = nullptr;
};