// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioAcceleratorKeyPressed : public ComponentBase
{
public:
    ScenarioAcceleratorKeyPressed(AppWindow* appWindow);
    ~ScenarioAcceleratorKeyPressed() override;

private:
    EventRegistrationToken m_acceleratorKeyPressedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};

    AppWindow* m_appWindow = nullptr;
    std::wstring m_sampleUri;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Settings3> m_settings3;
};