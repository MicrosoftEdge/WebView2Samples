// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioSharedBuffer : public ComponentBase
{
public:
    ScenarioSharedBuffer(AppWindow* appWindow);

    ~ScenarioSharedBuffer() override;

private:
    void WebViewMessageReceived(ICoreWebView2WebMessageReceivedEventArgs* args, bool fromFrame);
    void EnsureSharedBuffer();
    void DisplaySharedBufferData();

    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_17> m_webView17;
    wil::com_ptr<ICoreWebView2Frame4> m_webviewFrame4;
    wil::com_ptr<ICoreWebView2SharedBuffer> m_sharedBuffer;
    std::wstring m_sampleUri;
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_frameCreatedToken = {};
};
