// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"
#include "HostObjectSample_h.h"

class ScenarioAddHostObject : public ComponentBase
{
public:
    ScenarioAddHostObject(AppWindow* appWindow);
    ~ScenarioAddHostObject() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<HostObjectSample> m_hostObject;

    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_frameCreatedToken = {};
};
