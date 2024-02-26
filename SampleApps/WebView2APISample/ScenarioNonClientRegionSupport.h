// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioNonClientRegionSupport : public ComponentBase
{
public:
    ScenarioNonClientRegionSupport(AppWindow* appWindow);
    void AddChangeListener();
    ~ScenarioNonClientRegionSupport() override;

private:
    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Settings> m_settings;

    wil::com_ptr<ICoreWebView2Settings9> m_settings9;
    wil::com_ptr<ICoreWebView2CompositionController4> m_compController4;
    std::wstring m_sampleUri;

    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_ContentLoadingToken = {};
    EventRegistrationToken m_nonClientRegionChanged = {};
};