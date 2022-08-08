// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioCustomSchemeNavigate : public ComponentBase
{
public:
    ScenarioCustomSchemeNavigate(AppWindow* appWindow);
    ~ScenarioCustomSchemeNavigate() override;

private:
    EventRegistrationToken m_webResourceRequestedToken = {};
    EventRegistrationToken m_navigationCompletedToken = {};

    AppWindow* m_appWindow = nullptr;
};
