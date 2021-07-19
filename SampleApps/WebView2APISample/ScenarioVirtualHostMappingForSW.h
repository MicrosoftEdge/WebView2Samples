#pragma once
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioVirtualHostMappingForSW : public ComponentBase
{
public:
    ScenarioVirtualHostMappingForSW(AppWindow* appWindow);
    ~ScenarioVirtualHostMappingForSW() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    std::wstring m_sampleUri;
    EventRegistrationToken m_contentLoadingToken = {};
};
