// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

// Demonstrates configuring WebRTC UDP port ranges via WebView2 environment options.
class ScenarioWebRtcUdpPortConfiguration : public ComponentBase
{
public:
    ScenarioWebRtcUdpPortConfiguration(AppWindow* appWindow);
    ~ScenarioWebRtcUdpPortConfiguration() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    EventRegistrationToken m_contentLoadingToken = {};
    std::wstring m_sampleUri;
};
