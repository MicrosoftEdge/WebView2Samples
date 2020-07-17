// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioAuthentication : public ComponentBase
{
public:
    ScenarioAuthentication(AppWindow* appWindow);
    ~ScenarioAuthentication() override;

private:
    EventRegistrationToken m_webResourceResponseReceivedToken = {};
    AppWindow* m_appWindow = nullptr;
};
