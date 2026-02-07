// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"
#include "ComponentBase.h"
#include "SLMHostObjectImpl.h"

class AppWindow;

class ScenarioSLM : public ComponentBase
{
public:
    ScenarioSLM(AppWindow* appWindow);
    ~ScenarioSLM() override;

    static bool CanLaunch();

private:
    static bool AreFileUrisEqual(const std::wstring& leftUri, const std::wstring& rightUri);

    AppWindow* m_appWindow = nullptr;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
    Microsoft::WRL::ComPtr<SLMHostObjectImpl> m_hostObject;
    EventRegistrationToken m_navigationStartingToken = {};
};
