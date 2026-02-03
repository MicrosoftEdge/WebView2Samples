// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"
#include "SLMHostObject_h.h"
#include "FoundryLocalClient.h"

class ScenarioSLM : public ComponentBase
{
public:
    ScenarioSLM(AppWindow* appWindow);
    ~ScenarioSLM() override;

    // Check if SLM scenario can be launched (online or model cached)
    static bool CanLaunch();

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<SLMHostObject> m_hostObject;

    EventRegistrationToken m_navigationStartingToken = {};
    
    // Helper to check URI match
    bool AreFileUrisEqual(const std::wstring& leftUri, const std::wstring& rightUri);
};
