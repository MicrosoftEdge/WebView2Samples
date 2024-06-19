// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioScreenCapture : public ComponentBase
{
public:
    ScenarioScreenCapture(AppWindow* appWindow);
    ~ScenarioScreenCapture() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_4> m_webView4;
    wil::com_ptr<ICoreWebView2Experimental26> m_webViewExperimental26;
    wil::com_ptr<ICoreWebView2ExperimentalFrame6> m_experimentalFrame6;
    std::wstring m_sampleUri;
    std::map<int, BOOL> m_screenCaptureFrameIdPermission;
    BOOL m_mainFramePermission = TRUE;
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_frameCreatedToken = {};
    EventRegistrationToken m_screenCaptureStartingToken = {};
    EventRegistrationToken m_frameScreenCaptureStartingToken = {};
};
