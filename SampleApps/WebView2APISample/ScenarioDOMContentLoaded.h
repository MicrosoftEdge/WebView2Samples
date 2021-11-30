// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioDOMContentLoaded : public ComponentBase
{
public:
    ScenarioDOMContentLoaded(AppWindow* appWindow);
    ~ScenarioDOMContentLoaded() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_2> m_webView2;
    std::wstring m_sampleUri;
    EventRegistrationToken m_DOMContentLoadedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_frameCreatedToken = {};
};
